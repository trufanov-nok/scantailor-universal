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

kernel void render_polynomial_surface(
	int const width, int const height,
	global uchar* const dst, int const dst_stride,
	constant float const* const coeffs,
	int const coeffs_cols, int const coeffs_rows)
{
	int const x = get_global_id(0);
	int const y = get_global_id(1);
	bool const outside_bounds = (x >= width) | (y >= height);
	if (outside_bounds) {
		return;
	}

	float const x_norm = (float)x / (float)max(width - 1, 1);
	float const y_norm = (float)y / (float)max(height - 1, 1);

	float sum = 0.f;
	float y_pow = 1.f;
	int coeffs_idx = 0;
	for (int i = 0; i < coeffs_rows; ++i, y_pow *= y_norm) {
		float pow = y_pow;
		for (int j = 0; j < coeffs_cols; ++j, ++coeffs_idx, pow *= x_norm) {
			sum += coeffs[coeffs_idx] * pow;
		}
	}

	dst[mad24(y, dst_stride, x)] = clamp((int)rint(sum * 255.f), 0, 255);
}
