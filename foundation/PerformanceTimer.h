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

#ifndef PERFORMANCETIMER_H_
#define PERFORMANCETIMER_H_

#include "foundation_config.h"
#include <chrono>

class FOUNDATION_EXPORT PerformanceTimer
{
private:
	using timer = std::chrono::high_resolution_clock;
public:
	PerformanceTimer() : m_start(timer::now()) {}
	
	void print(char const* prefix = "");
private:
	timer::time_point const m_start;
};

#endif
