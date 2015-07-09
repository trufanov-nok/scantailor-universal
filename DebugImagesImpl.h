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

#ifndef DEBUG_IMAGES_IMPL_H_
#define DEBUG_IMAGES_IMPL_H_

#include "DebugImages.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "DebugViewFactory.h"
#include "Utils.h"
#include "Grid.h"
#include "VecNT.h"
#include <boost/function.hpp>
#include <QString>
#include <deque>
#include <utility>
#include <memory>

class QImage;
class QWidget;

namespace imageproc
{
	class BinaryImage;
}

/**
 * \brief A sequence of image + label pairs.
 */
class DebugImagesImpl : public DebugImages
{
public:
	explicit DebugImagesImpl(
		QString const& swap_dir = Utils::swappingDir(), bool ensure_exists = true);

	virtual QString swappingDir() const;

	virtual void add(QImage const& image, QString const& label);
	
	virtual void add(imageproc::BinaryImage const& image, QString const& label);

	virtual void addVectorFieldView(
		QImage const& image, Grid<Vec2f> const& vector_field, QString const& label);

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
		boost::function<void()> const& swap_out_action, bool swap_out_now = true);
	
	bool empty() const { return m_sequence.empty(); }

	/**
	 * \brief Removes and returns the front DebugViewFactory.
	 *
	 * \param[out] label The label will be written here.
	 * \return A smart pointer to a DebugViewFactory instance, or null if nothing to retrieve.
	 */
	IntrusivePtr<DebugViewFactory> retrieveNext(QString* label);
private:
	QString m_swapDir;
	std::deque<std::pair<IntrusivePtr<DebugViewFactory>, QString> > m_sequence;
};

#endif
