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

#include "SavGolKernel.h"
#include <boost/test/auto_unit_test.hpp>
#include <QSize>
#include <QPoint>

namespace imageproc
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(SavGolKernelTestSuite);

BOOST_AUTO_TEST_CASE(TestKnownCoefficients)
{
	// 5x1 window, 3x0 degree.
	SavGolKernel const kernel(QSize(5, 1), QPoint(2, 0), 3, 0);

	// Coefficients taken from Wikipedia.
	static double const control_coeffs[] = {-3.0/35, 12.0/35, 17.0/35, 12.0/35, -3.0/35};

	for (int i = 0; i < 5; ++i) {
		BOOST_CHECK_CLOSE(kernel[i], control_coeffs[i], 1e-5);
	}
}

BOOST_AUTO_TEST_CASE(TestSymmetricKernelIsSeparable)
{
	// 5x1 window, 3x0 degree.
	SavGolKernel const kernel_1d(QSize(5, 1), QPoint(2, 0), 3, 0);

	// 5x5 window, 3x3 degree.
	SavGolKernel const kernel_2d(QSize(5, 5), QPoint(2, 2), 3, 3);

	for (int y = 0; y < 5; ++y) {
		for (int x = 0; x < 5; ++x) {
			BOOST_CHECK_CLOSE(kernel_2d[y*5+x], kernel_1d[x] * kernel_1d[y], 1e-4);
		}
	}
}

BOOST_AUTO_TEST_CASE(TestAssymmetricKernelIsSeparable)
{
	// 5x1 window, 3x0 degree.
	SavGolKernel const h_kernel(QSize(5, 1), QPoint(2, 0), 3, 0);

	// 1x6 window, 0x4 degree.
	SavGolKernel const v_kernel(QSize(1, 6), QPoint(0, 4), 0, 4);

	// 5x6 window, 3x4 degree.
	SavGolKernel const kernel_2d(QSize(5, 6), QPoint(2, 4), 3, 4);

	for (int y = 0; y < 6; ++y) {
		for (int x = 0; x < 5; ++x) {
			BOOST_CHECK_CLOSE(kernel_2d[y*5+x], h_kernel[x] * v_kernel[y], 1e-4);
		}
	}
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
