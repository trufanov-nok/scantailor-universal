/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "Dependencies.h"
#include "Params.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>
#include <QTransform>

namespace page_split
{

Dependencies::Dependencies()
{
}

Dependencies::Dependencies(QDomElement const& el)
    :   m_imageSize(XmlUnmarshaller::size(el.namedItem("size").toElement())),
        m_rotation(XmlUnmarshaller::rotation(el.namedItem("rotation").toElement())),
        m_layoutType(
            layoutTypeFromString(
                XmlUnmarshaller::string(el.namedItem("layoutType").toElement())
            )
        )
{
}

Dependencies::Dependencies(
    QSize const& image_size, OrthogonalRotation const rotation,
    LayoutType const layout_type)
    :   m_imageSize(image_size),
        m_rotation(rotation),
        m_layoutType(layout_type)
{
}

bool
Dependencies::compatibleWith(Params const& params) const
{
    Dependencies const& deps = params.dependencies();

    if (m_imageSize != deps.m_imageSize) {
        return false;
    }
    if (m_rotation != deps.m_rotation) {
        return false;
    }
    if (m_layoutType == deps.m_layoutType) {
        return true;
    }
    if (m_layoutType == SINGLE_PAGE_UNCUT) {
        // The split line doesn't matter here.
        return true;
    }
    if (m_layoutType == TWO_PAGES && params.splitLineMode() == MODE_MANUAL) {
        // Two pages and a specified split line means we have all the data.
        // Note that if layout type was PAGE_PLUS_OFFCUT, we would
        // not know if that page is to the left or to the right of the
        // split line.
        return true;
    }

    return false;
}

QDomElement
Dependencies::toXml(QDomDocument& doc, QString const& tag_name) const
{
    if (isNull()) {
        return QDomElement();
    }

    XmlMarshaller marshaller(doc);

    QDomElement el(doc.createElement(tag_name));
    el.appendChild(marshaller.rotation(m_rotation, "rotation"));
    el.appendChild(marshaller.size(m_imageSize, "size"));
    el.appendChild(marshaller.string(layoutTypeToString(m_layoutType), "layoutType"));

    return el;
}

bool
Dependencies::fixCompatibility(Params& params) const
{
    QTransform rotate = m_rotation.transform(m_imageSize);
    QSize new_size = rotate.map(QRegion(QRect(QPoint(0, 0), m_imageSize))).boundingRect().size();

    QSize old_size = params.dependencies().m_imageSize;
    rotate = params.dependencies().m_rotation.transform(old_size);
    old_size = rotate.map(QRegion(QRect(QPoint(0, 0), old_size))).boundingRect().size();

    qreal scale_x = (qreal) new_size.width() / old_size.width();
    qreal scale_y = (qreal) new_size.height() / old_size.height();

    PageLayout new_page_layout = params.pageLayout().transformed(QTransform::fromScale(scale_x, scale_y));

    params = Params(new_page_layout, *this, params.splitLineMode(), params.origDpi());
    return compatibleWith(params);
}

} // namespace page_split
