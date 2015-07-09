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

#ifndef CACHING_FACTORY_H_
#define CACHING_FACTORY_H_

#include "foundation_config.h"
#include <boost/optional.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

/**
 * @brief Wraps a factory functor to provide caching of the returned value.
 *
 * @note Multiple copies of CachingFactory share the same cache.
 * @note This class is thread-safe.
 */
template<typename T>
class CachingFactory
{
private:
	struct SharedState
	{
		std::mutex mutex;
		std::function<T()> factory;
		boost::optional<T> cached;

		template<typename F>
		SharedState(F&& factory) : factory(std::forward<F>(factory)) {}
	};
public:
	template<typename F>
	explicit CachingFactory(F&& factory)
	: m_ptrState(std::make_shared<SharedState>(std::forward<F>(factory))) {}

	T operator()() const {
		SharedState& s = *m_ptrState;
		std::lock_guard<std::mutex> const lock(s.mutex);

		if (!s.cached) {
			s.cached = s.factory();
		}

		return *s.cached;
	}

	bool isCached() const {
		SharedState& s = *m_ptrState;
		std::lock_guard<std::mutex> const lock(s.mutex);
		return s.cached.is_initialized();
	}

	void clearCache() {
		SharedState& s = *m_ptrState;
		std::lock_guard<std::mutex> const lock(s.mutex);
		s.cached.reset();
	}
private:
	std::shared_ptr<SharedState> m_ptrState;
};


/**
 * @brief Wraps a factory functor into a CachingFactory.
 */
template<typename T, typename F>
CachingFactory<T> cachingFactory(F&& factory)
{
	return CachingFactory<T>(std::forward<F>(factory));
}

#endif
