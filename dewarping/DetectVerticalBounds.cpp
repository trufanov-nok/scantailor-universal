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

#include "DetectVerticalBounds.h"
#include "NumericTraits.h"
#include "ToLineProjector.h"
#include "LineIntersectionScalar.h"
#include "imageproc/Constants.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <cmath>
#include <utility>
#include <cassert>

using namespace imageproc;

namespace dewarping
{

namespace
{

enum LeftRightSide
{
	LEFT_SIDE,
	RIGHT_SIDE
};

QLineF findVerticalBound(std::vector<QLineF> const& chords,
	double const dist_threshold, LeftRightSide const side)
{
	if (chords.size() < 2) {
		return QLineF();
	}

	// Lines between 90 - N and 90 + N degrees will be considered.
	double const max_degrees_from_vertical = 15.0;
	double const min_angle_tan = std::tan((90.0 - max_degrees_from_vertical)*constants::DEG2RAD);
	assert(min_angle_tan > 0);

	int const target_idx = side == LEFT_SIDE ? 0 : 1;
	int const opposite_idx = side == LEFT_SIDE ? 1 : 0;
	QLineF best_vert_line;
	double best_score = NumericTraits<double>::min();

	// Repeatablity is important, so don't seed the RNG.
	boost::random::mt19937 rng;
	boost::random::uniform_int_distribution<> rng_dist(0, chords.size() - 1);
	for (int ransac_iteration = 0; ransac_iteration < 2000; ++ransac_iteration) {
		int const i1 = rng_dist(rng);
		int const i2 = rng_dist(rng);
		if (i1 == i2) {
			continue;
		}

		QPointF const pts1[] = { chords[i1].p1(), chords[i1].p2() };
		QPointF const pts2[] = { chords[i2].p1(), chords[i2].p2() };
		QLineF const candidate_line(pts1[target_idx], pts2[target_idx]);

		// Check if the angle is within range.
		QPointF const vec(candidate_line.p2() - candidate_line.p1());
		if (std::abs(vec.y()) * min_angle_tan < std::abs(vec.x())) {
			continue;
		}

		ToLineProjector const proj(candidate_line);

		double score = 0.0;

		int num_support_points = 0;
		for (QLineF const& chord : chords) {
			QPointF const pts[] = { chord.p1(), chord.p2() };
			QPointF const pt(pts[target_idx]);
			double const dist = proj.projectionDist(pt);
			if (dist < dist_threshold) {
				double const relative_dist = dist / dist_threshold;
				double const decay = std::exp(-relative_dist*relative_dist);
				score += decay * chord.length() * chord.length();
				++num_support_points;
			}

			double s = 0.0;
			if (lineIntersectionScalar(QLineF(pts[opposite_idx], pts[target_idx]), candidate_line, s)) {
				if (s < 1.0) {
					// Some part of that line is further to the target side.
					double const frac = std::min(1.0, 1.0 - s);
					score -= frac * frac * chord.length() * chord.length();
				}
			}
		}

		if (num_support_points > 3 && score > best_score) {
			best_vert_line = candidate_line;
			best_score = score;
		}
	}

	return best_vert_line;
}

} // anonymous namespace

std::pair<QLineF, QLineF> detectVerticalBounds(
	std::list<std::vector<QPointF>> const& curves, double const dist_threshold)
{
	std::vector<QLineF> chords;
	chords.reserve(curves.size());

	for (auto const& polyline : curves) {
		if (polyline.size() > 1) {
			chords.push_back(QLineF(polyline.front(), polyline.back()));
		}
	}

	QLineF const left_line(findVerticalBound(chords, dist_threshold, LEFT_SIDE));
	QLineF const right_line(findVerticalBound(chords, dist_threshold, RIGHT_SIDE));

	return std::make_pair(left_line, right_line);
}

} // namespace dewarping
