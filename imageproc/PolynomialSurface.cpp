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
    along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "PolynomialSurface.h"
#include "AlignedArray.h"
#include "BinaryImage.h"
#include "GrayImage.h"
#include "Grayscale.h"
#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <stdexcept>
#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <assert.h>

using namespace Eigen;

namespace imageproc
{

PolynomialSurface::PolynomialSurface(
	int const hor_degree, int const vert_degree, GrayImage const& src)
:	m_horDegree(hor_degree),
	m_vertDegree(vert_degree)
{
	// Note: m_horDegree and m_vertDegree may still change!
	
	if (hor_degree < 0) {
		throw std::invalid_argument("PolynomialSurface: horizontal degree is invalid");
	}
	if (vert_degree < 0) {
		throw std::invalid_argument("PolynomialSurface: vertical degree is invalid");
	}
	
	int const num_data_points = src.width() * src.height();
	if (num_data_points == 0) {
		m_horDegree = 0;
		m_vertDegree = 0;
		m_coeffs.setZero(1);
		return;
	}
	
	maybeReduceDegrees(num_data_points);
	
	int const num_terms = calcNumTerms();

	// The least squares equation is A'*A*x = A'*b
	// We will be building A'*A and A'*b incrementally.
	// This allows us not to build matrix A at all.
	MatrixXd AtA(num_terms, num_terms);
	AtA.setZero();
	VectorXd Atb(num_terms);
	Atb.setZero();
	prepareDataForLeastSquares(src, AtA, Atb, m_horDegree, m_vertDegree);

	fixSquareMatrixRankDeficiency(AtA);
	m_coeffs = AtA.selfadjointView<Upper>().ldlt().solve(Atb);
}

PolynomialSurface::PolynomialSurface(
	int const hor_degree, int const vert_degree,
	GrayImage const& src, BinaryImage const& mask)
:	m_horDegree(hor_degree),
	m_vertDegree(vert_degree)
{
	// Note: m_horDegree and m_vertDegree may still change!
	
	if (hor_degree < 0) {
		throw std::invalid_argument("PolynomialSurface: horizontal degree is invalid");
	}
	if (vert_degree < 0) {
		throw std::invalid_argument("PolynomialSurface: vertical degree is invalid");
	}
	if (src.size() != mask.size()) {
		throw std::invalid_argument("PolynomialSurface: image and mask have different sizes");
	}
	
	int const num_data_points = mask.countBlackPixels();
	if (num_data_points == 0) {
		m_horDegree = 0;
		m_vertDegree = 0;
		m_coeffs.setZero(1);
		return;
	}
	
	maybeReduceDegrees(num_data_points);
	
	int const num_terms = calcNumTerms();

	// The least squares equation is A'*A*x = A'*b
	// We will be building A'*A and A'*b incrementally.
	// This allows us not to build matrix A at all.
	MatrixXd AtA(num_terms, num_terms);
	AtA.setZero();
	VectorXd Atb(num_terms);
	Atb.setZero();
	prepareDataForLeastSquares(src, AtA, Atb, m_horDegree, m_vertDegree);

	fixSquareMatrixRankDeficiency(AtA);
	m_coeffs = AtA.selfadjointView<Upper>().ldlt().solve(Atb);
}

GrayImage
PolynomialSurface::render(QSize const& size) const
{
	if (size.isEmpty()) {
		return GrayImage();
	}
	
	GrayImage image(size);
	int const width = size.width();
	int const height = size.height();
	unsigned char* line = image.data();
	int const bpl = image.stride();
	int const num_coeffs = m_coeffs.size();
	
	// Pretend that both x and y positions of pixels
	// lie in range of [0, 1].
	double const xscale = calcScale(width);
	double const yscale = calcScale(height);
	
	AlignedArray<float, 4> vert_matrix(num_coeffs * height);
	float* out = &vert_matrix[0];
	for (int y = 0; y < height; ++y) {
		double const y_adjusted = y * yscale;
		double pow = 1.0;
		int pos = 0;
		for (int i = 0; i <= m_vertDegree; ++i) {
			for (int j = 0; j <= m_horDegree; ++j, ++pos, ++out) {
				*out = static_cast<float>(m_coeffs[pos] * pow);
			}
			pow *= y_adjusted;
		}
	}
	
	AlignedArray<float, 4> hor_matrix(num_coeffs * width);
	out = &hor_matrix[0];
	for (int x = 0; x < width; ++x) {
		double const x_adjusted = x * xscale;
		for (int i = 0; i <= m_vertDegree; ++i) {
			double pow = 1.0;
			for (int j = 0; j <= m_horDegree; ++j, ++out) {
				*out = static_cast<float>(pow);
				pow *= x_adjusted;
			}
		}
	}
	
	float const* vert_line = &vert_matrix[0];
	for (int y = 0; y < height; ++y, line += bpl, vert_line += num_coeffs) {
		float const* hor_line = &hor_matrix[0];
		for (int x = 0; x < width; ++x, hor_line += num_coeffs) {
			float sum = 0.5f / 255.0f; // for rounding purposes.
			for (int i = 0; i < num_coeffs; ++i) {
				sum += hor_line[i] * vert_line[i];
			}
			int const isum = (int)(sum * 255.0);
			line[x] = static_cast<unsigned char>(qBound(0, isum, 255));
		}
	}
	
	return image;
}

void
PolynomialSurface::maybeReduceDegrees(int const num_data_points)
{
	assert(num_data_points > 0);
	
	while (num_data_points < calcNumTerms()) {
		if (m_horDegree > m_vertDegree) {
			--m_horDegree;
		} else {
			--m_vertDegree;
		}
	}
}

int
PolynomialSurface::calcNumTerms() const
{
	return (m_horDegree + 1) * (m_vertDegree + 1);
}

double
PolynomialSurface::calcScale(int const dimension)
{
	if (dimension <= 1) {
		return 0.0;
	} else {
		return 1.0 / (dimension - 1);
	}
}

void PolynomialSurface::prepareDataForLeastSquares(
	GrayImage const& image, MatrixXd& AtA, VectorXd& Atb,
	int const h_degree, int const v_degree)
{
	int const width = image.width();
	int const height = image.height();
	int const num_terms = Atb.size();

	uint8_t const* line = image.data();
	int const stride = image.stride();

	// Pretend that both x and y positions of pixels
	// lie in range of [0, 1].
	double const xscale = calcScale(width);
	double const yscale = calcScale(height);

	// To force data samples into [0, 1] range.
	double const data_scale = 1.0 / 255.0;

	// 1, y, y^2, y^3, ...
	VectorXd y_powers(v_degree + 1); // Uninitialized so far.

	// Same as y_powers, except y_powers correspond to a given y,
	// while x_powers are computed for all possible x values.
	MatrixXd x_powers(h_degree + 1, width); // Uninitialized so far.
	for (int x = 0; x < width; ++x) {
		double const x_adjusted = xscale * x;
		double x_power = 1.0;
		for (int i = 0; i <= h_degree; ++i) {
			x_powers(i, x) = x_power;
			x_power *= x_adjusted;
		}
	}

	VectorXd full_powers(num_terms); // Uninitialized so far.

	for (int y = 0; y < height; ++y, line += stride) {
		double const y_adjusted = yscale * y;

		double y_power = 1.0;
		for (int i = 0; i <= v_degree; ++i) {
			y_powers[i] = y_power;
			y_power *= y_adjusted;
		}

		for (int x = 0; x < width; ++x) {
			double const data_point = data_scale * line[x];

			int pos = 0;
			for (int i = 0; i <= v_degree; ++i) {
				for (int j = 0; j <= h_degree; ++j, ++pos) {
					full_powers[pos] = y_powers[i] * x_powers(j, x);
				}
			}

			for (int i = 0; i < num_terms; ++i) {
				double const i_val = full_powers[i];
				Atb[i] += i_val * data_point;

				// Only updating the upper triangular part as this matrix
				// is symmetric and we'll be using selfadjointView() on it.
				for (int j = i; j < num_terms; ++j) {
					double const j_val = full_powers[j];
					AtA(i, j) += i_val * j_val;
				}
			}
		}
	}
}

void PolynomialSurface::prepareDataForLeastSquares(
	GrayImage const& image, BinaryImage const& mask,
	MatrixXd& AtA, VectorXd& Atb,
	int const h_degree, int const v_degree)
{
	int const width = image.width();
	int const height = image.height();
	int const num_terms = Atb.size();

	uint8_t const* image_line = image.data();
	int const image_stride = image.stride();

	uint32_t const* mask_line = mask.data();
	int const mask_stride = mask.wordsPerLine();

	// Pretend that both x and y positions of pixels
	// lie in range of [0, 1].
	double const xscale = calcScale(width);
	double const yscale = calcScale(height);

	// To force data samples into [0, 1] range.
	double const data_scale = 1.0 / 255.0;

	// 1, y, y^2, y^3, ...
	VectorXd y_powers(v_degree + 1); // Uninitialized so far.

	// Same as y_powers, except y_powers correspond to a given y,
	// while x_powers are computed for all possible x values.
	MatrixXd x_powers(h_degree + 1, width); // Uninitialized so far.
	for (int x = 0; x < width; ++x) {
		double const x_adjusted = xscale * x;
		double x_power = 1.0;
		for (int i = 0; i <= h_degree; ++i) {
			x_powers(i, x) = x_power;
			x_power *= x_adjusted;
		}
	}

	VectorXd full_powers(num_terms); // Uninitialized so far.

	uint32_t const msb = uint32_t(1) << 31;
	for (int y = 0; y < height; ++y) {
		double const y_adjusted = yscale * y;

		double y_power = 1.0;
		for (int i = 0; i <= v_degree; ++i) {
			y_powers[i] = y_power;
			y_power *= y_adjusted;
		}

		for (int x = 0; x < width; ++x) {
			if (!(mask_line[x >> 5] & (msb >> (x & 31)))) {
				continue;
			}

			double const data_point = data_scale * image_line[x];

			int pos = 0;
			for (int i = 0; i <= v_degree; ++i) {
				for (int j = 0; j <= h_degree; ++j, ++pos) {
					full_powers[pos] = y_powers[i] * x_powers(j, x);
				}
			}

			for (int i = 0; i < num_terms; ++i) {
				double const i_val = full_powers[i];
				Atb[i] += i_val * data_point;

				// Only updating the upper triangular part as this matrix
				// is symmetric and we'll be using selfadjointView() on it.
				for (int j = 0; j < num_terms; ++j) {
					double const j_val = full_powers[j];
					AtA(i, j) += i_val * j_val;
				}
			}
		}

		image_line += image_stride;
		mask_line += mask_stride;
	}
}

void
PolynomialSurface::fixSquareMatrixRankDeficiency(MatrixXd& mat)
{
	assert(mat.cols() == mat.rows());

	int const dim = mat.cols();
	for (int i = 0; i < dim; ++i) {
		mat(i, i) += 1e-5; // Add a small value to the diagonal.
	}
}

} // namespace imagproc
