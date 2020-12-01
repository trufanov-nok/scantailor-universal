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

#include "TabbedImageView.h"

#include "ImageViewBase.h"
#include "DespeckleView.h"
#include <QScrollBar>

namespace output
{

TabbedImageView::TabbedImageView(QWidget* parent)
    :   QTabWidget(parent), m_previouslySelectedTabId(-1), m_stored_zoom_lvl(0),
        m_stored_hrz_slider(nullptr), m_stored_vrt_slider(nullptr)
{
    connect(this, SIGNAL(currentChanged(int)), SLOT(tabChangedSlot(int)));
}

qreal getSliderScale(QAbstractSlider* _old, QAbstractSlider* _new)
{
    if (_old && _new) {
        const int val1 = _old->maximum() - _old->minimum();
        const int val2 = _new->maximum() - _new->minimum();
        return (qreal) val2 / val1;
    }
    return 1.;
}

void
TabbedImageView::addTab(QWidget* widget, QString const& label, ImageViewTab tab)
{
    QTabWidget::addTab(widget, label);
    m_registry[widget] = tab;

    if (DespeckleView* dw = qobject_cast<DespeckleView*>(widget)) {
        connect(dw, &DespeckleView::imageViewCreated, this, [this](ImageViewBase * iv) {
            // this will be called once or twice when Despeckle view first opened
            // and create its image views
            if (!iv) {
                return;
            }

            if (m_stored_zoom_lvl != 0) {
                iv->setZoomLevel(m_stored_zoom_lvl);
            }
            if (m_stored_hrz_slider) {
                qreal val = m_stored_hrz_slider->value();
                val *= getSliderScale(m_stored_hrz_slider, iv->horizontalScrollBar());
                iv->horizontalScrollBar()->setValue(val);
            }
            if (m_stored_vrt_slider) {
                qreal val = m_stored_vrt_slider->value();
                val *= getSliderScale(m_stored_vrt_slider, iv->verticalScrollBar());
                iv->verticalScrollBar()->setValue(val);
            }

        });
    }
}

void
TabbedImageView::setCurrentTab(ImageViewTab const tab)
{
    int const cnt = count();
    for (int i = 0; i < cnt; ++i) {
        QWidget* wgt = widget(i);
        std::map<QWidget*, ImageViewTab>::const_iterator it(m_registry.find(wgt));
        if (it != m_registry.end()) {
            if (it->second == tab) {
                setCurrentIndex(i);
                break;
            }
        }
    }
}

ImageViewBase* findImageViewBase(QObject* parent)
{
    if (!parent) {
        return nullptr;
    }

    if (ImageViewBase* res = qobject_cast<ImageViewBase*> (parent)) {
        return res;
    } else {
        for (QObject* ch : parent->children()) {
            if (ImageViewBase* res = findImageViewBase(ch)) {
                return res;
            }
        }
    }
    return nullptr;
}

void
TabbedImageView::tabChangedSlot(int const idx)
{
    QWidget* wgt = widget(idx);
    std::map<QWidget*, ImageViewTab>::const_iterator it(m_registry.find(wgt));
    if (it != m_registry.end()) {
        emit tabChanged(it->second);
    }

    // copy zoom settings
    if (m_previouslySelectedTabId != -1) {
        ImageViewBase* old_tab = findImageViewBase(widget(m_previouslySelectedTabId));
        ImageViewBase* new_tab(nullptr);
        if (old_tab) {
            new_tab = findImageViewBase(wgt);
            if (new_tab && old_tab != new_tab) {
                qreal val = old_tab->zoomLevel();
                new_tab->setZoomLevel(val);

                val = old_tab->horizontalScrollBar()->value();
                val *= getSliderScale(old_tab->horizontalScrollBar(), new_tab->horizontalScrollBar());
                new_tab->horizontalScrollBar()->setValue(val);

                val = old_tab->verticalScrollBar()->value();
                val *= getSliderScale(old_tab->verticalScrollBar(), new_tab->verticalScrollBar());
                new_tab->verticalScrollBar()->setValue(val);

                m_previouslySelectedTabId = idx;
            } else if (qobject_cast<DespeckleView*>(wgt)) {
                // imageview isn't ready in despeckle yet
                m_stored_zoom_lvl = old_tab->zoomLevel();
                m_stored_hrz_slider = old_tab->horizontalScrollBar();
                m_stored_vrt_slider = old_tab->verticalScrollBar();
            }
        }
    }
    if (m_previouslySelectedTabId == -1) {
        m_previouslySelectedTabId = idx;
    }
}

} // namespace output

