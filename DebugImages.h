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

#ifndef DEBUG_IMAGES_H_
#define DEBUG_IMAGES_H_

#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "DebugViewFactory.h"
#include "Utils.h"
#include <boost/function.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <QString>
#include <QWidget>
#include <deque>
#include <utility>
#include <memory>

class QImage;

namespace imageproc
{
	class BinaryImage;
}

/**
 * \brief A sequence of image + label pairs.
 */
class DebugImages
{
public:
	explicit DebugImages(QString const& swap_dir = Utils::swappingDir(), bool ensure_exists = true);

	void add(QImage const& image, QString const& label);
	
	void add(imageproc::BinaryImage const& image, QString const& label);

	/**
	 * \brief Adds a debug view with parameters to be swapped out.
	 *
	 * Usage example:
	 * \code
	 * QImage image = ...;
	 * Grid<Vec2f> vector_field = ...; 
	 * ObjectSwapperFactory factory(swap_dir);
	 * ObjectSwapper<QImage> image_swapper(factory(image));
	 * ObjectSwapper<Grid<Vec2f> > vector_field_swapper(factory(vector_field));
	 * using namespace boost::lambda;
	 * DebugImages* dbg = ...;
	 * dbg->add(
	 *     "label", boost::fusion::make_vector(image_swapper, vector_field_swapper),
	 *     bind(new_ptr<DebugImageView>(), bind(image_swapper.accessor()), bind(vector_field_swapper.accessor()))
	 * );
	 * \endcode
	 */
	template<typename SwappableParams>
	void add(QString const& label, SwappableParams const& swappable_params,
		boost::function<QWidget* ()> const& delegate_factory, bool swap_out_now = true);
	
	bool empty() const { return m_sequence.empty(); }

	/**
	 * \brief Removes and returns the front DebugViewFactory.
	 *
	 * \param[out] label The label will be written here.
	 * \return A smart pointer to a DebugViewFactory instance, or null if nothing to retrieve.
	 */
	IntrusivePtr<DebugViewFactory> retrieveNext(QString* label);
private:
	class SwapInFunctor
	{
	public:
		template<typename T>
		void operator()(T& obj) const {
			obj.swapIn();
		}
	};

	class SwapOutFunctor
	{
	public:
		template<typename T>
		void operator()(T& obj) const {
			obj.swapOut();
		}
	};

	QString m_swapDir;
	std::deque<std::pair<IntrusivePtr<DebugViewFactory>, QString> > m_sequence;
};

template<typename SwappableParams>
void
DebugImages::add(QString const& label, SwappableParams const& swappable_params,
	boost::function<QWidget* ()> const& delegate_factory, bool swap_out_now)
{
	class Factory : public DebugViewFactory
	{
	public:
		Factory(SwappableParams const& swappable_params,
			boost::function<QWidget* ()> const& delegate_factory)
			: m_params(swappable_params), m_delegateFactory(delegate_factory) {}

		virtual void swapIn() { boost::fusion::for_each(m_params, SwapInFunctor()); }

		virtual void swapOut() { boost::fusion::for_each(m_params, SwapOutFunctor()); }

		virtual std::auto_ptr<QWidget> newInstance() { return std::auto_ptr<QWidget>(m_delegateFactory()); }
	private:
		SwappableParams m_params;
		boost::function<QWidget* ()> m_delegateFactory;
	};

	IntrusivePtr<DebugViewFactory> factory(new Factory(swappable_params, delegate_factory));
	if (swap_out_now) {
		factory->swapOut();
	}
	m_sequence.push_back(std::make_pair(factory, label));
}

#endif
