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

#ifndef OBJECT_SWAPPER_FACTORY_H_
#define OBJECT_SWAPPER_FACTORY_H_

#include "foundation_config.h"
#include "ObjectSwapper.h"
#include <memory>
#include <QString>

class FOUNDATION_EXPORT ObjectSwapperFactory
{
public:
	explicit ObjectSwapperFactory(QString const& swap_dir, bool ensure_exists = true);

	template<typename Obj>
	ObjectSwapper<Obj> operator()(Obj const& obj) const {
		return ObjectSwapper<Obj>(obj, m_swapDir);
	}

	template<typename Obj>
	ObjectSwapper<Obj> operator()(std::auto_ptr<Obj> obj) const {
		return ObjectSwapper<Obj>(obj, m_swapDir);
	}
private:
	QString m_swapDir;
};

#endif
