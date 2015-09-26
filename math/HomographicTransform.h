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

#ifndef HOMOGRAPHIC_TRANSFORM_H_
#define HOMOGRAPHIC_TRANSFORM_H_

#include "math_config.h"
#include <Eigen/Core>
#include <Eigen/LU>
#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <string>
#include <limits>

template<size_t N, typename T> class HomographicTransform;

template<size_t N, typename T>
class HomographicTransformBase
{
public:
	typedef Eigen::Matrix<T, N, 1> Vec;
	typedef Eigen::Matrix<T, N+1, N+1> Mat;

	explicit HomographicTransformBase(Mat const& mat) : m_mat(mat) {}

	/** @throw std::runtime_error if not invertible. */
	HomographicTransform<N, T> inv() const;

	Vec operator()(Vec const& from) const;

	Mat const& mat() const { return m_mat; }
private:
	Mat m_mat;
};


template<size_t N, typename T>
class HomographicTransform : public HomographicTransformBase<N, T>
{
public:
	explicit HomographicTransform(
		typename HomographicTransformBase<N, T>::Mat const& mat) : HomographicTransformBase<N, T>(mat) {}
};


/** A 1D specialisation providing additional methods. */
template<typename T>
class HomographicTransform<1, T> : public HomographicTransformBase<1, T>
{
public:
	explicit HomographicTransform(
		typename HomographicTransformBase<1, T>::Mat const& mat)
			: HomographicTransformBase<1, T>(mat) {}

	T operator()(T from) const;

	// Prevent it's shadowing by the above one.
	using HomographicTransformBase<1, T>::operator();

	/**
	 * Every homographic transform has a hyperplane of discontinuity in its domain.
	 * If a homographic transform was generated from a "reasonable" set of points,
	 * all of those points are going to be on the same side of the discontinuity hyperplane.
	 * For points on the other side, mirrorSide() returns true.
	 */
	bool mirrorSide(T from) const;

	/**
	 * Treating a 1D homographic transform as an ordinary function f: R -> R, computes its
	 * derivative at a given point. Evaluating the derivative at a point of discontinuity
	 * results in undefined behaviour.
	 */
	T derivativeAt(T from) const;

	/**
	 * Treating a 1D homographic transform as an ordinary function f: R -> R, computes its
	 * 2nd derivative at a given point. Evaluating the derivative at a point of discontinuity
	 * results in undefined behaviour.
	 */
	T secondDerivativeAt(T from) const;

	/**
	 * Treating a 1D homographic transform as an ordinary function f: R -> R, finds points
	 * in its domain where the derivative takes the desired value. There may be 0, 1 or 2
	 * such points. Results are reported through @p sink like this:
	 * @code
	 * double const x = ...;
	 * sink(x);
	 * @endcode
	 */
	template<typename F>
	void solveForDeriv(T deriv, F&& sink) const;
};


template<size_t N, typename T>
HomographicTransform<N, T>
HomographicTransformBase<N, T>::inv() const
{
	Eigen::Matrix<T, N+1, N+1> inv_mat;
	bool invertible = false;

	m_mat.computeInverseWithCheck(inv_mat, invertible);
	if (!invertible) {
		throw std::runtime_error("Attempt to invert a non-invertible HomographicTransform");
	}

	return HomographicTransform<N, T>(inv_mat);
}

template<size_t N, typename T>
typename HomographicTransformBase<N, T>::Vec
HomographicTransformBase<N, T>::operator()(Vec const& from) const
{
	Eigen::Matrix<T, N+1, 1> hsrc;
	hsrc << from, T(1);

	Eigen::Matrix<T, N+1, 1> const hdst(m_mat * hsrc);
	return hdst.topLeftCorner(N, 1) / hdst[N];
}

template<typename T>
T
HomographicTransform<1, T>::operator()(T from) const
{
	// Optimized version for 1D case.
	auto const& m = this->mat();
	return (from * m(0, 0) + m(0, 1)) / (from * m(1, 0) + m(1, 1));
}

template<typename T>
bool
HomographicTransform<1, T>::mirrorSide(T from) const
{
	auto const& m = this->mat();
	return std::signbit(from * m(1, 0) + m(1, 1));
}

template<typename T>
T
HomographicTransform<1, T>::derivativeAt(T from) const
{
	auto const& m = this->mat();
	T const den = from * m(1, 0) + m(1, 1);
	return m.determinant() / (den * den);
}

template<typename T>
T
HomographicTransform<1, T>::secondDerivativeAt(T from) const
{
	auto const& m = this->mat();
	T const den = from * m(1, 0) + m(1, 1);
	return T(-2.0) * m(1, 0) * m.determinant() / (den * den * den);
}

template<typename T>
template<typename F>
void
HomographicTransform<1, T>::solveForDeriv(T deriv, F&& sink) const
{
	auto const& m = this->mat();
	T const a = deriv * m(1, 0) * m(1, 0);
	T const b = T(2.0) * deriv * m(1, 0) * m(1, 1);
	T const c = deriv * m(1, 1) * m(1, 1) - m.determinant();
	T const d = b * b - T(4.0) * a * c;

	if (std::abs(a) < std::numeric_limits<T>::epsilon()) {
		// We've got a linear not quadratic function.
		if (std::abs(b) >= std::numeric_limits<T>::epsilon()) {
			sink(c / -b);
		}
	} else if (std::abs(d) < std::numeric_limits<T>::epsilon()) {
		// 1 root.
		sink(-b / (T(2.0) * a));
	} else if (d > T(0.0)) {
		// 2 roots.
		T const sqrt_d = std::sqrt(d);
		sink((-b + sqrt_d) / (T(2.0) * a));
		sink((-b - sqrt_d) / (T(2.0) * a));
	}
}

#endif
