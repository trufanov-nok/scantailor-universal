/*

    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "EditableZoneSet.h"

#include "ZoneSet.h"

EditableZoneSet::EditableZoneSet()
{
}

void
EditableZoneSet::setDefaultProperties(PropertySet const& props)
{
    m_defaultProps = props;
}

void
EditableZoneSet::addSpline(EditableSpline::Ptr const& spline)
{
    IntrusivePtr<PropertySet> new_props(new PropertySet(m_defaultProps));
    m_zonesMap.insert(Map::value_type(GenericZonePtr(spline), new_props));
}

void
EditableZoneSet::addSpline(EditableSpline::Ptr const& spline, PropertySet const& props)
{
    IntrusivePtr<PropertySet> new_props(new PropertySet(props));
    m_zonesMap.insert(Map::value_type(GenericZonePtr(spline), new_props));
}

void
EditableZoneSet::removeSpline(EditableSpline::Ptr const& spline)
{
    m_zonesMap.erase(GenericZonePtr(spline));
}

void
EditableZoneSet::addEllipse(EditableEllipse::Ptr const& ellipse)
{
    IntrusivePtr<PropertySet> new_props(new PropertySet(m_defaultProps));
    m_zonesMap.insert(Map::value_type(GenericZonePtr(ellipse), new_props));
}

void
EditableZoneSet::addEllipse(EditableEllipse::Ptr const& ellipse, PropertySet const& props)
{
    IntrusivePtr<PropertySet> new_props(new PropertySet(props));
    m_zonesMap.insert(Map::value_type(GenericZonePtr(ellipse), new_props));
}

void
EditableZoneSet::removeEllipse(EditableEllipse::Ptr const& ellipse)
{
    m_zonesMap.erase(GenericZonePtr(ellipse));
}

void
EditableZoneSet::commit()
{
    emit committed();
}

IntrusivePtr<PropertySet>
EditableZoneSet::propertiesFor(EditableSpline::Ptr const& spline)
{
    Map::iterator it(m_zonesMap.find(spline));
    if (it != m_zonesMap.end()) {
        return it->second;
    } else {
        return IntrusivePtr<PropertySet>();
    }
}

IntrusivePtr<PropertySet const>
EditableZoneSet::propertiesFor(EditableSpline::Ptr const& spline) const
{
    Map::const_iterator it(m_zonesMap.find(spline));
    if (it != m_zonesMap.end()) {
        return it->second;
    } else {
        return IntrusivePtr<PropertySet const>();
    }
}
