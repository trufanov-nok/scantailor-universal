#ifndef CONTENTSMANAGERDIALOG_H
#define CONTENTSMANAGERDIALOG_H

#include <QDialog>
#include <QSizeF>

#include "IntrusivePtr.h"
#include "PageSequence.h"

#include <QTreeWidget>

namespace Ui {
class ContentsManagerDialog;
}

class QTreeWidgetItem;
class PageId;
class ThumbnailSequence;
class ThumbnailPixmapCache;
class ProjectPages;
class QAbstractButton;
class OutputFileNameGenerator;

namespace publish {

class DjVuBookmarkDispatcher;

class QContentsTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    QContentsTreeWidget(QWidget *parent = nullptr);
    ~QContentsTreeWidget();

    static const QString mimeType;
    void displayHeader();
    void setHintMode(bool on);
    void setPageUIDs(const QStringList& page_uids) { m_page_uids = page_uids; }
    void setOutputFileNameGenerator(const OutputFileNameGenerator* fgen) { m_outputFileNameGenerator = fgen; }
    bool hintMode() const { return m_hintMode; }
signals:
    void contentsTreeWasChanged();
    void hintModeIsChanged(bool);
protected:
    QStringList mimeTypes() const override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    bool m_hintMode;
    QStringList m_page_uids;
    const OutputFileNameGenerator * m_outputFileNameGenerator;
};


class Filter;

class ContentsManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ContentsManagerDialog(Filter* filter, QWidget *parent = nullptr);
    ~ContentsManagerDialog();

    const DjVuBookmarkDispatcher& bookmarks() const { return *m_local_bookmarks.get(); }
private slots:
    void dumpAndUpdate();

    void on_tabEditors_currentChanged(int index);

    void on_edDjVuSed_textChanged();

    void on_edPlainText_textChanged();

    void on_treeContents_itemSelectionChanged();

    void on_btnPageInc_clicked();

    void on_btnPageDec_clicked();

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_actionRemove_triggered();

private:
    void setupThumbView();
    void resetThumbSequence();

    void updateTreeView();
    void updateEditors();
    void dumpTreeStructure();
    void changePagesNumDlg(bool inc = true);

    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    Ui::ContentsManagerDialog *ui;
    Filter* m_filter;
    const OutputFileNameGenerator* m_outputFileNameGenerator;
    QStringList m_page_uids;
    std::unique_ptr<ThumbnailSequence> m_ptrThumbSequence;
    const PageSequence m_pageSequence;
    IntrusivePtr<ThumbnailPixmapCache> m_ptrThumbnailCache;
    QSizeF m_maxLogicalThumbSize;
    IntrusivePtr<DjVuBookmarkDispatcher> m_local_bookmarks;
};

}

#endif // CONTENTSMANAGERDIALOG_H
