/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "DebugImageView.h"
#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "ImageViewBase.h"
#include "BasicImageView.h"
#include "ProcessingIndicationWidget.h"
#include <QImage>
#include <QPointer>
#include <memory>

class DebugImageView::BackgroundLoadResult : public AbstractCommand0<void>
{
public:
	BackgroundLoadResult(QPointer<DebugImageView> const& owner) : m_ptrOwner(owner) {}

	// This method is called from the main thread.
	virtual void operator()() {
		if (DebugImageView* owner = m_ptrOwner) {
			owner->factoryReady();
		}
	}
private:
	QPointer<DebugImageView> m_ptrOwner;
};


class DebugImageView::BackgroundLoader :
	public AbstractCommand0<BackgroundExecutor::TaskResultPtr>
{
public:
	BackgroundLoader(DebugImageView* owner)
		: m_ptrOwner(owner), m_ptrFactory(owner->m_ptrFactory) {}

	virtual BackgroundExecutor::TaskResultPtr operator()() {
		m_ptrFactory->swapIn();
		return BackgroundExecutor::TaskResultPtr(new BackgroundLoadResult(m_ptrOwner));
	}
private:
	QPointer<DebugImageView> m_ptrOwner;
	IntrusivePtr<DebugViewFactory> m_ptrFactory;
};


DebugImageView::DebugImageView(IntrusivePtr<DebugViewFactory> const& factory, QWidget* parent)
:	QStackedWidget(parent)
,	m_ptrFactory(factory)
,	m_pPlaceholderWidget(new ProcessingIndicationWidget(this))
,	m_numBgTasksInitiated(0)
,	m_isLive(false)
{
	addWidget(m_pPlaceholderWidget);
}

void
DebugImageView::setLive(bool const live)
{
	if (live && !m_isLive) {
		// Going live.
		ImageViewBase::backgroundExecutor().enqueueTask(
			BackgroundExecutor::TaskPtr(new BackgroundLoader(this))
		);
		++m_numBgTasksInitiated;
	} else if (!live && m_isLive) {
		// Going dead.
		if (QWidget* wgt = currentWidget()) {
			if (wgt != m_pPlaceholderWidget) {
				removeWidget(wgt);
				delete wgt;
			}
		}
	}

	m_isLive = live;
}

void
DebugImageView::factoryReady()
{
	if (--m_numBgTasksInitiated != 0) {
		// We can't do anything with m_ptrFactory while a background
		// thread may be doing swapIn() on it.
		return;
	}

	if (m_isLive && currentWidget() == m_pPlaceholderWidget) {
		std::auto_ptr<QWidget> image_view(m_ptrFactory->newInstance());
		setCurrentIndex(addWidget(image_view.release()));
	}

	m_ptrFactory->swapOut();
}
