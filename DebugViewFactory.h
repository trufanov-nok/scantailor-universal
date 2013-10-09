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

#ifndef DEBUG_VIEW_FACTORY_H_
#define DEBUG_VIEW_FACTORY_H_

#include "RefCountable.h"
#include <QWidget>
#include <memory>

class DebugViewFactory : public RefCountable
{
public:
	/**
	 * On destruction, and files backing up swap data will be deleted.
	 */
	virtual ~DebugViewFactory() {}
	
	/**
	 * \brief Loads any data offloaded to disk back to memory.
	 *
	 * Does nothing if nothing was swapped out.
	 */
	virtual void swapIn() = 0;

	/**
	 * \brief Offloads any swappable data to disk, freeing up memory.
	 *
	 * Does nothing if data was swapped out already.
	 */
	virtual void swapOut() = 0;

	/**
	 * \brief Constructs a new debug view instance.
	 *
	 * Implementations will sandwich this code between swapIn() and swapOut()
	 * in case swap in is required. Otherwise, no swapIn() or swapOut() is called.
	 */
	virtual std::auto_ptr<QWidget> newInstance() = 0;	
};

#endif
