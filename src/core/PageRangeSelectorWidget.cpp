/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph_a@mail.ru>

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

#include "PageRangeSelectorWidget.h"
#include "ui_PageRangeSelectorWidget.h"
#include "PageInfo.h"

PageRangeSelectorSignalsPropagator* PageRangeSelectorSignalsPropagator::m_obj = nullptr;

PageRangeSelectorWidget::PageRangeSelectorWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::PageRangeSelectorWidget)
{
    ui->setupUi(this);
    connect(this, &PageRangeSelectorWidget::maybeTargeted,
            PageRangeSelectorSignalsPropagator::get(), &PageRangeSelectorSignalsPropagator::maybeTargeted);
}

void PageRangeSelectorWidget::setData(PageId const& cur_page, PageSelectionAccessor const& page_selection_accessor, PageView mode)
{
    m_pages = page_selection_accessor.allPages();
    m_selectedPages = page_selection_accessor.selectedPages();
    m_selectedRanges = page_selection_accessor.selectedRanges();
    m_curPage = cur_page;
    m_curFilter = PageRangeSelectorWidget::None;
    m_rangePages.clear();
    m_filteredPages.clear();
    m_lblCounterFiltered = m_lblCounterTotal = 0;

    const int sel_cnt = m_selectedPages.size();
    if (sel_cnt == 1) {
        ui->rbSelected->setText(tr("Selected (single page)"));
    } else {
        ui->rbSelected->setText(tr("Selected (%n pages)", "plural forms supported", sel_cnt));
    }

//    ui->rbSelected->setEnabled(sel_cnt > 1);

//    if (ui->rbSelected->isEnabled()) {
    ui->rbSelected->blockSignals(true);
    ui->rbSelected->setChecked(true);
    ui->rbSelected->blockSignals(false);
    setRange(PageRangeSelectorWidget::Selected, true);
//    } else {
//        ui->rbAll->blockSignals(true);
//        ui->rbAll->setChecked(true);
//        ui->rbAll->blockSignals(false);
//        setRange(PageRangeSelectorWidget::All, true);
//    }

    if (mode == PageView::IMAGE_VIEW) {
        // pages aren't split to left/right yet
        ui->cbSingle->setVisible(false);
        ui->cbLeft->setVisible(false);
        ui->cbRight->setVisible(false);
    }

    std::vector<PageInfo>::const_iterator it_all = m_pages.begin();
    std::vector<PageInfo>::const_iterator it_all_end = m_pages.end();
    bool odd = true;
    while (it_all->id() != m_curPage && it_all != it_all_end) {
        ++it_all;
        odd = !odd;
    }

    ui->lblCurrentIsEvenOrOdd->setText(
        odd ? tr("(current page is odd)") :
        tr("(current page is even)")
    );

    displayLabelCounters();
}

PageRangeSelectorWidget::~PageRangeSelectorWidget()
{
    // clear possible thumbnails hints
    emit maybeTargeted(std::vector<PageId>());

    delete ui;
}

void PageRangeSelectorWidget::on_rbSelected_clicked()
{
    const bool multirange = m_selectedRanges.size() > 1;
    ui->cbEven->setEnabled(!multirange);
    ui->cbOdd->setEnabled(!multirange);
    if (multirange) {
        ui->cbEven->blockSignals(true);
        ui->cbOdd->blockSignals(true);
        ui->cbEven->setChecked(false);
        ui->cbOdd->setChecked(false);
        const QString txt = tr("Can't do: more than one group is selected.");
        ui->cbEven->setToolTip(txt);
        ui->cbOdd->setToolTip(txt);

        int f = m_curFilter & ~(PageRangeSelectorWidget::EvenPages | PageRangeSelectorWidget::OddPages);
        if (m_range != PageRangeSelectorWidget::Selected || m_curFilter != f) {
            m_curFilter = f; // trick to change both filter and range at one call
            setRange(PageRangeSelectorWidget::Selected, true);
        }
        ui->cbEven->blockSignals(false);
        ui->cbOdd->blockSignals(false);
    } else {
        setRange(PageRangeSelectorWidget::Selected);
    }
}

