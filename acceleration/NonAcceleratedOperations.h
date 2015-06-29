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

#ifndef NON_ACCELERATED_OPERATIONS_H_
#define NON_ACCELERATED_OPERATIONS_H_

#include "AcceleratableOperations.h"
#include "NonCopyable.h"
#include "Grid.h"
#include "VecNT.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <utility>

class NonAcceleratedOperations : public AcceleratableOperations
{
	DECLARE_NON_COPYABLE(NonAcceleratedOperations)
public:
	NonAcceleratedOperations() = default;

	virtual Grid<float> anisotropicGaussBlur(
		Grid<float> const& src, float dir_x, float dir_y,
		float dir_sigma, float ortho_dir_sigma) const;

	virtual std::pair<Grid<float>, Grid<uint8_t>> textFilterBank(
		Grid<float> const& src, std::vector<Vec2f> const& directions,
		std::vector<Vec2f> const& sigmas, float shoulder_length) const;
};

#endif
