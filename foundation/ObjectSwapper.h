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

#ifndef OBJECT_SWAPPER_H_
#define OBJECT_SWAPPER_H_

#include "foundation_config.h"
#include "ObjectSwapperImpl.h"
#include <boost/shared_ptr.hpp>
#include <memory>
#include <assert.h>

class QString;

/**
 * \brief Handles swapping in and out of a single object.
 *
 * \note Copying an ObjectSwapper object produces a shallow copy. 
 */
template<typename Obj>
class ObjectSwapper
{
private:
	typedef ObjectSwapperImpl<Obj> Impl;
public:
	class ConstObjectAccessor
	{
	public:
		typedef Obj const& result_type;

		ConstObjectAccessor(ObjectSwapper<Obj> const& obj_swapper) : m_objectSwapper(obj_swapper) {}

		Obj const& operator()() const { return m_objectSwapper.constObject(); }
	private:
		ObjectSwapper<Obj> m_objectSwapper;
	};

	ObjectSwapper(Obj const& obj, QString const& swap_dir)
		: m_ptrObj(new Obj(obj)), m_ptrImpl(new Impl(swap_dir)) {}

	ObjectSwapper(std::auto_ptr<Obj> obj, QString const& swap_dir)
		: m_ptrObj(obj.release()), m_ptrImpl(new Impl(swap_dir)) {
		assert(m_ptrObj.get());
	}

	void swapIn() { if (!m_ptrObj.get()) m_ptrObj = m_ptrImpl->swapIn(); }

	void swapOut() { if (m_ptrObj.get()) { m_ptrImpl->swapOut(m_ptrObj); m_ptrObj.reset(); } }

	bool swappedOut() const { return !m_ptrObj.get(); }

	/**
	 * \brief Exposes the stored object through a const reference.
	 *
	 * \note Calling this function when the object is swapped out results in undefined behavior.
	 */
	Obj const& constObject() const { assert(m_ptrObj.get()); return *m_ptrObj; }

	/**
	 * Returns a functor that returns Obj const& when called without arguments.
	 */
	ConstObjectAccessor accessor() const { return ConstObjectAccessor(*this); }
private:
	/** A null m_ptrObj indicates the object has been swapped out. */
	boost::shared_ptr<Obj> m_ptrObj;

	/** m_ptrImpl is always set. */
	boost::shared_ptr<Impl> m_ptrImpl;
};

#endif
