/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Alexander Trufanov <trufanovan@gmail.com>

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

#include "OrderByRotation.h"
#include <QObject>

namespace fix_orientation
{

OrderByRotationProvider::OrderByRotationProvider(IntrusivePtr<Settings> const& settings)
    :   m_ptrSettings(settings)
{
}

bool
OrderByRotationProvider::precedes(
    PageId const& lhs_page, bool const /*lhs_incomplete*/,
    PageId const& rhs_page, bool const /*rhs_incomplete*/) const
{
    int const lrotation = m_ptrSettings->getRotationFor(lhs_page.imageId()).toDegrees();
    int const rrotation = m_ptrSettings->getRotationFor(rhs_page.imageId()).toDegrees();

    if (lrotation != rrotation) {
        return lrotation < rrotation;
    }

    return lhs_page < rhs_page;
}

QString
OrderByRotationProvider::hint(PageId const& page) const
{
    int const rotation = m_ptrSettings->getRotationFor(page.imageId()).toDegrees();
    QString res(QObject::tr("rotation: %1°"));
    return res.arg(rotation);
}

} // namespace fix_orientation
