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

#include "PerformanceTimer.h"
#include <QDebug>

void
PerformanceTimer::print(char const* prefix)
{
	timer::time_point const now = timer::now();
	uint64_t const usec = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count();
	if (usec < 10000) {
		qDebug() << prefix << usec << "usec";
	} else if (usec < 10000000) {
		qDebug() << prefix << (usec / 1000) << "msec";
	} else {
		qDebug() << prefix << (usec / 1000000) << "sec";
	}
}
