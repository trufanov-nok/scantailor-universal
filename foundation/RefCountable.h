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

#ifndef REFCOUNTABLE_H_
#define REFCOUNTABLE_H_

#include "foundation_config.h"
#include <QAtomicInt>

class FOUNDATION_EXPORT RefCountable
{
public:
	RefCountable() : m_refCounter(0) {}
	
	RefCountable(RefCountable const& other) {
		// don't copy the reference counter!
	}
	
	void operator=(RefCountable const& other) {
		// don't copy the reference counter!
	}
	
	virtual ~RefCountable() {}
	
	void ref() const { m_refCounter.fetchAndAddRelaxed(1); }
	
	void unref() const {
		if (m_refCounter.fetchAndAddRelease(-1) == 1) {
			delete this;
		}
	}
private:
	mutable QAtomicInt m_refCounter;
};

#endif
