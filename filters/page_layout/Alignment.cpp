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
    m_val = vert|hor;
}

Alignment::Alignment(Vertical vert, Horizontal hor, bool is_null, double tolerance)
    : m_val(vert|hor), m_isNull(is_null), m_tolerance(tolerance)
{
}


Alignment::Alignment(QDomElement const& el)
{
    CommandLine cli = CommandLine::get();
    m_isNull = cli.getDefaultNull(); 
    
	QString const vert(el.attribute("vert"));
	QString const hor(el.attribute("hor"));
	m_isNull = el.attribute("null").toInt() != 0;
	m_tolerance = el.attribute("tolerance", QString::number(DEFAULT_TOLERANCE)).toDouble();

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
    settings.setValue("alignment/default_alignment_vert", verticalToStr(vertical()));
    settings.setValue("alignment/default_alignment_hor", horizontalToStr(horizontal()));
    settings.setValue("alignment/default_alignment_null", m_isNull);
    settings.setValue("alignment/default_alignment_tolerance", m_tolerance);
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
    Vertical vert = strToVertical( settings.value("alignment/default_alignment_vert", verticalToStr(Vertical::VCENTER)).toString() );
    Horizontal hor = strToHorizontal( settings.value("alignment/default_alignment_hor", horizontalToStr(Horizontal::HCENTER)).toString() );
    CommandLine cli = CommandLine::get();

    bool isnull = cli.getDefaultNull();
    /* read in cli.getDefaultNull(); */
//    m_isNull = settings.value("alignment/default_alignment_null", false).toBool();
    double toler = settings.value("alignment/default_alignment_tolerance", DEFAULT_TOLERANCE).toDouble();
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
    bool has_side = false;
    bool has_corner = false;
    bool has_center = false;
    bool has_auto_magnet = false;
    bool has_orig_props = false;


    QString direction;

    if (val & Alignment::HCENTER) {
        direction = QObject::tr(" horizontally");
        has_center = true;
    }

    if (val & Alignment::VCENTER) {
        direction = direction.isEmpty() ? QObject::tr(" vertically") : "";
        has_center = true;
    }

    if (has_center) {
        direction = QObject::tr("centered%1").arg(direction);
    }

    QString hside;
    if (val & Alignment::LEFT) {
        hside = QObject::tr("left");
        has_side = true;
    } else if (val & Alignment::RIGHT) {
        hside = QObject::tr("right");
        has_side = true;
    }

    QString vside;
    if (val & Alignment::TOP) {
        vside = QObject::tr("top");
        has_side = true;
    }
    if (val & Alignment::BOTTOM) {
        vside = QObject::tr("bottom");
        has_side = true;
    }

    QString corner = !hside.isEmpty() && !vside.isEmpty() ? QObject::tr("%1-%2 corner").arg(vside).arg(hside) : "";
    if (!corner.isEmpty()) {
        has_corner = true;
        has_side = false;
    } else {
        hside = hside + vside + QObject::tr(" side");
    }

    QString auto_magnet;
    if (val & Alignment::HAUTO) {
        auto_magnet = QObject::tr(" by width");
        has_auto_magnet = true;
    }
    if (val & Alignment::VAUTO) {
        auto_magnet = auto_magnet.isEmpty()? QObject::tr(" by height") : "";
        has_auto_magnet = true;
    }

    if (has_auto_magnet) {
        auto_magnet = QObject::tr("automatically%1").arg(auto_magnet);
    }

    QString original_proportions;
    if (val & Alignment::HORIGINAL) {
        original_proportions = QObject::tr(" horizontal");
        has_orig_props = true;
    }
    if (val & Alignment::VORIGINAL) {
        original_proportions = original_proportions.isEmpty()? QObject::tr(" vertical") : "";
        has_orig_props = true;
    }

    if (has_orig_props) {
        original_proportions = QObject::tr("proportional to original%1 position").arg(original_proportions);
    }

    const QString plus = QObject::tr("%1 + %2");

    if (has_corner) {
        txt = corner;
    } else {
        if (has_side) {
            txt = txt.isEmpty()? hside : plus.arg(txt).arg(hside);
        }

        if (has_auto_magnet) {
            txt = txt.isEmpty()? auto_magnet : plus.arg(txt).arg(auto_magnet);
        }

        if (has_center) {
            txt = txt.isEmpty()? direction : plus.arg(txt).arg(direction);
        }

        if (has_orig_props) {
            txt = txt.isEmpty()? original_proportions : plus.arg(txt).arg(original_proportions);
        }
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
Alignment::strToHorizontal(const QString &val)
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

