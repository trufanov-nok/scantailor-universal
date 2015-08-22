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

#include "SavGolFilter.h"
#include "SavGolKernel.h"
#include "GrayImage.h"
#include "AlignedArray.h"
#include <QImage>
#include <QSize>
#include <QPoint>
#include <QtGlobal>
#include <stdexcept>
#include <algorithm>
#include <cstdint>

namespace imageproc
{

namespace
{

int calcNumTerms(int const hor_degree, int const vert_degree)
{
	return (hor_degree + 1) * (vert_degree + 1);
}

} // anonymous namespace

GrayImage savGolFilter(
	GrayImage const& src, QSize const& window_size,
	int const hor_degree, int const vert_degree)
{
	if (hor_degree < 0 || vert_degree < 0) {
		throw std::invalid_argument("savGolFilter: invalid polynomial degree");
	}
	if (window_size.isEmpty()) {
		throw std::invalid_argument("savGolFilter: invalid window size");
	}
	
	if (calcNumTerms(hor_degree, vert_degree)
			> window_size.width() * window_size.height()) {
		throw std::invalid_argument(
			"savGolFilter: order is too big for that window");
	}
	
	int const width = src.width();
	int const height = src.height();

	// Kernel width and height.
	int const kw = window_size.width();
	int const kh = window_size.height();

	if (kw > width || kh > height) {
		return src;
	}

	/*
	 * Consider a 5x5 kernel:
	 * |x|x|T|x|x|
	 * |x|x|T|x|x|
	 * |L|L|C|R|R|
	 * |x|x|B|x|x|
	 * |x|x|B|x|x|
	 */

	// Co-ordinates of the central point (C) of the kernel.
	QPoint const k_center(kw / 2, kh / 2);

	// Length of the top segment (T) of the kernel.
	int const k_top = k_center.y();

	// Length of the bottom segment (B) of the kernel.
	int const k_bottom = kh - k_top - 1;

	// Length of the left segment (L) of the kernel.
	int const k_left = k_center.x();

	// Length of the right segment (R) of the kernel.
	int const k_right = kw - k_left - 1;

	uint8_t const* const src_data = src.data();
	int const src_stride = src.stride();

	GrayImage dst(QSize(width, height));
	uint8_t* const dst_data = dst.data();
	int const dst_stride = dst.stride();

	// Take advantage of Savitzky-Golay filter being separable.
	SavGolKernel const hor_kernel(
		QSize(window_size.width(), 1),
		QPoint(k_center.x(), 0), hor_degree, 0
	);
	SavGolKernel const vert_kernel(
		QSize(1, window_size.height()),
		QPoint(0, k_center.y()), 0, vert_degree
	);

	// Allocate a 16-byte aligned temporary storage.
	// That may help the compiler to emit efficient SSE code.
	int const temp_stride = (width + 3) & ~3;
	AlignedArray<float, 4> temp_array(temp_stride * (height + kh - 1));
	AlignedArray<float, 4> line_buffer(width + kw - 1);

	// Horizontal pass.
	uint8_t const* src_line = src_data;
	float* temp_line = temp_array.data() + k_top * temp_stride;
	for (int y = 0; y < height; ++y) {
		// Fill the line buffer.
		for (int x = 0; x < width; ++x) {
			line_buffer[x + k_left] = static_cast<float>(src_line[x]);
		}

		// Mirror the edge pixels.
		std::reverse_copy(
			line_buffer.data() + k_left + 1,
			line_buffer.data() + k_left + 1 + k_left,
			line_buffer.data()
		);
		std::reverse_copy(
			line_buffer.data() + k_left + width - 1 - k_right,
			line_buffer.data() + k_left + width - 1,
			line_buffer.data() + k_left + width
		);

		for (int x = 0; x < width; ++x) {
			float sum = 0.0f;
			float const* src = line_buffer.data() + x;
			for (int i = 0; i < kw; ++i) {
				sum += src[i] * hor_kernel[i];
			}
			temp_line[x] = sum;
		}

		temp_line += temp_stride;
		src_line += src_stride;
	}

	// Mirror the top and bottom areas in temp_array.
	for (int x = 0; x < width; ++x) {
		float* temp_src = temp_array.data() + x + (k_top + 1) * temp_stride;
		float* temp_dst = temp_src - temp_stride * 2;
		for (int i = 0; i < k_top; ++i) {
			*temp_dst = *temp_src;
			temp_dst -= temp_stride;
			temp_src += temp_stride;
		}

		temp_dst = temp_array.data() + x + (k_top + height) * temp_stride;
		temp_src = temp_dst - temp_stride * 2;
		for (int i = 0; i < k_bottom; ++i) {
			*temp_dst = *temp_src;
			temp_dst += temp_stride;
			temp_src -= temp_stride;
		}
	}

	// Vertical pass.
	for (int x = 0; x < width; ++x) {
		float const* p_tmp = temp_array.data() + x;
		uint8_t* p_dst = dst_data + x;

		for (int y = 0; y < height; ++y) {
			float const* p_tmp1 = p_tmp;
			float sum = 0.5f; // For rounding purposes.

			for (int i = 0; i < kh; ++i) {
				sum += *p_tmp1 * vert_kernel[i];
				p_tmp1 += temp_stride;
			}

			int const val = static_cast<int>(sum);
			*p_dst = static_cast<uint8_t>(qBound(0, val, 255));

			p_dst += dst_stride;
			p_tmp += temp_stride;
		}
	}

	return dst;
}

} // namespace imageproc
