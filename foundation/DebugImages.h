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

#ifndef DEBUG_IMAGES_H_
#define DEBUG_IMAGES_H_

#include "foundation_config.h"
#include "Grid.h"
#include "VecNT.h"
#include <boost/function.hpp>
#include <QString>

class QImage;
class QWidget;

namespace imageproc
{
	class BinaryImage;
}

/**
 * \brief Stores images used for debugging.
 */
class FOUNDATION_EXPORT DebugImages
{
public:
	virtual ~DebugImages();

	virtual QString swappingDir() const = 0;

	virtual void add(QImage const& image, QString const& label) = 0;
	
	virtual void add(imageproc::BinaryImage const& image, QString const& label) = 0;

	virtual void addVectorFieldView(
		QImage const& image, Grid<Vec2f> const& vector_field, QString const& label) = 0;

	/**
	 * \brief The most general add() function.
	 *
	 * Usage example:
	 * \code
	 * QImage image = ...;
	 * Grid<Vec2f> vector_field = ...; 
	 * ObjectSwapperFactory factory(swap_dir);
	 * ObjectSwapper<QImage> image_swapper(factory(image));
	 * ObjectSwapper<Grid<Vec2f> > vector_field_swapper(factory(vector_field));
	 * DebugImages* dbg = ...;
	 * dbg->add(
	 *     "label", [=]() {
	 *         return new CustomImageView(
	 *             image_swapper.constObject(), vector_field_swapper.constObject());
	 *         );
	 *     },
	 *     [=]() mutable { image_swapper.swapIn(); vector_field_swapper.swapIn(); },
	 *     [=]() mutable { image_swapper.swapOut(); vector_field_swapper.swapOut(); }
	 * );
	 * \endcode
	 */
	virtual void add(QString const& label,
		boost::function<QWidget*()> const& image_view_factory,
		boost::function<void()> const& swap_in_action,
		boost::function<void()> const& swap_out_action, bool swap_out_now = true) = 0;
};

#endif
