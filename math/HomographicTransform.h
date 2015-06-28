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

#include <Eigen/Core>
#include <Eigen/LU>
#include <cstddef>
#include <stdexcept>
#include <string>

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


/** An optimized, both in terms of API and performance, 1D version. */
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

#endif
