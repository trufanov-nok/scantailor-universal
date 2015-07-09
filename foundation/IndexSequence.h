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

#ifndef INDEX_SEQUENCE_H_
#define INDEX_SEQUENCE_H_

#include "foundation_config.h"
#include <cstddef>

/**
 * @brief The IndexSequence template type is used in template
 *        metaprogramming to unroll parameter packs.
 *
 * Typically you'll be interested in the form of
 * IndexSequence<0, 1, 2, ... N>. Such a form can be obtained
 * from IndexSequenceGenerator:
 * @code
 * IndexSequenceGenerator<N>::type
 * @endcode
 *
 * Usage example:
 * @code
 * #include "IndexSequence.h"
 * #include <cstddef>
 * #include <tuple>
 * #include <utility>
 * #include <iostream>
 *
 * void clientFunction(int arg1, double arg2)
 * {
 *     std::cout << "arg1: " << arg1 << ", arg2: " << arg2 << std::endl;
 * }
 *
 * template<typename F, typename Tuple, size_t... Idx>
 * void callWithParamsImpl(F func, Tuple params, IndexSequence<Idx...>)
 * {
 *     func(std::get<Idx>(params)...);
 * }
 *
 * template<typename F, typename Tuple>
 * void callWithParams(F func, Tuple params)
 * {
 *     using Seq = IndexSequenceGenerator<std::tuple_size<Tuple>::value>::type;
 *     callWithParamsImpl(func, params, Seq());
 * }
 *
 * int main()
 * {
 *     auto params = std::make_tuple(1, -0.5);
 *     callWithParams(clientFunction, params);
 * }
 * @endcode
 */
template<size_t... Indices>
struct IndexSequence
{
};

template<size_t remaining, size_t... Indices>
struct IndexSequenceGenerator :
	public IndexSequenceGenerator<remaining - 1, remaining - 1, Indices...>
{
};

template<size_t... Indices>
struct IndexSequenceGenerator<size_t(0), Indices...>
{
	using type = IndexSequence<Indices...>;
};

#endif
