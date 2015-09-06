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

#include "HitMissTransform.h"
#include "BinaryFill.h"
#include "BinaryRasterOp.h"
#include "Utils.h"
#include <QPoint>
#include <QRect>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <utility>

using namespace imageproc;

namespace opencl
{

namespace
{

/**
 * Clip @p fit_and_adjust in order for it to fit into @p fit_into,
 * applying identical clipping to @p adjust_only as well.
 */
void adjustToFit(QRect const& fit_into, QRect& fit_and_adjust, QRect& adjust_only)
{
	int adj_left = fit_into.left() - fit_and_adjust.left();
	if (adj_left < 0) {
		adj_left = 0;
	}

	int adj_right = fit_into.right() - fit_and_adjust.right();
	if (adj_right > 0) {
		adj_right = 0;
	}

	int adj_top = fit_into.top() - fit_and_adjust.top();
	if (adj_top < 0) {
		adj_top = 0;
	}

	int adj_bottom = fit_into.bottom() - fit_and_adjust.bottom();
	if (adj_bottom > 0) {
		adj_bottom = 0;
	}

	fit_and_adjust.adjust(adj_left, adj_top, adj_right, adj_bottom);
	adjust_only.adjust(adj_left, adj_top, adj_right, adj_bottom);
}

} // anonymous namespace

void hitMissMatch(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<uint32_t> const& src, int const src_pixel_width,
	imageproc::BWColor src_surroundings, OpenCLGrid<uint32_t> const& dst,
	std::vector<QPoint> const& hits, std::vector<QPoint> const& misses,
	std::vector<cl::Event> const* dependencies, std::vector<cl::Event>* completion_set)
{
	int const src_pixel_height = src.height();
	QRect const rect(0, 0, src_pixel_width, src_pixel_height);

	std::vector<cl::Event> events;
	if (dependencies) {
		events = *dependencies;
	}

	bool first = true;

	for (QPoint const& hit : hits) {
		QRect src_rect(rect);
		QRect dst_rect(rect.translated(-hit));
		adjustToFit(rect, dst_rect, src_rect);

		if (first) {
			first = false;
			binaryRasterOp(
				command_queue, program, "binary_rop_kernel_src",
				src, src_rect, dst, dst_rect, &events, &events
			);
			if (src_surroundings == BLACK) {
				binaryFillFrame(
					command_queue, program, dst,
					rect, dst_rect, BLACK, &events, &events
				);
			}
		} else {
			binaryRasterOp(
				command_queue, program, "binary_rop_kernel_and",
				src, src_rect, dst, dst_rect, &events, &events
			);
		}

		if (src_surroundings == WHITE) {
			// No hits on white surroundings.
			binaryFillFrame(
				command_queue, program, dst,
				rect, dst_rect, WHITE, &events, &events
			);
		}
	}

	for (QPoint const& miss : misses) {
		QRect src_rect(rect);
		QRect dst_rect(rect.translated(-miss));
		adjustToFit(rect, dst_rect, src_rect);

		if (first) {
			first = false;
			binaryRasterOp(
				command_queue, program, "binary_rop_kernel_not_src",
				src, src_rect, dst, dst_rect, &events, &events
			);
			if (src_surroundings == WHITE) {
				binaryFillFrame(
					command_queue, program, dst,
					rect, dst_rect, BLACK, &events, &events
				);
			}
		} else {
			binaryRasterOp(
				command_queue, program, "binary_rop_kernel_dst_and_not_src",
				src, src_rect, dst, dst_rect, &events, &events
			);
		}

		if (src_surroundings == BLACK) {
			// No misses on black surroundings.
			binaryFillFrame(
				command_queue, program, dst,
				rect, dst_rect, WHITE, &events, &events
			);
		}
	}

	if (first) {
		// No matches.
		binaryFillRect(command_queue, program, dst, rect, WHITE, &events, &events);
	}

	indicateCompletion(completion_set, std::move(events));
}

void hitMissReplaceInPlace(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<uint32_t> const& image, int image_pixel_width,
	imageproc::BWColor image_surroundings, OpenCLGrid<uint32_t> const& tmp,
	char const* pattern, int pattern_width, int pattern_height,
	std::vector<cl::Event> const* dependencies, std::vector<cl::Event>* completion_set)
{
	int const image_pixel_height = image.height();
	QRect const rect(0, 0, image_pixel_width, image_pixel_height);

	// It's better to have the origin at one of the replacement positions.
	// Otherwise we may miss a partially outside-of-image match because
	// the origin point was outside of the image as well.
	int const pattern_len = pattern_width * pattern_height;
	char const* const minus_pos = (char const*)memchr(pattern, '-', pattern_len);
	char const* const plus_pos = (char const*)memchr(pattern, '+', pattern_len);
	char const* origin_pos;
	if (minus_pos && plus_pos) {
		origin_pos = std::min(minus_pos, plus_pos);
	} else if (minus_pos) {
		origin_pos = minus_pos;
	} else if (plus_pos) {
		origin_pos = plus_pos;
	} else {
		// No replacements requested - nothing to do.
		return;
	}

	QPoint const origin(
		(origin_pos - pattern) % pattern_width,
		(origin_pos - pattern) / pattern_width
	);

	std::vector<QPoint> hits;
	std::vector<QPoint> misses;
	std::vector<QPoint> white_to_black;
	std::vector<QPoint> black_to_white;

	char const* p = pattern;
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x, ++p) {
			switch (*p) {
				case '-':
					black_to_white.push_back(QPoint(x, y) - origin);
					// fall through
				case 'X':
					hits.push_back(QPoint(x, y) - origin);
					break;
				case '+':
					white_to_black.push_back(QPoint(x, y) - origin);
					// fall through
				case ' ':
					misses.push_back(QPoint(x, y) - origin);
					break;
				case '?':
					break;
				default:
					throw std::invalid_argument(
						"hitMissReplace: invalid character in pattern"
					);
			}
		}
	}

	std::vector<cl::Event> events;
	if (dependencies) {
		events = *dependencies;
	}

	hitMissMatch(
		command_queue, program, image, image_pixel_width,
		image_surroundings, tmp, hits, misses, &events, &events
	);

	for (QPoint const& offset : white_to_black) {
		QRect src_rect(rect);
		QRect dst_rect(rect.translated(offset));
		adjustToFit(rect, dst_rect, src_rect);

		binaryRasterOp(
			command_queue, program, "binary_rop_kernel_or",
			tmp, src_rect, image, dst_rect, &events, &events
		);
	}

	for (QPoint const& offset : black_to_white) {
		QRect src_rect(rect);
		QRect dst_rect(rect.translated(offset));
		adjustToFit(rect, dst_rect, src_rect);

		binaryRasterOp(
			command_queue, program, "binary_rop_kernel_dst_and_not_src",
			tmp, src_rect, image, dst_rect, &events, &events
		);
	}

	indicateCompletion(completion_set, std::move(events));
}

} // namespace opencl
