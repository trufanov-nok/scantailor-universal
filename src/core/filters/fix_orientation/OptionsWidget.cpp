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

#include "OptionsWidget.h"

#include "Filter.h"
#include "ApplyToDialog.h"
#include "Settings.h"
#include "ProjectPages.h"
#include "ImageId.h"
#include "PageId.h"
#include <vector>
#include <assert.h>

namespace fix_orientation
{

OptionsWidget::OptionsWidget(
    IntrusivePtr<Settings> const& settings,
    PageSelectionAccessor const& page_selection_accessor)
    :   m_ptrSettings(settings),
        m_pageSelectionAccessor(page_selection_accessor)
{
    setupUi(this);

    connect(rotateLeftBtn, SIGNAL(clicked()), this, SLOT(rotateLeft()));
    connect(rotateRightBtn, SIGNAL(clicked()), this, SLOT(rotateRight()));
    connect(resetBtn, SIGNAL(clicked()), this, SLOT(resetRotation()));
    connect(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(
    PageId const& page_id, OrthogonalRotation const rotation)
{
    m_pageId = page_id;
    m_rotation = rotation;
    setRotationPixmap();
}

void
OptionsWidget::postUpdateUI()
{
    setRotation(m_rotation);
}

void
OptionsWidget::rotateLeft()
{
    OrthogonalRotation rotation(m_rotation);
    rotation.prevClockwiseDirection();
    setRotation(rotation);
}

void
OptionsWidget::rotateRight()
{
    OrthogonalRotation rotation(m_rotation);
    rotation.nextClockwiseDirection();
    setRotation(rotation);
}

void
OptionsWidget::resetRotation()
{
    setRotation(OrthogonalRotation());
}

void
OptionsWidget::showApplyToDialog()
{
    ApplyToDialog* dialog = new ApplyToDialog(
        this, m_pageId, m_pageSelectionAccessor, PageView::IMAGE_VIEW
    );

    dialog->setWindowTitle(tr("Fix Orientation"));

    connect(dialog, &ApplyToDialog::accepted, this, [ = ]() {
        std::vector<PageId> vec = dialog->getPageRangeSelectorWidget().result();
        std::set<PageId> pages(vec.begin(), vec.end());
        if (!dialog->getPageRangeSelectorWidget().allPagesSelected()) {
            appliedTo(pages);
        } else {
            appliedToAllPages(pages);
        }
    });

    dialog->show();
}

void
OptionsWidget::appliedTo(std::set<PageId> const& pages)
{
    if (pages.empty()) {
        return;
    }

    m_ptrSettings->applyRotation(pages, m_rotation);
    for (PageId const& page_id : pages) {
        emit invalidateThumbnail(page_id);
    }
}

void
OptionsWidget::appliedToAllPages(std::set<PageId> const& pages)
{
    m_ptrSettings->applyRotation(pages, m_rotation);
    emit invalidateAllThumbnails();
}

void
OptionsWidget::setRotation(OrthogonalRotation const rotation)
{
    if (rotation == m_rotation) {
        return;
    }

    m_rotation = rotation;
    setRotationPixmap();

    m_ptrSettings->applyRotation(m_pageId.imageId(), rotation);

    emit rotated(rotation);
    emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::setRotationPixmap()
{
    char const* path = 0;

    switch (m_rotation.toDegrees()) {
    case 0:
        path = ":/icons/big-up-arrow.png";
        break;
    case 90:
        path = ":/icons/big-right-arrow.png";
        break;
    case 180:
        path = ":/icons/big-down-arrow.png";
        break;
    case 270:
        path = ":/icons/big-left-arrow.png";
        break;
    default:
        assert(!"Unreachable");
    }

    rotationIndicator->setPixmap(QPixmap(path));
}

} // namespace fix_orientation
