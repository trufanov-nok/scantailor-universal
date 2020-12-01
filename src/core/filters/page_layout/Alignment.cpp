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

#include "Alignment.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

#include "CommandLine.h"

namespace page_layout
{

Alignment::Alignment()
{
    *this = Alignment::load();
}

Alignment::Alignment(Vertical vert, Horizontal hor)
{
    *this = Alignment::load();
    m_val = vert | hor;
}

Alignment::Alignment(Vertical vert, Horizontal hor, bool is_null, double tolerance)
    : m_val(vert | hor), m_isNull(is_null), m_tolerance(tolerance)
{
}

Alignment::Alignment(QDomElement const& el)
{
    CommandLine cli = CommandLine::get();
    m_isNull = cli.getDefaultNull();

    QString const vert(el.attribute("vert"));
    QString const hor(el.attribute("hor"));
    m_isNull = el.attribute("null").toInt() != 0;
    m_tolerance = el.attribute("tolerance", QString::number(_key_alignment_default_alig_tolerance_def)).toDouble();

    m_val = strToVertical(vert);
    m_val |= strToHorizontal(hor);
}

QDomElement
Alignment::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    el.setAttribute("vert", verticalToStr(vertical()));
    el.setAttribute("hor", horizontalToStr(horizontal()));
    el.setAttribute("null", m_isNull ? 1 : 0);
    el.setAttribute("tolerance", QString::number(m_tolerance));
    return el;
}

void
Alignment::save(QSettings* _settings) const
{
    std::unique_ptr<QSettings> ptr;
    if (!_settings) {
        ptr.reset(new QSettings());
        _settings = ptr.get();
    }

    QSettings& settings = *_settings;
    settings.setValue(_key_alignment_default_alig_vert, verticalToStr(vertical()));
    settings.setValue(_key_alignment_default_alig_hor, horizontalToStr(horizontal()));
    settings.setValue(_key_alignment_default_alig_null, m_isNull);
    settings.setValue(_key_alignment_default_alig_tolerance, m_tolerance);
}

Alignment
Alignment::load(QSettings* _settings)
{
    std::unique_ptr<QSettings> ptr;
    if (!_settings) {
        ptr.reset(new QSettings());
        _settings = ptr.get();
    }

    QSettings& settings = *_settings;
    Vertical vert = strToVertical(settings.value(_key_alignment_default_alig_vert, verticalToStr(Vertical::VCENTER)).toString());
    Horizontal hor = strToHorizontal(settings.value(_key_alignment_default_alig_hor, horizontalToStr(Horizontal::HCENTER)).toString());
    CommandLine cli = CommandLine::get();

    bool isnull = cli.getDefaultNull();
    /* read in cli.getDefaultNull(); */
//    m_isNull = settings.value(_key_alignment_default_alig_null, _key_alignment_default_alig_null_def).toBool();
    double toler = settings.value(_key_alignment_default_alig_tolerance, _key_alignment_default_alig_tolerance_def).toDouble();
    return Alignment(vert, hor, isnull, toler);
}

QString
Alignment::getVerboseDescription(const Alignment& alignment)
{
    QString txt;
    if (alignment.isNull()) {
        return txt;
    }

    const int val = alignment.compositeAlignment();

    // We avoid combining txt from words and use phrases to easier localization
    QString center;
    if (val == (Alignment::HCENTER | Alignment::VCENTER)) {
        return QObject::tr("centered");
    } else if (val & Alignment::HCENTER) {
        center = QObject::tr("centered horizontally");
    } else if (val & Alignment::VCENTER) {
        center = QObject::tr("centered vertically");
    }

    QString side;
    if (val & Alignment::TOP) {
        if (val & Alignment::LEFT) {
            return QObject::tr("top-left corner");
        } else if (val & Alignment::RIGHT) {
            return QObject::tr("top-right corner");
        } else {
            side = QObject::tr("top side");
        }
    } else if (val & Alignment::BOTTOM) {
        if (val & Alignment::LEFT) {
            return QObject::tr("bottom-left corner");
        } else if (val & Alignment::RIGHT) {
            return QObject::tr("bottom-right corner");
        } else {
            side = QObject::tr("bottom side");
        }
    } else {
        if (val & Alignment::LEFT) {
            side = QObject::tr("left side");
        } else if (val & Alignment::RIGHT) {
            side = QObject::tr("right side");
        }
    }

    QString auto_magnet;
    if (val == (Alignment::HAUTO | Alignment::VAUTO)) {
        return QObject::tr("automatically");
    } else if (val & Alignment::HAUTO) {
        auto_magnet = QObject::tr("automatically by width");
    } else if (val & Alignment::VAUTO) {
        auto_magnet = QObject::tr("automatically by height");
    }

    QString original_proportions;
    if (val == (Alignment::HORIGINAL | Alignment::VORIGINAL)) {
        return QObject::tr("proportional to original position");
    } else if (val & Alignment::HORIGINAL) {
        original_proportions = QObject::tr("proportional to original horizontal position");
    } else if (val & Alignment::VORIGINAL) {
        original_proportions = QObject::tr("proportional to original vertical position");
    }

    const QString plus = QObject::tr("%1 + %2");

    if (!auto_magnet.isEmpty()) {
        txt = txt.isEmpty() ? auto_magnet : plus.arg(txt, auto_magnet);
    }

    if (!original_proportions.isEmpty()) {
        txt = txt.isEmpty() ? original_proportions : plus.arg(txt, original_proportions);
    }

    if (!side.isEmpty()) {
        txt = txt.isEmpty() ? side : plus.arg(txt, side);
    }

    if (!center.isEmpty()) {
        txt = txt.isEmpty() ? center : plus.arg(txt, center);
    }

    return txt;
}

