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

#include "WatershedSegmentation.h"
#include "GrayImage.h"
#include "ColorForId.h"
#include "FastQueue.h"
#include <QImage>
#include <QPoint>
#include <QRect>
#include <algorithm>
#include <vector>
#include <assert.h>

class QImage;

namespace imageproc
{

class GrayImage;

WatershedSegmentation::WatershedSegmentation()
:	m_maxLabel(0)
{
}

WatershedSegmentation::WatershedSegmentation(
	GrayImage const& image, Connectivity const conn)
:	m_grid(image.width(), image.height(), /*padding=*/0)
,	m_maxLabel(0)
{
	if (image.isNull()) {
		return;
	}

	m_grid.initInterior(0);

	int const width = m_grid.width();
	int const height = m_grid.height();

	uint8_t const* const image_data = image.data();
	uint8_t const* image_line = image_data;
	int const image_stride = image.stride();

	uint32_t* grid_line = m_grid.data();
	int const grid_stride = m_grid.stride();

	QPoint const offsets4[] = {
		QPoint(0, -1),
		QPoint(-1, 0), QPoint(1, 0),
		QPoint(0, 1)
	};

	QPoint const offsets8[] = {
		QPoint(-1, -1), QPoint(0, -1), QPoint(1, -1),
		QPoint(-1, 0),                 QPoint(1, 0),
		QPoint(-1, 1),  QPoint(0, 1),  QPoint(1, 1)
	};

	QPoint const* offsets = conn == CONN4 ? offsets4 : offsets8;
	int const num_neighbours = conn == CONN4 ? 4 : 8;

	QRect const rect(image.rect());

	std::vector<uint32_t> remapping_table(1, 0); // Reserve space for label 0.
	FastQueue<QPoint> queue;
	uint32_t this_label = 0;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (!grid_line[x]) {
				++this_label;

				uint8_t last_altitude = image_line[x];
				uint8_t next_altitude = last_altitude;
				QPoint next_pos;

				assert(queue.empty());
				queue.push(QPoint(x, y));
				grid_line[x] = this_label;

				for (;;) {
					while (!queue.empty()) {
						QPoint const pos(queue.front());
						queue.pop();

						for (int i = 0; i < num_neighbours; ++i) {
							QPoint const new_pos(pos + offsets[i]);
							if (!rect.contains(new_pos)) {
								continue;
							}

							uint8_t const px = image_data[image_stride * new_pos.y() + new_pos.x()];

							if (px > last_altitude) {
								continue;
							} else if (px == last_altitude) {
								uint32_t& label = m_grid(new_pos.x(), new_pos.y());
								if (!label) {
									queue.push(new_pos);
									m_grid(new_pos.x(), new_pos.y()) = this_label;
								} else {
									assert(label == this_label);
								}
							} else {
								assert(px < last_altitude);
								next_altitude = px;
								next_pos = new_pos;
							}
						}
					}

					if (next_altitude >= last_altitude) {
						// Reached the local minima without merging with another stream.
						assert(next_altitude == last_altitude);
						remapping_table.push_back(this_label);
						break;
					}

					uint32_t& next_label = m_grid(next_pos.x(), next_pos.y());
					if (next_label) {
						// Merging with another stream.
						remapping_table.push_back(remapping_table[next_label]);
						break;
					}

					// Continuing downstream.
					assert(queue.empty());
					queue.push(next_pos);
					next_label = this_label;
					last_altitude = next_altitude;
				}
			}
		}

		image_line += image_stride;
		grid_line += grid_stride;
	}

	uint32_t last_label = 0;
	uint32_t const remapping_table_size = remapping_table.size();
	std::vector<uint32_t> remapping_table2(remapping_table_size, 0);
	for (uint32_t src_label = 1; src_label < remapping_table_size; ++src_label) {
		uint32_t const dst_label = remapping_table[src_label];
		if (dst_label == src_label) {
			remapping_table2[dst_label] = ++m_maxLabel;
		}
	}

	grid_line = m_grid.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t& label = grid_line[x];
			label = remapping_table2[remapping_table[label]];
			assert(label);
		}
		grid_line += grid_stride;
	}
}

void
WatershedSegmentation::swap(WatershedSegmentation& other)
{
	m_grid.swap(other.m_grid);
	std::swap(m_maxLabel, other.m_maxLabel);
}

QImage
WatershedSegmentation::visualized() const
{
	if (m_grid.isNull()) {
		return QImage();
	}

	int const width = m_grid.width();
	int const height = m_grid.height();

	QImage dst(width, height, QImage::Format_RGB32);

	uint32_t const* src_line = m_grid.data();
	int const src_stride = m_grid.stride();

	uint32_t* dst_line = reinterpret_cast<uint32_t*>(dst.bits());
	int const dst_stride = dst.bytesPerLine() / sizeof(uint32_t);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			dst_line[x] = colorForId(src_line[x]).rgba();
		}
		src_line += src_stride;
		dst_line += dst_stride;
	}

	return dst;
}

} // namespace imageproc
