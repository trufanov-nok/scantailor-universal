/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef SCOPEDDECINC_H_
#define SCOPEDDECINC_H_

#include "foundation_config.h"

template<class T>
class ScopedDecInc
{
public:
	ScopedDecInc(T& counter) : m_counter(counter) { --counter; }
	
	~ScopedDecInc() { ++m_counter; }
private:
	T& m_counter;
};

#endif
