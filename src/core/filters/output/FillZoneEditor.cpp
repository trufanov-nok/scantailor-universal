/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FillZoneEditor.h"

#include "ZoneContextMenuInteraction.h"
#include "ZoneContextMenuItem.h"
#include "ColorPickupInteraction.h"
#include "NonCopyable.h"
#include "Zone.h"
#include "ZoneSet.h"
#include "SerializableSpline.h"
#include "PropertySet.h"
#include "FillColorProperty.h"
#include "Settings.h"
#include "ImageTransformation.h"
#include "ImagePresentation.h"
#include "OutputMargins.h"
#include "VirtualZoneProperty.h"
#include <QTransform>
#include <QImage>
#include <QPointer>
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#endif
#include <vector>
#include <assert.h>

namespace output
{

class FillZoneEditor::MenuCustomizer
{
private:
    typedef ZoneContextMenuInteraction::StandardMenuItems StdMenuItems;
public:
    MenuCustomizer(FillZoneEditor* editor) : m_pEditor(editor) {}

    std::vector<ZoneContextMenuItem> operator()(
        EditableZoneSet::Zone const& zone, StdMenuItems const& std_items);
private:
    FillZoneEditor* m_pEditor;
};

FillZoneEditor::FillZoneEditor(
    QImage const& image, ImagePixmapUnion const& downscaled_version,
    boost::function<QPointF(QPointF const&)> const& orig_to_image,
    boost::function<QPointF(QPointF const&)> const& image_to_orig,
    PageId const& page_id, IntrusivePtr<Settings> const& settings)
    :   ImageViewBase(
            image, downscaled_version,
            ImagePresentation(QTransform(), QRectF(image.rect())),
            OutputMargins()
        ),
        m_colorAdapter(colorAdapterFor(image)),
        m_context(*this, m_zones),
        m_colorPickupInteraction(m_zones, m_context),
        m_dragHandler(*this),
        m_zoomHandler(*this),
        m_origToImage(orig_to_image),
        m_imageToOrig(image_to_orig),
        m_pageId(page_id),
        m_ptrSettings(settings)
{
    m_zones.setDefaultProperties(m_ptrSettings->defaultFillZoneProperties());

    setMouseTracking(true);

    m_context.setContextMenuInteractionCreator(
        boost::bind(&FillZoneEditor::createContextMenuInteraction, this, _1)
    );

    connect(&m_zones, SIGNAL(committed()), SLOT(commitZones()));

    makeLastFollower(*m_context.createDefaultInteraction());

    rootInteractionHandler().makeLastFollower(*this);

    // We want these handlers after zone interaction handlers,
    // as some of those have their own drag and zoom handlers,
    // which need to get events before these standard ones.
    rootInteractionHandler().makeLastFollower(m_dragHandler);
    rootInteractionHandler().makeLastFollower(m_zoomHandler);

    const ZoneSet zones = m_ptrSettings->fillZonesForPage(page_id);
    for (Zone const& zone : zones) {
        if (zone.type() == Zone::SplineType) {
            EditableSpline::Ptr spline(
                        new EditableSpline(zone.spline().transformed(m_origToImage))
                        );
            m_zones.addSpline(spline, zone.properties());
        } else if (zone.type() == Zone::EllipseType) {
            EditableEllipse::Ptr ellipse(
                        new EditableEllipse(zone.ellipse().transformed(m_origToImage))
                        );
            m_zones.addEllipse(ellipse, zone.properties());
        }
    }
}

FillZoneEditor::~FillZoneEditor()
{
    m_ptrSettings->setDefaultFillZoneProperties(m_zones.defaultProperties());
}

void
FillZoneEditor::onPaint(QPainter& painter, InteractionState const& interaction)
{
    if (m_colorPickupInteraction.isActive(interaction)) {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.setPen(Qt::NoPen);

    for (EditableZoneSet::Zone const& zone : qAsConst(m_zones)) {
        typedef FillColorProperty FCP;
        QColor const color(zone.properties()->locateOrDefault<FCP>()->color());
        painter.setBrush(m_colorAdapter(color));
        if (!zone.isEllipse()) {
            painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
        } else {
            const EditableEllipse::Ptr& e = zone.ellipse();
            const QPointF& center = e->center();

            QPainterPath path;
            path.addEllipse(center, e->rx(), e->ry());
            QTransform t;
            t.translate(center.x(), center.y());
            t.rotate(zone.ellipse()->angle());
            t.translate(-center.x(), -center.y());
            painter.drawPolygon(t.map(path).toFillPolygon(), Qt::WindingFill);
        }
    }
}

InteractionHandler*
FillZoneEditor::createContextMenuInteraction(InteractionState& interaction)
{
    // Return a standard ZoneContextMenuInteraction but with a customized menu.
    return ZoneContextMenuInteraction::create(
               m_context, interaction, MenuCustomizer(this)
           );
}

InteractionHandler*
FillZoneEditor::createColorPickupInteraction(
    EditableZoneSet::Zone const& zone, InteractionState& interaction)
{
    m_colorPickupInteraction.startInteraction(zone, interaction);
    return &m_colorPickupInteraction;
}

void
FillZoneEditor::commitZones()
{
    ZoneSet zones;

    for (EditableZoneSet::Zone const& zone : qAsConst(m_zones)) {
        if (!zone.isEllipse()) {
            SerializableSpline const spline(
                        SerializableSpline(*zone.spline()).transformed(m_imageToOrig)
                        );
            zones.add(Zone(spline, *zone.properties()));
        } else {
            SerializableEllipse const ellipse(
                        SerializableEllipse(*zone.ellipse()).transformed(m_imageToOrig)
                        );
            zones.add(Zone(ellipse, *zone.properties()));
        }
    }

    m_ptrSettings->setFillZones(m_pageId, zones);

    emit invalidateThumbnail(m_pageId);
}

void
FillZoneEditor::updateRequested()
{
    update();
}

QColor
FillZoneEditor::toOpaque(QColor const& color)
{
    QColor adapted(color);
    adapted.setAlpha(0xff);
    return adapted;
}

QColor
FillZoneEditor::toGrayscale(QColor const& color)
{
    int const gray = qGray(color.rgb());
    return QColor(gray, gray, gray);
}

QColor
FillZoneEditor::toBlackWhite(QColor const& color)
{
    int const gray = qGray(color.rgb());
    return gray < 128 ? Qt::black : Qt::white;
}

FillZoneEditor::ColorAdapter
FillZoneEditor::colorAdapterFor(QImage const& image)
{
    switch (image.format()) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        return &FillZoneEditor::toBlackWhite;
    case QImage::Format_Indexed8:
        if (image.allGray()) {
            return &FillZoneEditor::toGrayscale;
        }
    // fall through
    default:
        return &FillZoneEditor::toOpaque;
    }
}

/*=========================== MenuCustomizer =========================*/

std::vector<ZoneContextMenuItem>
FillZoneEditor::MenuCustomizer::operator()(
    EditableZoneSet::Zone const& zone, StdMenuItems const& std_items)
{
    bool was_copyed_to = false;
    if (zone.properties().get()) {
        IntrusivePtr<output::VirtualZoneProperty> ptrSet =
            zone.properties()->locate<output::VirtualZoneProperty>();
        was_copyed_to = ptrSet.get() && !ptrSet->uuid().isEmpty();
    }

    std::vector<ZoneContextMenuItem> items;
    items.reserve(was_copyed_to ? 6 : 5);
    items.push_back(
        ZoneContextMenuItem(
            tr("Pick color"),
            boost::bind(
                &FillZoneEditor::createColorPickupInteraction,
                m_pEditor, zone, _1
            )
        )
    );
    items.push_back(std_items.copyItem);
    items.push_back(std_items.copyToItem);
    if (was_copyed_to) {
        items.push_back(std_items.deleteFromItem);
    }
    items.push_back(std_items.deleteItem);

    return items;
}

} // namespace output