QString
Alignment::getShortDescription(const Alignment& alignment)
{
    QString txt;
    if (alignment.isNull()) {
        return txt;
    }

    const int val = alignment.compositeAlignment();

    // We avoid combining txt from words and use phrases to easier localization
    QString center;
    if (val == (Alignment::HCENTER | Alignment::VCENTER)) {
        return QObject::tr("centered");
    } else if (val & Alignment::HCENTER) {
        center = QObject::tr("x: center");
    } else if (val & Alignment::VCENTER) {
        center = QObject::tr("y: center");
    }

    QString side;
    if (val & Alignment::TOP) {
        if (val & Alignment::LEFT) {
            return QObject::tr("top-left");
        } else if (val & Alignment::RIGHT) {
            return QObject::tr("top-right");
        } else {
            side = QObject::tr("y: top");
        }
    } else if (val & Alignment::BOTTOM) {
        if (val & Alignment::LEFT) {
            return QObject::tr("bottom-left");
        } else if (val & Alignment::RIGHT) {
            return QObject::tr("bottom-right");
        } else {
            side = QObject::tr("y: bottom");
        }
    } else {
        if (val & Alignment::LEFT) {
            side = QObject::tr("x: left");
        } else if (val & Alignment::RIGHT) {
            side = QObject::tr("x: right");
        }
    }

    QString auto_magnet;
    if (val == (Alignment::HAUTO | Alignment::VAUTO)) {
        return QObject::tr("auto");
    } else if (val & Alignment::HAUTO) {
        auto_magnet = QObject::tr("x: auto");
    } else if (val & Alignment::VAUTO) {
        auto_magnet = QObject::tr("y: auto");
    }

    QString original_proportions;
    if (val == (Alignment::HORIGINAL | Alignment::VORIGINAL)) {
        return QObject::tr("proportional");
    } else if (val & Alignment::HORIGINAL) {
        original_proportions = QObject::tr("x: prop.");
    } else if (val & Alignment::VORIGINAL) {
        original_proportions = QObject::tr("y: prop.");
    }

    const QString plus = QObject::tr("%1 + %2");

    if (!auto_magnet.isEmpty()) {
        txt = txt.isEmpty() ? auto_magnet : plus.arg(txt, auto_magnet);
    }

    if (!original_proportions.isEmpty()) {
        txt = txt.isEmpty() ? original_proportions : plus.arg(txt, original_proportions);
    }

    if (!side.isEmpty()) {
        txt = txt.isEmpty() ? side : plus.arg(txt, side);
    }

    if (!center.isEmpty()) {
        txt = txt.isEmpty() ? center : plus.arg(txt, center);
    }

    return txt;
}

const QString
Alignment::verticalToStr(Vertical val)
{
    switch (val) {
    case TOP:
        return "top";
    case VCENTER:
        return "vcenter";
    case BOTTOM:
        return "bottom";
    case VAUTO:
        return "vauto";
    case VORIGINAL:
        return "voriginal";
    }
    return "";
}

const QString
Alignment::horizontalToStr(Horizontal val)
{
    switch (val) {
    case LEFT:
        return "left";
    case HCENTER:
        return "hcenter";
    case RIGHT:
        return "right";
    case HAUTO:
        return "hauto";
    case HORIGINAL:
        return "horiginal";
    }
    return "";
}

Alignment::Vertical
Alignment::strToVertical(const QString& val)
{
    if (val == "top") {
        return TOP;
    } else if (val == "bottom") {
        return BOTTOM;
    } else if (val == "vauto") {
        return VAUTO;
    } else if (val == "voriginal") {
        return VORIGINAL;
    } else {
        return VCENTER;
    }
}

Alignment::Horizontal
Alignment::strToHorizontal(const QString& val)
{
    if (val == "left") {
        return LEFT;
    } else if (val == "right") {
        return RIGHT;
    } else if (val == "hauto") {
        return HAUTO;
    } else if (val == "horiginal") {
        return HORIGINAL;
    } else {
        return HCENTER;
    }
}

} // namespace page_layout

