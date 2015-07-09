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

#include "DebugImagesImpl.h"
#include "ObjectSwapper.h"
#include "ObjectSwapperFactory.h"
#include "ObjectSwapperImplQImage.h"
#include "ObjectSwapperImplGrid.h"
#include "BasicImageView.h"
#include "VectorFieldImageView.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/RasterOpGeneric.h"
#include <QImage>
#include <QDir>
#include <QDebug>

using namespace imageproc;

DebugImagesImpl::DebugImagesImpl(QString const& swap_dir, bool ensure_exists)
:	m_swapDir(swap_dir)
{
	if (ensure_exists) {
		if (!QDir().mkpath(swap_dir)) {
			qDebug() << "Unable to create swap directory " << swap_dir;
		}
	}
}

QString
DebugImagesImpl::swappingDir() const
{
	return m_swapDir;
}

void
DebugImagesImpl::add(QImage const& image, QString const& label)
{
	class Factory : public DebugViewFactory
	{
	public:
		Factory(QImage const& image, QString const& swap_dir)
			: m_swapper(image, swap_dir) {} 

		virtual void swapIn() { m_swapper.swapIn(); }

		virtual void swapOut() { m_swapper.swapOut(); }

		virtual std::auto_ptr<QWidget> newInstance() {
			return std::auto_ptr<QWidget>(new BasicImageView(m_swapper.constObject()));
		}
	private:
		ObjectSwapper<QImage> m_swapper;
	};

	IntrusivePtr<DebugViewFactory> factory(new Factory(image, m_swapDir));
	factory->swapOut();
	m_sequence.push_back(std::make_pair(factory, label));
}

void
DebugImagesImpl::add(imageproc::BinaryImage const& image, QString const& label)
{
	add(image.toQImage(), label);
}

void
DebugImagesImpl::addVectorFieldView(
	QImage const& image, Grid<Vec2f> const& vector_field,
	QString const& label)
{
	float max_squared_magnitude = 0;
	rasterOpGeneric(
		[&max_squared_magnitude](Vec2f const& dir) {
			float const squared_norm = dir.squaredNorm();
			if (squared_norm > max_squared_magnitude) {
				max_squared_magnitude = squared_norm;
			}
		},
		vector_field
	);

	ObjectSwapperFactory factory(m_swapDir);
	ObjectSwapper<QImage> image_swapper(factory(image));
	ObjectSwapper<Grid<Vec2f>> vector_field_swapper(factory(vector_field));
	add(
		label,
		[=]() {
			return new VectorFieldImageView(
				image_swapper.constObject(), vector_field_swapper.constObject(),
				std::sqrt(max_squared_magnitude)
			);
		},
		[=]() mutable { image_swapper.swapIn(); vector_field_swapper.swapIn(); },
		[=]() mutable { image_swapper.swapOut(); vector_field_swapper.swapOut(); }
	);
}

void
DebugImagesImpl::add(QString const& label,
	boost::function<QWidget* ()> const& image_view_factory,
	boost::function<void()> const& swap_in_action,
	boost::function<void()> const& swap_out_action, bool swap_out_now)
{
	class Factory : public DebugViewFactory
	{
	public:
		Factory(
			boost::function<QWidget* ()> const& image_view_factory,
			boost::function<void()> const& swap_in_action,
			boost::function<void()> const& swap_out_action)
		:	m_imageViewFactory(image_view_factory)
		,	m_swapInAction(swap_in_action)
		,	m_swapOutAction(swap_out_action)
		{
		}

		virtual void swapIn() { m_swapInAction(); }

		virtual void swapOut() { m_swapOutAction(); }

		virtual std::auto_ptr<QWidget> newInstance() {
			return std::auto_ptr<QWidget>(m_imageViewFactory());
		}
	private:
		boost::function<QWidget*()> m_imageViewFactory;
		boost::function<void()> m_swapInAction;
		boost::function<void()> m_swapOutAction;
	};

	IntrusivePtr<DebugViewFactory> factory(
		new Factory(image_view_factory, swap_in_action, swap_out_action)
	);
	if (swap_out_now) {
		factory->swapOut();
	}
	m_sequence.push_back(std::make_pair(factory, label));
}

IntrusivePtr<DebugViewFactory>
DebugImagesImpl::retrieveNext(QString* label)
{
	IntrusivePtr<DebugViewFactory> factory;

	if (!m_sequence.empty()) {
		factory = m_sequence.front().first;
		*label = m_sequence.front().second;
		m_sequence.pop_front();
	}

	return factory;
}
