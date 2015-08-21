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

#ifndef IMAGEPROC_SAVGOL_KERNEL_H_
#define IMAGEPROC_SAVGOL_KERNEL_H_

#include "imageproc_config.h"
#include "AlignedArray.h"
#include <Eigen/Core>
#include <Eigen/Cholesky>

class QPoint;
class QSize;

namespace imageproc
{

class IMAGEPROC_EXPORT SavGolKernel
{
public:
	SavGolKernel(
		QSize const& size, QPoint const& origin,
		int hor_degree, int vert_degree);
	
	void recalcForOrigin(QPoint const& origin);
	
	int width() const { return m_width; }
	
	int height() const { return m_height; }
	
	float const* data() const { return m_kernel.data(); }
	
	float operator[](size_t idx) const { return m_kernel[idx]; }
private:
	static void fillSample(double* sampleData, double x, double y, int hor_degree, int vert_degree);
	
	Eigen::LLT<Eigen::MatrixXd, Eigen::Lower> m_leastSquaresDecomp;
	
	/**
	 * 16-byte aligned convolution kernel of size m_numDataPoints.
	 */
	AlignedArray<float, 4> m_kernel;
	
	/**
	 * The degree of the polynomial in horizontal direction.
	 */
	int m_horDegree;
	
	/**
	 * The degree of the polynomial in vertical direction.
	 */
	int m_vertDegree;
	
	/**
	 * The width of the convolution kernel.
	 */
	int m_width;
	
	/**
	 * The height of the convolution kernel.
	 */
	int m_height;
	
	/**
	 * The number of terms in the polynomial.
	 */
	int m_numTerms;
};

} // namespace imageproc

#endif
