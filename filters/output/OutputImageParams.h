/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef OUTPUT_OUTPUT_IMAGE_PARAMS_H_
#define OUTPUT_OUTPUT_IMAGE_PARAMS_H_

#include "ColorParams.h"
#include "DespeckleLevel.h"
#include <QSize>
#include <QRect>
#include <QString>

class QDomDocument;
class QDomElement;
class QTransform;

namespace output
{

/**
 * \brief Parameters of the output image used to determine if we need to re-generate it.
 */
class OutputImageParams
{
public:
	OutputImageParams(QString const& transform_fingerprint,
		QRect const& output_image_rect, QRect const& content_rect,
		ColorParams const& color_params, DespeckleLevel despeckle_level);
	
	explicit OutputImageParams(QDomElement const& el);

	DespeckleLevel despeckleLevel() const { return m_despeckleLevel; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
	
	/**
	 * \brief Returns true if two sets of parameters are close enough
	 *        to avoid re-generating the output image.
	 */
	bool matches(OutputImageParams const& other) const;
private:
	static bool colorParamsMatch(
		ColorParams const& cp1, DespeckleLevel dl1,
		ColorParams const& cp2, DespeckleLevel dl2);
	
	/**
	 * Identifies the original -> output image transformation.
	 * @see AbstractImageTransform::fingerprint()
	 */
	QString m_transformFingerprint;

	/**
	 * The rectangle in transformed space corresponding to the output image.
	 * @see OutputGenerator::outputImageRect()
	 */
	QRect m_outputImageRect;
	
	/**
	 * Content rectangle in output image coordinates.
	 * @see OutputGenerator::contentRect()
	 */
	QRect m_contentRect;
	
	/** Non-geometric parameters used to generate the output image. */
	ColorParams m_colorParams;

	/** Despeckle level of the output image. */
	DespeckleLevel m_despeckleLevel;
};

} // namespace output

#endif
