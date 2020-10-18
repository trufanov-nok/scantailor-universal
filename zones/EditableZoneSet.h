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

#ifndef EDITABLE_ZONE_SET_H_
#define EDITABLE_ZONE_SET_H_

#include "EditableSpline.h"
#include "EditableEllipse.h"
#include "PropertySet.h"
#include "IntrusivePtr.h"
#include <QObject>
#ifndef Q_MOC_RUN
#include <boost/mpl/bool.hpp>
#include <boost/iterator/iterator_facade.hpp>
#endif
#include <map>

class ZoneSet;

struct GenericZonePtr
{
    GenericZonePtr(): m_spline(nullptr), m_ellipse(nullptr) {}
    GenericZonePtr(EditableSpline::Ptr const& spline): m_spline(spline), m_ellipse(nullptr) {}
    GenericZonePtr(EditableEllipse::Ptr const& ellipse): m_spline(nullptr), m_ellipse(ellipse) {}
    bool isEllipse() const { return m_ellipse.get() != nullptr; }
    bool operator<(GenericZonePtr const& rhs) const {
        if (m_spline && rhs.m_spline) {
            return m_spline < rhs.m_spline;
        }
        if (m_ellipse && rhs.m_ellipse) {
            return m_ellipse < rhs.m_ellipse;
        }
        return m_ellipse.get();
    }

    EditableSpline::Ptr m_spline;
    EditableEllipse::Ptr m_ellipse;
};

class EditableZoneSet : public QObject
{
    Q_OBJECT
private:
    typedef std::map<GenericZonePtr, IntrusivePtr<PropertySet> > Map;
public:
    class const_iterator;

    class Zone
    {
        friend class EditableZoneSet::const_iterator;
    public:
        Zone() {}

        EditableSpline::Ptr const& spline() const
        {
            return m_iter->first.m_spline;
        }

        EditableEllipse::Ptr const& ellipse() const
        {
            return m_iter->first.m_ellipse;
        }

        bool isEllipse() const
        {
            return m_iter->first.isEllipse();
        }

        IntrusivePtr<PropertySet> const& properties() const
        {
            return m_iter->second;
        }
    private:
        explicit Zone(Map::const_iterator it) : m_iter(it) {}

        Map::const_iterator m_iter;
    };

    class const_iterator : public boost::iterator_facade <
        const_iterator, Zone const, boost::forward_traversal_tag
        >
    {
        friend class EditableZoneSet;
        friend class boost::iterator_core_access;
    public:
        const_iterator() : m_zone() {}

        void increment()
        {
            ++m_zone.m_iter;
        }

        bool equal(const_iterator const& other) const
        {
            return m_zone.m_iter == other.m_zone.m_iter;
        }

        Zone const& dereference() const
        {
            return m_zone;
        }
    private:
        explicit const_iterator(Map::const_iterator it) : m_zone(it) {}

        Zone m_zone;
    };

    typedef const_iterator iterator;

    EditableZoneSet();

    const_iterator begin() const
    {
        return iterator(m_zonesMap.begin());
    }

    const_iterator end() const
    {
        return iterator(m_zonesMap.end());
    }

    PropertySet const& defaultProperties() const
    {
        return m_defaultProps;
    }

    void setDefaultProperties(PropertySet const& props);

    void addSpline(EditableSpline::Ptr const& spline);

    void addSpline(EditableSpline::Ptr const& spline, PropertySet const& props);

    void removeSpline(EditableSpline::Ptr const& spline);

    void addEllipse(EditableEllipse::Ptr const& ellipse);

    void addEllipse(EditableEllipse::Ptr const& ellipse, PropertySet const& props);

    void removeEllipse(EditableEllipse::Ptr const& ellipse);

    void commit();

    IntrusivePtr<PropertySet> propertiesFor(EditableSpline::Ptr const& spline);

    IntrusivePtr<PropertySet const> propertiesFor(EditableSpline::Ptr const& spline) const;
signals:
    void committed();
    void manuallyDeleted();
private:
    Map m_zonesMap;
    PropertySet m_defaultProps;
};

#endif
