#ifndef PAGERANGESELECTORWIDGET_H
#define PAGERANGESELECTORWIDGET_H

#include "PageId.h"
#include "PageSequence.h"
#include "PageView.h"
#include "PageSelectionAccessor.h"
#include <QWidget>

class PageRangeSelectorSignalsPropagator: public QObject
{
    Q_OBJECT
public:
    static PageRangeSelectorSignalsPropagator* get() {
        if (!m_obj) {
            m_obj = new PageRangeSelectorSignalsPropagator();
        }
        return m_obj;
    }
signals:
    void maybeTargeted(const std::vector<PageId> pages);
private:
    static PageRangeSelectorSignalsPropagator* m_obj;
};


namespace Ui {
class PageRangeSelectorWidget;
}

class PageRangeSelectorWidget : public QWidget
{
    Q_OBJECT
    enum RangeOptions {
        Selected,
        All,
        AllAfter,
        AllBefore
    };

    enum FilterOption {
        None        = 0,
        EvenPages   = 1,
        OddPages    = 2,
        SinglePages = 4,
        LeftPages   = 8,
        RightPages = 16
    };

public:
    explicit PageRangeSelectorWidget(QWidget *parent = Q_NULLPTR);
    void setData(PageId const& cur_page, PageSelectionAccessor const& page_selection_accessor, PageView mode = PageView::PAGE_VIEW);
    ~PageRangeSelectorWidget();

    std::vector<PageId> result() const { return m_filteredPages; }
    bool allPagesSelected() const {
        // we could check m_range and m_curFilter setting, but filter might remove nothing
        // and note that m_curPage is always removed from result
        return m_filteredPages.size() == m_pages.numPages()-1;
    }

private slots:
    void on_rbSelected_clicked();

    void on_rbAll_clicked();

    void on_rbAllAfter_clicked();

    void on_rbAllBefore_clicked();

    void on_cbEven_toggled(bool checked);

    void on_cbOdd_toggled(bool checked);

    void on_cbSingle_toggled(bool checked);

    void on_cbLeft_toggled(bool checked);

    void on_cbRight_toggled(bool checked);

signals:

    void maybeTargeted(const std::vector<PageId> pages);

private:
    RangeOptions range() const { return m_range; }
    int filter() const { return m_curFilter; }

    void setRange(RangeOptions range, bool force_upd = false);
    void setFilter(int filter, bool force_upd = false);
    void displayLabelCounters();
private:
    PageSequence m_pages;
    std::set<PageId> m_selectedPages;
    std::vector<PageRange> m_selectedRanges;
    PageId m_curPage;
    Ui::PageRangeSelectorWidget *ui;
    int m_curFilter;
    RangeOptions m_range;
    std::vector<PageId> m_rangePages;
    std::vector<PageId> m_filteredPages;
    int m_lblCounterTotal;
    int m_lblCounterFiltered;
};

#endif // PAGERANGESELECTORWIDGET_H
