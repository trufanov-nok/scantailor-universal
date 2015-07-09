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

#ifndef ADIFF_FUNCTION_H_
#define ADIFF_FUNCTION_H_

#include "math_config.h"
#include "SparseMap.h"
#include <Eigen/Core>
#include <cstddef>

namespace adiff
{

/**
 * \brief Represents a multivariable function and its derivatives.
 *
 * It would be nice to have a generic version,
 * but for now it's only specialized for ORD == 2.
 */
template<int ORD> class Function;

template<> class MATH_EXPORT Function<2>
{
	// Member-wise copying is OK.
public:
	/** The value of the function. */
	double value;

	/**
	 * First directional derivatives in the direction
	 * of u = i + j for every non-zero Hessian element at i, j.
	 */
	Eigen::VectorXd firstDerivs;

	/**
	 * Second directional derivatives in the direction
	 * of u = i + j for every non-zero Hessian element at i, j.
	 */
	Eigen::VectorXd secondDerivs;

	/**
	 * Constructs the "f(x1, x2, ...) = 0" function.
	 */
	explicit Function(size_t num_non_zero_vars);

	/**
	 * Constructs the "f(x1, x2, ...) = 0" function.
	 */
	explicit Function(SparseMap<2> const& sparse_map);

	/**
	 * Constructs a function representing an argument.
	 *
	 * \param arg_idx Argument number.
	 * \param val Argument value.
	 * \param sparse_map Tells which derivatives to compute.
	 */
	Function(size_t arg_idx, double val, SparseMap<2> const& sparse_map);

	Eigen::VectorXd gradient(SparseMap<2> const& sparse_map) const;

	Eigen::MatrixXd hessian(SparseMap<2> const& sparse_map) const;

	void swap(Function& other);

	Function& operator+=(Function const& other);

	Function& operator-=(Function const& other);

	Function& operator*=(double scalar);
};

inline void swap(Function<2>& f1, Function<2>& f2)
{
	f1.swap(f2);
}

MATH_EXPORT Function<2> operator+(Function<2> const& f1, Function<2> const& f2);

MATH_EXPORT Function<2> operator-(Function<2> const& f1, Function<2> const& f2);

MATH_EXPORT Function<2> operator*(Function<2> const& f1, Function<2> const& f2);

MATH_EXPORT Function<2> operator*(Function<2> const& f, double scalar);

MATH_EXPORT Function<2> operator*(double scalar, Function<2> const& f);

MATH_EXPORT Function<2> operator/(Function<2> const& num, Function<2> const& den);

} // namespace adiff

#endif