void PageRangeSelectorWidget::on_rbAll_clicked()
{
    setRange(PageRangeSelectorWidget::All);
}

void PageRangeSelectorWidget::on_rbAllAfter_clicked()
{
    setRange(PageRangeSelectorWidget::AllAfter);
}

void PageRangeSelectorWidget::on_rbAllBefore_clicked()
{
    setRange(PageRangeSelectorWidget::AllBefore);
}

void PageRangeSelectorWidget::on_cbEven_toggled(bool checked)
{
    ui->cbOdd->setEnabled(!checked);
    if (checked) {
        int f = filter();
        // do explicitly to call setFilter() once for two filter changes
        ui->cbOdd->blockSignals(true);
        ui->cbOdd->setChecked(false);
        f &= ~PageRangeSelectorWidget::OddPages;
        ui->cbOdd->blockSignals(false);
        setFilter(f | PageRangeSelectorWidget::EvenPages);
    } else {
        setFilter(filter() & ~PageRangeSelectorWidget::EvenPages);
    }

}

void PageRangeSelectorWidget::on_cbOdd_toggled(bool checked)
{
    ui->cbEven->setEnabled(!checked);
    if (checked) {
        int f = filter();
        // do explicitly to call setFilter() once for two filter changes
        ui->cbEven->blockSignals(true);
        ui->cbEven->setChecked(false);
        f &= ~PageRangeSelectorWidget::EvenPages;
        ui->cbEven->blockSignals(false);
        setFilter(f | PageRangeSelectorWidget::OddPages);
    } else {
        setFilter(filter() & ~PageRangeSelectorWidget::OddPages);
    }
}

void PageRangeSelectorWidget::on_cbSingle_toggled(bool checked)
{
    int f = checked ? filter() | PageRangeSelectorWidget::SinglePages :
            filter() & ~PageRangeSelectorWidget::SinglePages;
    setFilter(f);
}

void PageRangeSelectorWidget::on_cbLeft_toggled(bool checked)
{
    int f = checked ? filter() | PageRangeSelectorWidget::LeftPages :
            filter() & ~PageRangeSelectorWidget::LeftPages;
    setFilter(f);
}

void PageRangeSelectorWidget::on_cbRight_toggled(bool checked)
{
    int f = checked ? filter() | PageRangeSelectorWidget::RightPages :
            filter() & ~PageRangeSelectorWidget::RightPages;
    setFilter(f);
}

void PageRangeSelectorWidget::setRange(RangeOptions range, bool force_upd)
{
    if (m_range == range && !force_upd) {
        return;
    } else {
        m_rangePages.clear();

        if (range == PageRangeSelectorWidget::All) {
            for (PageInfo const& page_info : m_pages) {
                m_rangePages.push_back(page_info.id());
            }
        } else if (range == PageRangeSelectorWidget::AllBefore) {
            std::vector<PageInfo>::const_iterator it(m_pages.begin());
            std::vector<PageInfo>::const_iterator const end(m_pages.end());
            for (; it != end && it->id() != m_curPage; ++it) {
                // Continue till we have a match.
                m_rangePages.push_back(it->id());
            }

            m_rangePages.push_back(m_curPage);
        } else if (range == PageRangeSelectorWidget::AllAfter) {
            std::vector<PageInfo>::const_iterator it(m_pages.begin());
            std::vector<PageInfo>::const_iterator const end(m_pages.end());
            for (; it != end && it->id() != m_curPage; ++it) {
                // Continue until we have a match.
            }
            for (; it != end; ++it) {
                m_rangePages.push_back(it->id());
            }
        } else if (range == PageRangeSelectorWidget::Selected) {
            for (PageInfo const& page_info : m_pages) {
                // m_rangePages will contain all pages from m_selectedPages but in order as in m_pages
                if (m_selectedPages.find(page_info.id()) != m_selectedPages.end()) {
                    m_rangePages.push_back(page_info.id());
                }
            }
        }
    }

    m_lblCounterTotal = m_rangePages.size();
    m_range = range;
    setFilter(m_curFilter, true);
}

