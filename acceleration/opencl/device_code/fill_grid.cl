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

kernel void fill_byte_grid(
	global uchar* grid, int const grid_offset, int const grid_stride,
	uchar const value)
{
	int const x = get_global_id(0);
	int const y = get_global_id(1);

	grid[grid_offset + x + y * grid_stride] = value;
}

kernel void fill_float_grid(
	global float* grid, int const grid_offset, int const grid_stride,
	float const value)
{
	int const x = get_global_id(0);
	int const y = get_global_id(1);

	grid[grid_offset + x + y * grid_stride] = value;
}
