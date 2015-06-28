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

#include "GaussBlur.h"
#include "RasterOpGeneric.h"
#include "GrayImage.h"
#include "Constants.h"
#include "Grid.h"
#include <Eigen/Core>
#include <Eigen/LU>
#include <QSize>
#include <QPoint>
#include <QString>
#include <QApplication>
#include <boost/test/auto_unit_test.hpp>
#include <cmath>

using namespace Eigen;

namespace imageproc
{

namespace tests
{

static float calcAnalyticalPeakImpulseResponse(Vector2f const& sigmas)
{
	float const detCov = sigmas[0] * sigmas[0] * sigmas[1] * sigmas[1];
	return 0.5 / (constants::PI * std::sqrt(detCov));

}

static Grid<float> computeAnalyticalImpulseResponse(
	float const scale, QSize const& size, QPoint const& center,
	Vector2f const& direction, Vector2f const& sigmas)
{
	Grid<float> impulse_response(size.width(), size.height());

	Matrix2d R;
	R <<
		direction[0], -direction[1],
		direction[1], direction[0];

	Matrix2d L;
	L <<
		sigmas[0] * sigmas[0], 0.0,
		0.0, sigmas[1] * sigmas[1];

	Matrix2d const cov(R * L * R.transpose()); // Covariance matrix.
	Matrix2d const invCov(cov.inverse());

	// Rotation doesn't affect the determinant.
	double const detCov = sigmas[0] * sigmas[0] * sigmas[1] * sigmas[1];
	double const response_normalizer = scale * 0.5 / (constants::PI * std::sqrt(detCov));

	rasterOpGenericXY(
		[center, invCov, response_normalizer](float& response, int x, int y) {
			// From Wikipedia:
			// response = ((2*pi)^(-k/2))*(det(cov)^-0.5)*exp(-0.5*(x - mu)'*inv(cov)*(x - mu))

			Vector2d const v(x - center.x(), y - center.y());
			double const proximity = v.transpose() * invCov * v;

			response = response_normalizer * std::exp(-0.5 * proximity);
		},
		impulse_response
	);

	return impulse_response;
}

static double calcRMSE(
	Grid<float> const& impulse_response, Grid<float> const& analytical_response)
{
	double const error_normalizer = 1.0 / (impulse_response.width() * impulse_response.height());
	double mse = 0;
	rasterOpGeneric(
		[error_normalizer, &mse](float response, float analytical_response) {
			double const error = response - analytical_response;
			mse += error * error * error_normalizer;
		},
		impulse_response, analytical_response
	);

	return std::sqrt(mse);
}

/** Use for debugging only. */
static void saveImpulseResponse(Grid<float> const& response, QString const& file)
{
	GrayImage image(QSize(response.width(), response.height()));
	rasterOpGeneric(
		[](float const src, uint8_t& dst) {
			dst = static_cast<uint8_t>(qBound<long>(0, std::lroundf(src * 255.f), 255));
		},
		response, image
	);
	image.toQImage().save(file);
}

BOOST_AUTO_TEST_SUITE(GaussBlurTestSuite);

BOOST_AUTO_TEST_CASE(test_aligned_gaussian)
{
	QSize const size(101, 101);
	QPoint const center(size.width() / 2, size.height() / 2);
	Vector2f const sigmas(15.f, 7.f);
	float const scale = 1.f / calcAnalyticalPeakImpulseResponse(sigmas);

	Grid<float> impulse(size.width(), size.height(), /*padding=*/0);
	impulse.initInterior(0.f);
	impulse(center.x(), center.y()) = scale;

	Grid<float> impulse_response(size.width(), size.height(), /*padding=*/0);

	gaussBlurGeneric(
		size, sigmas[0], sigmas[1],
		impulse.data(), impulse.stride(), [](float val) { return val; },
		impulse_response.data(), impulse_response.stride(),
		[](float& dst, float src) { dst = src; }
	);

	Grid<float> const analytical_response(
		computeAnalyticalImpulseResponse(scale, size, center, Vector2f(1.f, 0.f), sigmas)
	);

	//saveImpulseResponse(impulse_response, "/path/to/image.png");
	//saveImpulseResponse(analytical_response, "/path/to/another-image.png");

	double rmse = calcRMSE(impulse_response, analytical_response);
	BOOST_CHECK_LT(rmse, 0.01);
}

BOOST_AUTO_TEST_CASE(test_oriented_gaussians)
{
	QSize const size(101, 101);
	QPoint const center(size.width() / 2, size.height() / 2);
	Grid<float> impulse(size.width(), size.height(), /*padding=*/0);
	impulse.initInterior(0.f);
	Grid<float> impulse_response(size.width(), size.height(), /*padding=*/0);

	std::vector<Vector2f> const sigmas{
		{ 10.f, 0.5f },
		{ 20.f, 0.5f },
		{ 10.f, 1.f },
		{ 20.f, 1.f },
		{ 10.f, 2.f },
		{ 20.f, 2.f }
	};

	std::vector<Vector2f> directions;
	for (int angle_deg = 0; angle_deg < 360; angle_deg += 6) {
		float const angle_rad = angle_deg * constants::DEG2RAD;
		directions.emplace_back(std::cos(angle_rad), std::sin(angle_rad));
	}

	double const mean_rmse_normalizer = 1.0 / (sigmas.size() * directions.size());
	double mean_rmse = 0;
	double worst_rmse = 0;

	for (Vector2f const& s : sigmas) {
		for (Vector2f const& direction : directions) {
			float const scale = 1.f / calcAnalyticalPeakImpulseResponse(s);
			impulse(center.x(), center.y()) = scale;

			anisotropicGaussBlurGeneric(
				size, direction[0], direction[1], s[0], s[1],
				impulse.data(), impulse.stride(), [](float val) { return val; },
				impulse_response.data(), impulse_response.stride(),
				[](float& dst, float src) { dst = src; }
			);

			Grid<float> const analytical_response(
				computeAnalyticalImpulseResponse(scale, size, center, direction, s)
			);

			double const rmse = calcRMSE(impulse_response, analytical_response);
			mean_rmse += rmse * mean_rmse_normalizer;
			if (rmse > worst_rmse) {
				worst_rmse = rmse;
				//saveImpulseResponse(impulse_response, "/path/to/image.png");
				//saveImpulseResponse(analytical_response, "/path/to/another-image.png");
			}

			BOOST_CHECK_LT(rmse, 0.02);
		}
	}

	BOOST_MESSAGE("Anisotropic Gauss blur RMSE: mean = " << mean_rmse << ", worst = " << worst_rmse);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
