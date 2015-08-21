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

#include "SavGolKernel.h"
#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <QSize>
#include <QPoint>
#include <stdexcept>

using namespace Eigen;

namespace imageproc
{

namespace
{

int calcNumTerms(int const hor_degree, int const vert_degree)
{
	return (hor_degree + 1) * (vert_degree + 1);
}

} // anonymous namespace


SavGolKernel::SavGolKernel(
	QSize const& size, QPoint const& origin,
	int const hor_degree, int const vert_degree)
:	m_horDegree(hor_degree),
	m_vertDegree(vert_degree),
	m_width(size.width()),
	m_height(size.height()),
	m_numTerms(calcNumTerms(hor_degree, vert_degree))
{
	if (size.isEmpty()) {
		throw std::invalid_argument("SavGolKernel: invalid size");
	}
	if (hor_degree < 0) {
		throw std::invalid_argument("SavGolKernel: invalid hor_degree");
	}
	if (vert_degree < 0) {
		throw std::invalid_argument("SavGolKernel: invalid vert_degree");
	}
	if (m_numTerms > m_width * m_height) {
		throw std::invalid_argument("SavGolKernel: too high degree for this amount of data");
	}

	VectorXd sample(m_numTerms);
	MatrixXd AtA;
	AtA.setZero(m_numTerms, m_numTerms);

	for (int y = 1; y <= m_height; ++y) {
		for (int x = 1; x <= m_width; ++x) {
			fillSample(sample.data(), x, y, m_horDegree, m_vertDegree);

			for (int i = 0; i < m_numTerms; ++i) {
				for (int j = 0; j <= i; ++j) {
					AtA(i, j) += sample[i] * sample[j];
				}
			}
		}
	}

	m_leastSquaresDecomp.compute(AtA);
	AlignedArray<float, 4>(m_width * m_height).swap(m_kernel);

	recalcForOrigin(origin);
}

void
SavGolKernel::recalcForOrigin(QPoint const& origin)
{
	VectorXd AtB(m_numTerms);
	fillSample(AtB.data(), origin.x() + 1, origin.y() + 1, m_horDegree, m_vertDegree);

	m_leastSquaresDecomp.solveInPlace(AtB);

	VectorXd sample(m_numTerms);
	float* p_kernel = m_kernel.data();

	for (int y = 1; y <= m_height; ++y) {
		for (int x = 1; x <= m_width; ++x) {
			fillSample(sample.data(), x, y, m_horDegree, m_vertDegree);
			*p_kernel = static_cast<float>(AtB.dot(sample));
			++p_kernel;
		}
	}
}

/**
 * Prepare a single row for matrix A to be used to solve Ax = b, except we never
 * build matrix A but rather incrementally update matrix A'A and vector A'b.
 *
 * @note x1 and y1 stand for 1-based (as opposed to zero-based) coordinates in the kernel window.
 */
void
SavGolKernel::fillSample(double* sampleData, double x1, double y1, int hor_degree, int vert_degree)
{
	double pow1 = 1.0;
	for (int i = 0; i <= vert_degree; ++i, pow1 *= y1) {
		double pow2 = pow1;
		for (int j = 0; j <= hor_degree; ++j, pow2 *= x1) {
			*sampleData = pow2;
			++sampleData;
		}
	}
}

} // namespace imageproc
