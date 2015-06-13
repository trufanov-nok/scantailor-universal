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

#include "DebugImages.h"
#include "ObjectSwapper.h"
#include "ObjectSwapperImplQImage.h"
#include "BasicImageView.h"
#include "imageproc/BinaryImage.h"
#include <QImage>
#include <QDir>
#include <QDebug>

DebugImages::DebugImages(QString const& swap_dir, bool ensure_exists)
:	m_swapDir(swap_dir)
{
	if (ensure_exists) {
		if (!QDir().mkpath(swap_dir)) {
			qDebug() << "Unable to create swap directory " << swap_dir;
		}
	}
}

void
DebugImages::add(QImage const& image, QString const& label)
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
DebugImages::add(imageproc::BinaryImage const& image, QString const& label)
{
	add(image.toQImage(), label);
}

void
DebugImages::add(QString const& label,
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
DebugImages::retrieveNext(QString* label)
{
	IntrusivePtr<DebugViewFactory> factory;

	if (!m_sequence.empty()) {
		factory = m_sequence.front().first;
		*label = m_sequence.front().second;
		m_sequence.pop_front();
	}

	return factory;
}
