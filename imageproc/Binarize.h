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

#ifndef IMAGEPROC_BINARIZE_H_
#define IMAGEPROC_BINARIZE_H_

#include "imageproc_config.h"
#include <QSize>

class QImage;

namespace imageproc
{

class BinaryImage;
class GrayImage;

/**
 * \brief Image binarization using Otsu's global thresholding method.
 *
 * \see Help -> About -> References -> [7]
 */
IMAGEPROC_EXPORT BinaryImage binarizeOtsu(QImage const& src);

/**
 * \brief Image binarization using Mokji's global thresholding method.
 *
 * \param src The source image.  May be in any format.
 * \param max_edge_width The maximum gradient length to consider.
 * \param min_edge_magnitude The minimum color difference in a gradient.
 * \return A black and white image.
 *
 * \see Help -> About -> References -> [8]
 */
IMAGEPROC_EXPORT BinaryImage binarizeMokji(
	QImage const& src, unsigned max_edge_width = 3,
	unsigned min_edge_magnitude = 20);

/**
  * \brief Image binarization using Niblack's local thresholding method.
  *
  * \see Help -> About -> References -> [9]
  */
IMAGEPROC_EXPORT BinaryImage binarizeNiblack(GrayImage const& src, QSize window_size);

/**
 * \brief Image binarization using Gatos' local thresholding method.
 *
 * This implementation doesn't include the post-processing steps from
 * the above paper.
 *
 * \see Help -> About -> References -> [10]
 */
IMAGEPROC_EXPORT BinaryImage binarizeGatos(
    GrayImage const& src, QSize window_size, double noise_sigma);

/**
 * \brief Image binarization using Sauvola's local thresholding method.
 *
 * \see Help -> About -> References -> [11]
 */
IMAGEPROC_EXPORT BinaryImage binarizeSauvola(QImage const& src, QSize window_size);

/**
 * \brief Image binarization using Wolf's local thresholding method.
 *
 * \see Help -> About -> References -> [12]
 *
 * \param src The image to binarize.
 * \param window_size The dimensions of a pixel neighborhood to consider.
 * \param lower_bound The minimum possible gray level that can be made white.
 * \param upper_bound The maximum possible gray level that can be made black.
 */
IMAGEPROC_EXPORT BinaryImage binarizeWolf(
	QImage const& src, QSize window_size,
	unsigned char lower_bound = 1, unsigned char upper_bound = 254);

} // namespace imageproc

#endif
