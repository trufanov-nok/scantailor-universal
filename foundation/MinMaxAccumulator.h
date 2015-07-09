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

#ifndef MIN_MAX_ACCUMULATOR_H_
#define MIN_MAX_ACCUMULATOR_H_

#include "foundation_config.h"
#include "NumericTraits.h"

template<typename T>
class MinMaxAccumulator
{
public:
	typedef void result_type;

	explicit MinMaxAccumulator(T initial_min = NumericTraits<T>::max(), T initial_max = NumericTraits<T>::min())
	: m_min(initial_min), m_max(initial_max) {}

	void operator()(T val) {
		if (val < m_min) {
			m_min = val;
		}
		if (m_max < val) {
			m_max = val;
		}
	}

	T const& min() const { return m_min; }

	T const& max() const { return m_max; }
private:
	T m_min;
	T m_max;
};

#endif
