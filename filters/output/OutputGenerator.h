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

#ifndef OUTPUT_OUTPUTGENERATOR_H_
#define OUTPUT_OUTPUTGENERATOR_H_

#include "ColorParams.h"
#include "DespeckleLevel.h"
#include "CachingFactory.h"
#include "Grid.h"
#include "imageproc/AbstractImageTransform.h"
#include <boost/optional.hpp>
#include <QSize>
#include <QRect>
#include <QTransform>
#include <QColor>
#include <QPointF>
#include <QLineF>
#include <QPolygonF>
#include <functional>
#include <memory>
#include <vector>
#include <utility>
#include <stdint.h>

class TaskStatus;
class DebugImages;
class ZoneSet;
class QSize;
class QRectF;
class QImage;

namespace imageproc
{
	class BinaryImage;
	class BinaryThreshold;
	class GrayImage;
}

namespace dewarping
{
	class DistortionModel;
	class CylindricalSurfaceDewarper;
}

namespace output
{

class OutputGenerator
{
public:
	OutputGenerator(
		std::shared_ptr<imageproc::AbstractImageTransform const> const& image_transform,
		QRectF const& content_rect, QRectF const& outer_rect,
		ColorParams const& color_params,
		DespeckleLevel despeckle_level);
	
	/**
	 * \brief Produce the output image.
	 * \param status For asynchronous task cancellation.
	 * \param input The input image plus data produced by previous stages.
	 * \param picture_zones A set of manual picture zones.
	 * \param fill_zones A set of manual fill zones.
	 * \param out_auto_picture_mask If provided, the auto-detected picture mask
	 *        will be written there. It would only happen if automatic picture
	 *        detection actually took place. Otherwise, nothing will be
	 *        written into the provided image. The picture mask image has the same
	 *        geometry as the output image. White areas on the mask indicate pictures.
	 *        The manual zones aren't represented in it.
	 * \param out_speckles_image If provided, the speckles removed from the
	 *        binarized image will be written there. It would only happen
	 *        if despeckling was required and actually took place.
	 *        Otherwise, nothing will be written into the provided image.
	 *        The speckles image has the same geometry as the output image.
	 *        Because despeckling is intentionally the last operation on
	 *        the B/W part of the image, the "pre-despeckle" image may be
	 *        restored from the output and speckles images, allowing despeckling
	 *        to be performed again with different settings, without going
	 *        through the whole output generation process again.
	 * \param dbg An optional sink for debugging images.
	 */
	QImage process(
		TaskStatus const& status,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QImage const& orig_image,
		CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
		ZoneSet const& picture_zones, ZoneSet const& fill_zones,
		imageproc::BinaryImage* out_auto_picture_mask = nullptr,
		imageproc::BinaryImage* out_speckles_image = nullptr,
		DebugImages* dbg = nullptr) const;
	
	QSize outputImageSize() const;

	/**
	 * The rectangle in the transformed space of m_ptrImageTransform
	 * corresponding to the output image.
	 */
	QRect outputImageRect() const;
	
	/**
	 * \brief Returns the content rectangle in output image coordinates.
	 */
	QRect outputContentRect() const;

	/**
	 * @brief Returns a mapper from original image to output image coordinates.
	 */
	std::function<QPointF(QPointF const&)> origToOutputMapper() const;

	/**
	 * @brief Returns a mapper from output to original image coordinates.
	 */
	std::function<QPointF(QPointF const&)> outputToOrigMapper() const;
private:
	static QImage convertToRGBorRGBA(QImage const& src);

	static imageproc::GrayImage normalizeIlluminationGray(
		TaskStatus const& status,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		imageproc::GrayImage const& input_for_estimation,
		imageproc::GrayImage const& input_for_normalisation,
		boost::optional<QPolygonF> const& estimation_region_of_intereset,
		DebugImages* dbg);
	
	static imageproc::GrayImage detectPictures(
		TaskStatus const& status, imageproc::GrayImage const& downscaled,
		DebugImages* dbg);
	
	imageproc::BinaryImage estimateBinarizationMask(
		TaskStatus const& status, imageproc::GrayImage const& gray_source,
		DebugImages* dbg) const;

	void modifyBinarizationMask(
		imageproc::BinaryImage& bw_mask, ZoneSet const& zones,
		std::function<QPointF(QPointF const&)> const& orig_to_output) const;
	
	imageproc::BinaryThreshold adjustThreshold(
		imageproc::BinaryThreshold threshold) const;

	imageproc::BinaryImage binarize(
		QImage const& image, imageproc::BinaryImage const& mask) const;
	
	void maybeDespeckleInPlace(
		imageproc::BinaryImage& image, DespeckleLevel level,
		imageproc::BinaryImage* out_speckles_img,
		TaskStatus const& status, DebugImages* dbg) const;

	static imageproc::GrayImage smoothToGrayscale(
		QImage const& src, std::shared_ptr<AcceleratableOperations> const& accel_ops);
	
	static void morphologicalSmoothInPlace(imageproc::BinaryImage& img,
		std::shared_ptr<AcceleratableOperations> const& accel_ops);
	
	static void generatePatternsForAllDirections(
		std::vector<Grid<char>>& sink, char const* const pattern,
		int const pattern_width, int const pattern_height);
	
	static QSize calcLocalWindowSize();
	
	static unsigned char calcDominantBackgroundGrayLevel(QImage const& img);
	
	static QImage normalizeIllumination(QImage const& gray_input, DebugImages* dbg);

	QImage transformAndNormalizeIllumination(
		QImage const& gray_input, DebugImages* dbg,
		QImage const* morph_background = 0) const;
	
	QImage transformAndNormalizeIllumination2(
		QImage const& gray_input, DebugImages* dbg,
		QImage const* morph_background = 0) const;

	void applyFillZonesInPlace(QImage& img, ZoneSet const& zones,
		std::function<QPointF(QPointF const&)> const& orig_to_output) const;

	void applyFillZonesInPlace(QImage& img, ZoneSet const& zones) const;

	void applyFillZonesInPlace(imageproc::BinaryImage& img, ZoneSet const& zones,
		std::function<QPointF(QPointF const&)> const& orig_to_output) const;

	void applyFillZonesInPlace(imageproc::BinaryImage& img, ZoneSet const& zones) const;
	
	std::shared_ptr<imageproc::AbstractImageTransform const> m_ptrImageTransform;
	ColorParams m_colorParams;

	/**
	 * The rectangle corresponding to the output image in transformed coordinates.
	 */
	QRect m_outRect;

	/**
	 * The content rectangle in output image coordinates. That is, if there are
	 * no margins, m_contentRect is going to be:
	 * \code
	 * QRect(QPoint(0, 0), m_outRect.size())
	 * \endcode
	 */
	QRect m_contentRect;

	DespeckleLevel m_despeckleLevel;
};

} // namespace output

#endif
