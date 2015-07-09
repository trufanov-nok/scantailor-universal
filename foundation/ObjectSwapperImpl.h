/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef OBJECT_SWAPPER_IMPL_H_
#define OBJECT_SWAPPER_IMPL_H_

#include "foundation_config.h"
#include <boost/shared_ptr.hpp>

class QString;

/**
 * This non-specialized version is defunct. It will compile but not link.
 * Its purpose is to illustrate the interface to be implemented by
 * specializations.
 */
template<typename Obj>
class ObjectSwapperImpl
{
public:
	ObjectSwapperImpl(QString const& swap_dir);

	boost::shared_ptr<Obj> swapIn();

	void swapOut(boost::shared_ptr<Obj> const& obj);
};

#endif