void PageRangeSelectorWidget::setFilter(int filter, bool force_upd)
{
    if (m_curFilter == filter && !force_upd) {
        displayLabelCounters();
        return;
    }

    m_curFilter = filter;

    if (m_curFilter == PageRangeSelectorWidget::None) {
        m_filteredPages = std::vector<PageId>(m_rangePages);
        m_lblCounterFiltered = 0;

//        {   // can't remove m_curPage beforehand as it affects odd/even
//            std::vector<PageId>::iterator it;
//            if ((it = std::find(m_filteredPages.begin(), m_filteredPages.end(), m_curPage)) != m_filteredPages.end()) {
//                m_filteredPages.erase(it);
//            }
//        }

        displayLabelCounters();
        emit maybeTargeted(m_filteredPages);
        return;
    }

    m_filteredPages.clear();

    std::vector<PageInfo>::const_iterator it_all = m_pages.begin();
    std::vector<PageInfo>::const_iterator it_all_end = m_pages.end();
    std::vector<PageId>::const_iterator it = m_rangePages.cbegin();
    std::vector<PageId>::const_iterator it_end = m_rangePages.cend();

    bool odd = true;
    const bool count_odds = m_curFilter & (PageRangeSelectorWidget::OddPages | PageRangeSelectorWidget::EvenPages);

    if (count_odds && m_range != PageRangeSelectorWidget::All) {
        while (it_all->id() != *it &&
                it_all != it_all_end) {
            ++it_all;
            odd = !odd;
        }
    }

    for (; it != it_end; ++it) {

        bool filter_it = false;//(*it == m_curPage); // exclude current page
        if (!count_odds || it_all != it_all_end) {
            // filter PageID
            if (count_odds) {
                filter_it = filter_it || ((m_curFilter & PageRangeSelectorWidget::OddPages) && odd);
                filter_it = filter_it || ((m_curFilter & PageRangeSelectorWidget::EvenPages) && !odd);
            }
            filter_it = filter_it || ((m_curFilter & PageRangeSelectorWidget::SinglePages) && it->subPage() == PageId::SINGLE_PAGE);
            filter_it = filter_it || ((m_curFilter & PageRangeSelectorWidget::LeftPages) && it->subPage() == PageId::LEFT_PAGE);
            filter_it = filter_it || ((m_curFilter & PageRangeSelectorWidget::RightPages) && it->subPage() == PageId::RIGHT_PAGE);
        }

        if (!filter_it) {
            m_filteredPages.push_back(*it);
        }

        if (count_odds) {
            odd = !odd;
        }
    }

    m_lblCounterFiltered = m_rangePages.size() - m_filteredPages.size();
    m_curFilter = filter;
    displayLabelCounters();
    emit maybeTargeted(m_filteredPages);
}

QString getText(int val)
{
    QString res;
    if (abs(val) != 1) {
        res = QObject::tr("%n pages", "plural forms supported", abs(val));
    } else {
        res = QObject::tr("%1 page").arg(abs(val));
    }

    if (val < 0) {
        // plural overload of tr() doesn't support negative values
        res = QObject::tr("-%1").arg(res);
    }
    return res;
}

void PageRangeSelectorWidget::displayLabelCounters()
{
    ui->lblAffectedPagesCnt->setText(getText(m_lblCounterTotal - m_lblCounterFiltered));
    ui->lblExcludedPagesCnt->setText(getText(-1 * m_lblCounterFiltered/* + 1*/));

//    const QString tt = m_lblCounterFiltered == 1 ? tr("Current page is always excluded") : "";
//    ui->lblExcludedPagesCnt->setToolTip(tt);
}
