#include "ContentsManagerDialog.h"
#include "ui_ContentsManagerDialog.h"

#include <QScrollBar>
#include <QSettings>
#include <QGraphicsItem>
#include <QMimeData>
#include <cmath>
#include <QShortcut>
#include <QAbstractButton>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>

#include "settings/globalstaticsettings.h"
#include "ThumbnailSequence.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailFactory.h"

#include "ProjectPages.h"
#include "Filter.h"
#include "Settings.h"
#include "DjVuBookmarkDispatcher.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"

#include <qdebug.h>

namespace publish {

const QString QContentsTreeWidget::mimeType = "application/stu-contents-entry";

QContentsTreeWidget::QContentsTreeWidget(QWidget *parent): QTreeWidget(parent),
    m_hintMode(false),
    m_outputFileNameGenerator(nullptr)
{
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
}

QContentsTreeWidget::~QContentsTreeWidget()
{
}

QStringList
QContentsTreeWidget::mimeTypes() const
{
    return QStringList() << QContentsTreeWidget::mimeType << PageId::mimeType;
}

void QContentsTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event && reinterpret_cast<QGraphicsScene*>( event->source())) {
        if (QTreeWidgetItem* item = itemAt(event->pos())) {
            if (item != currentItem()) {
                event->accept();
            } else {
                event->ignore();
            }
        }

    }

    QTreeWidget::dragMoveEvent(event);
}

void QContentsTreeWidget::dropEvent(QDropEvent *event)
{
    bool was_changed = false;

    if (const QMimeData* mime = event->mimeData()) {
        if (mime->hasFormat(PageId::mimeType)) {
            QByteArray data = mime->data(PageId::mimeType);

            int pages_cnt;
            int read = sizeof(pages_cnt);
            assert(data.size() > read);
            memcpy(&pages_cnt, data.data(), read);

            QVector<PageId> pages;
            PageId pageId;

            for (int i = 0; i < pages_cnt; i++) {
                data.setRawData(data.data() + read, data.size() - read);
                if (!data.isEmpty()) {
                    read = PageId::fromByteArray(data, pageId);
                    pages += pageId;
                }
            }

            assert(!pages.isEmpty());

            setHintMode(false);

            QTreeWidgetItem* parent = itemAt(event->pos());

            blockSignals(true);
            for (const PageId& p: qAsConst(pages)) {
                QTreeWidgetItem* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(this);
                item->setFlags(item->flags().setFlag(Qt::ItemIsDropEnabled, true).setFlag(Qt::ItemIsEditable, true));
                item->setText(0, tr("title"));

                const QString url = QString("#%1.djvu").
                           arg(QFileInfo(m_outputFileNameGenerator->fileNameFor(p)).completeBaseName());
                const int page_no = m_page_uids.indexOf(url);
                if (page_no != -1) {
                    item->setText(1, QString::number(page_no));
                } else {
                    item->setText(1, url);
                }
                item->setData(1, Qt::UserRole, item->text(1)); // remember initial value
            }
            blockSignals(false);

            if (parent) {
                expandItem(parent);
            }

            event->acceptProposedAction();
            was_changed = true;
        } else if (mime->hasFormat(QContentsTreeWidget::mimeType)) {
            was_changed = true;
        }
    }

    QTreeWidget::dropEvent(event);

    if (was_changed) {
        emit contentsTreeWasChanged();
    }
}

void
QContentsTreeWidget::displayHeader()
{
//    if (!m_hintMode) {
//        QTreeWidgetItem * header_item = new QTreeWidgetItem();
//        header_item->setText(0, tr("Title"));
//        header_item->setText(1, tr("Page"));
//        setHeaderItem(header_item);
//    }
}

void
QContentsTreeWidget::setHintMode(bool on)
{
    if (m_hintMode != on) {
        m_hintMode = on;
        clear();
        displayHeader();

        if (m_hintMode) {
            setColumnCount(1);
            QTreeWidgetItem* hint = new QTreeWidgetItem(this);
            hint->setFlags(hint->flags().setFlag(Qt::ItemIsDragEnabled, false));
            hint->setData(0, Qt::UserRole+1, "hint");
            QLabel* label = new QLabel(tr("Drag and drop page icons here to create a new contents entry."));
            label->setWordWrap(true);
            label->setMaximumWidth(width());
            setItemWidget(hint, 0, label);
        } else {
            if (columnCount() != 2) {
                setColumnCount(2);
            }
        }
//        setHeaderHidden(m_hintMode);
//        header()->setVisible(m_hintMode);


        emit hintModeIsChanged(m_hintMode);
    }
}


ContentsManagerDialog::ContentsManagerDialog(Filter* filter, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContentsManagerDialog),
    m_filter(filter),
    m_pageSequence(m_filter->pages()->toPageSequence(PAGE_VIEW)),
    // make a local copy of a djvu dispatcher
    m_local_bookmarks(new DjVuBookmarkDispatcher(*m_filter->settings()->djVuBookmarkDispatcher().get()))
{
    m_outputFileNameGenerator = m_filter->outputFileNameGenerator();
    assert(m_outputFileNameGenerator);
    for (const PageInfo& p: qAsConst(m_pageSequence)) {
        m_page_uids += QString("#%1.djvu").arg(
                    QFileInfo(m_outputFileNameGenerator->fileNameFor(p.id())).completeBaseName());
    }

    QSettings settings;
    m_maxLogicalThumbSize = settings.value(_key_thumbnails_max_thumb_size, _key_thumbnails_max_thumb_size_def).toSizeF();
    m_maxLogicalThumbSize /= 2;
    m_ptrThumbSequence.reset(new ThumbnailSequence(m_maxLogicalThumbSize));
    m_ptrThumbSequence->setDraggingEnabled(true);
    m_ptrThumbSequence->setIsDjbzView(true);

    ui->setupUi(this);
    ui->treeContents->setOutputFileNameGenerator(m_outputFileNameGenerator);
    ui->treeContents->setPageUIDs(m_page_uids);
    ui->treeContents->addAction(ui->actionSelect_all);
    ui->treeContents->addAction(ui->actionExpand_all);
    ui->treeContents->addAction(ui->actionCollapse_all);
    ui->treeContents->addAction(ui->actionRemove);
    ui->tabEditors->setCurrentIndex(0);

    setupThumbView();
    resetThumbSequence();

    updateTreeView();
    updateEditors();

    ui->thumbView->ensureVisible(m_ptrThumbSequence->selectionLeaderSceneRect(), 0, 0);

    connect(ui->treeContents, &QContentsTreeWidget::contentsTreeWasChanged, this, &ContentsManagerDialog::dumpAndUpdate);
    connect(ui->treeContents, &QContentsTreeWidget::itemChanged, this, &ContentsManagerDialog::dumpAndUpdate);
    connect(ui->treeContents, &QContentsTreeWidget::itemChanged, this, [=](QTreeWidgetItem *item, int column){
        if (item && column == 1) {
            QString old_val = item->data(1, Qt::UserRole).toString();
            QString new_val = item->text(1);
            if (old_val != new_val) {
                const std::set<PageId> items = m_ptrThumbSequence->selectedItems();
                QSet<PageId> selected_items(items.cbegin(), items.cend());
                bool ok;
                int page_no = old_val.toUInt(&ok);
                if (ok && page_no < m_page_uids.count()) {
                    selected_items -= m_pageSequence.pageAt(page_no).id();
                }

                page_no = new_val.toUInt(&ok);
                if (ok && page_no < m_page_uids.count()) {
                    selected_items += m_pageSequence.pageAt(page_no).id();
                }
                m_ptrThumbSequence->setSelection(selected_items, ThumbnailSequence::RESET_SELECTION);
                ui->treeContents->blockSignals(true);
                item->setData(1, Qt::UserRole, new_val);
                ui->treeContents->blockSignals(false);
            }
        }
    });


    connect(ui->treeContents, &QContentsTreeWidget::hintModeIsChanged, ui->btnPageInc, &QToolButton::setDisabled);
    connect(ui->treeContents, &QContentsTreeWidget::hintModeIsChanged, ui->btnPageDec, &QToolButton::setDisabled);

    connect(ui->actionSelect_all, &QAction::triggered, this, [=](){
       ui->treeContents->selectAll();
    });
    connect(ui->actionExpand_all, &QAction::triggered, ui->treeContents, &QTreeView::expandAll);
    connect(ui->actionCollapse_all, &QAction::triggered, ui->treeContents, &QTreeView::collapseAll);
}

void
ContentsManagerDialog::setupThumbView()
{
    int const sb = ui->thumbView->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int inner_width = ui->thumbView->maximumViewportSize().width() - sb;
    if (ui->thumbView->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0, ui->thumbView)) {
        inner_width -= ui->thumbView->frameWidth() * 2;
    }
    int const delta_x = ui->thumbView->size().width() - inner_width;
    ui->thumbView->setMinimumWidth((int)ceil(m_maxLogicalThumbSize.width() + delta_x));

    ui->thumbView->setBackgroundBrush(palette().color(QPalette::Window));
    m_ptrThumbSequence->attachView(ui->thumbView);

    ui->thumbView->removeEventFilter(this); // make sure installed once
    ui->thumbView->installEventFilter(this);
    if (ui->thumbView->verticalScrollBar()) {
        ui->thumbView->verticalScrollBar()->removeEventFilter(this);
        ui->thumbView->verticalScrollBar()->installEventFilter(this);
    }

    connect(ui->thumbView, &QGraphicsView::rubberBandChanged, this, [=](QRect viewportRect, QPointF /*fromScenePoint*/, QPointF /*toScenePoint*/){
        if (!viewportRect.isNull()) {
            QRectF rect_scene = ui->thumbView->mapToScene(viewportRect).boundingRect();
            const QList<QGraphicsItem*> list = ui->thumbView->scene()->items(rect_scene);
            QSet<PageId> items_to_select;
            for (QGraphicsItem* i: list) {
                PageId page;
                if (m_ptrThumbSequence->findPageByGraphicsItem(i, page)) {
                    items_to_select += page;
                }
            }

            m_ptrThumbSequence->setSelection(items_to_select);
        }
    });
}



QTreeWidgetItem*
readBookmark(DjVuBookmark* bookmark, const QStringList& page_uids)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setFlags(item->flags().setFlag(Qt::ItemIsDropEnabled, true).setFlag(Qt::ItemIsEditable, true));
    QString title = bookmark->title;
    item->setText(0, title.replace("\\\"", "\""));
    item->setToolTip(0, item->text(0));
    const int page_no = page_uids.indexOf(bookmark->url);
    if (page_no != -1) {
        item->setText(1, QString::number(page_no));
    } else {
        item->setText(1, bookmark->url);
    }
    item->setData(1, Qt::UserRole, item->text(1)); // remember initial value

    DjVuBookmark* child = bookmark->child;
    while (child) {
        item->addChild(readBookmark(child, page_uids));
        child = child->next;
    }

    return item;
}

void
ContentsManagerDialog::updateEditors()
{
    if (ui->tabEditors->currentWidget() == ui->pageDjVuSedEditor) {
        ui->edDjVuSed->blockSignals(true);
        ui->edDjVuSed->setPlainText(m_local_bookmarks->toDjVuSed(m_page_uids).join("\n"));
        ui->edDjVuSed->blockSignals(false);
    } else if (ui->tabEditors->currentWidget() == ui->pageTextEditor) {
        ui->edPlainText->blockSignals(true);
        ui->edPlainText->setPlainText(m_local_bookmarks->toPlainText(m_page_uids).join("\n"));
        ui->edPlainText->blockSignals(false);
    }
}

void
ContentsManagerDialog::updateTreeView()
{
    ui->treeContents->clear();
    ui->treeContents->displayHeader();

    DjVuBookmark* top_child = m_local_bookmarks->bookmarks()->child;
    while (top_child) {
        ui->treeContents->addTopLevelItem(readBookmark(top_child, m_page_uids));
        top_child = top_child->next;
    }

    ui->treeContents->setHintMode(m_local_bookmarks->isEmpty());
    ui->treeContents->expandAll();
    ui->treeContents->resizeColumnToContents(1);
}


void
dumpTreeItem(QTreeWidgetItem* const item, QStringList& vals)
{
    if (item) {
        QString url = item->text(1).trimmed();
        QRegularExpression reg("#*([0-9]+)");
        QRegularExpressionMatch match = reg.match(url);
        if (match.hasMatch() && match.capturedLength(1)) {
            // make sure numbers and #xxx (reference to a page number that's too big)
            // are uniformly encoded as #xxx
            url = "#" + match.capturedRef(1);
        }

        vals << QString("(\"%1\" \"%2\"").arg(item->text(0).replace("\"", "\\\""), url);
        for (int i = 0; i < item->childCount(); i++) {
            dumpTreeItem(item->child(i), vals);
        }
        vals << ")";
    }
}

void
ContentsManagerDialog::dumpTreeStructure()
{
    QStringList vals;
    vals << "(bookmarks";

    for (int i = 0; i < ui->treeContents->topLevelItemCount(); i++) {
        dumpTreeItem(ui->treeContents->topLevelItem(i), vals);
    }

    vals << ")";
    m_local_bookmarks->fromDjVuSed(vals, m_page_uids);
}

void
ContentsManagerDialog::dumpAndUpdate()
{
    dumpTreeStructure();
    updateEditors();
}


static int _wheel_val_sum_thumbs = 0;
bool
ContentsManagerDialog::eventFilter(QObject* obj, QEvent* ev)
{

    if (obj == ui->thumbView) {
        if (ev->type() == QEvent::Resize) {
            m_ptrThumbSequence->invalidateAllThumbnails();
        }
    }


    if (!GlobalStaticSettings::m_fixedMaxLogicalThumbSize) {
        if (obj == ui->thumbView || obj == ui->thumbView->verticalScrollBar()) {
            if (ev->type() == QEvent::Wheel) {
                Qt::KeyboardModifiers mods = GlobalStaticSettings::m_hotKeyManager.get(ThumbSizeChange)->
                        sequences().first().m_modifierSequence;
                QWheelEvent* wev = static_cast<QWheelEvent*>(ev);

                if ((wev->modifiers() & mods) == mods) {

                    const QPoint& angleDelta = wev->angleDelta();
                    _wheel_val_sum_thumbs += angleDelta.x() + angleDelta.y();

                    if (abs(_wheel_val_sum_thumbs) >= 30) {
                        wev->accept();

                        const int dy = (_wheel_val_sum_thumbs > 0) ? 10 : -10;
                        _wheel_val_sum_thumbs = 0;

                        m_maxLogicalThumbSize += QSizeF(dy, dy);
                        if (m_maxLogicalThumbSize.width() < 25.) {
                            m_maxLogicalThumbSize.setWidth(25.);
                        }
                        if (m_maxLogicalThumbSize.height() < 16.) {
                            m_maxLogicalThumbSize.setHeight(16.);
                        }

                        if (m_ptrThumbSequence.get()) {
                            m_ptrThumbSequence->setMaxLogicalThumbSize(m_maxLogicalThumbSize);
                        }

                        setupThumbView();
                        resetThumbSequence();
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void
ContentsManagerDialog::resetThumbSequence()
{
    if (m_filter->thumbnailPixmapCache()) {
        IntrusivePtr<CompositeCacheDrivenTask> const task(
                    m_filter->createCompositeCacheDrivenTask()
                    );

        assert(task);

        m_ptrThumbSequence->setThumbnailFactory(
                    IntrusivePtr<ThumbnailFactory>(
                        new ThumbnailFactory(
                            IntrusivePtr<ThumbnailPixmapCache>(m_filter->thumbnailPixmapCache()),
                            m_maxLogicalThumbSize, task
                            )
                        )
                    );
    }




    m_ptrThumbSequence->reset(m_pageSequence, ThumbnailSequence::SelectionAction::RESET_SELECTION);

    if (!m_filter->thumbnailPixmapCache()) {
        // Empty project.
        assert(m_filter->pages()->numImages() == 0);
        m_ptrThumbSequence->setThumbnailFactory(
                    IntrusivePtr<ThumbnailFactory>()
                    );
    }

}

ContentsManagerDialog::~ContentsManagerDialog()
{
    delete ui;
}

void ContentsManagerDialog::on_tabEditors_currentChanged(int index)
{
    if (ui->tabEditors->currentWidget() == ui->pageDjVuSedEditor)
    {
        if (ui->treeContents->hintMode()) {
            ui->treeContents->setHintMode(false);
            m_local_bookmarks->fromDjVuSed(ui->edDjVuSed->placeholderText().split('\n'), m_page_uids);
        }
        updateTreeView();
        updateEditors();
    } else if (ui->tabEditors->currentWidget() == ui->pageTextEditor)
    {
        if (ui->treeContents->hintMode()) {
            ui->treeContents->setHintMode(false);
            m_local_bookmarks->fromPlainText(ui->edPlainText->placeholderText().split('\n'), m_page_uids);
        }
        updateTreeView();
        updateEditors();
    }
}

void ContentsManagerDialog::on_edDjVuSed_textChanged()
{
    QStringList vals = ui->edDjVuSed->toPlainText().split('\n');
    bool has_header = false;
    if (!vals.isEmpty()) {
        QString s = vals.first().trimmed();
        QTextStream ts(&s, QIODevice::ReadOnly);
        QString word;
        ts >> word;
        if (word == "(bookmarks") {
            has_header = true;
        } else if (word == "(") {
            ts >> word;
            if (word == "bookmarks") {
                has_header = true;
            }
        }
    }

    if (!has_header) {
        vals.prepend("(bookmarks");
    }

    m_local_bookmarks->fromDjVuSed(vals, m_page_uids);
    updateTreeView();
}

void
ContentsManagerDialog::on_edPlainText_textChanged()
{
    const QStringList vals = ui->edPlainText->toPlainText().split('\n');
    m_local_bookmarks->fromPlainText(vals, m_page_uids);
    updateTreeView();
}


void
ContentsManagerDialog::on_treeContents_itemSelectionChanged()
{
    QSet<PageId> to_select;
    const QTreeWidgetItem* current_item = ui->treeContents->currentItem();
    const QList<QTreeWidgetItem*> items = ui->treeContents->selectedItems();
    PageId selectionLeader;
    for(QTreeWidgetItem* const & it: items) {
       bool ok;
       const uint page_no = it->text(1).toUInt(&ok) - 1;
       if (ok && page_no < m_page_uids.size()) {
           const PageId p = m_pageSequence.pageAt(page_no).id();
           to_select += p;
           if (it == current_item) {
               selectionLeader = p;
           }
       }
    }

    m_ptrThumbSequence->setSelection(to_select, ThumbnailSequence::RESET_SELECTION);

    if (!selectionLeader.isNull()) {
        m_ptrThumbSequence->setSelection(selectionLeader, ThumbnailSequence::KEEP_SELECTION);
        ui->thumbView->ensureVisible(m_ptrThumbSequence->selectionLeaderSceneRect(), 0, 0);
    }
}

void
ContentsManagerDialog::changePagesNumDlg(bool inc)
{
    if (ui->treeContents->hintMode()) {
        return;
    }

    const QList<QTreeWidgetItem*> items = ui->treeContents->selectedItems();

    if (items.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select entries to change in the contents tree."));
        return;
    }

    bool ok;
    QString txt = inc? tr("Increase page numbers for a selected contents entries by:") :
                       tr("Decrease page numbers for a selected contents entries by:");
    int val = QInputDialog::getInt(this, tr("Change page numbers"), txt, 1, 1/*min*/, 1e6, 1, &ok);
    if (!inc) {
        val *= -1;
    }

    if (ok) {
        ui->treeContents->blockSignals(true);
        QList<QTreeWidgetItem*> unchanged_items;

        QSet<PageId> remove_from_selection;
        QSet<PageId> add_to_selection;

        for (QTreeWidgetItem* const item: items) {
            int page_no = item->text(1).toUInt(&ok) - 1;
            if (ok) {
                if ( page_no < m_page_uids.size() ) {
                    const int new_page_no = page_no + val;
                    if ( new_page_no >= 0 && new_page_no < m_page_uids.size() ) {

                        add_to_selection += m_pageSequence.pageAt(new_page_no).id();
                        remove_from_selection += m_pageSequence.pageAt(page_no).id();
                        item->setText(1, QString::number(new_page_no + 1));
                        item->setData(1, Qt::UserRole, new_page_no + 1);
                    } else {
                        unchanged_items += item;
                    }
                }
            }
        }

        const std::set<PageId> sel_items = m_ptrThumbSequence->selectedItems();
        QSet<PageId> selected_items(sel_items.cbegin(), sel_items.cend());
        selected_items -= remove_from_selection;
        selected_items += add_to_selection;
        m_ptrThumbSequence->setSelection(selected_items, ThumbnailSequence::RESET_SELECTION);
        ui->treeContents->blockSignals(false);

        dumpAndUpdate();

        if (!unchanged_items.isEmpty()) {
            QStringList sl; int idx = 1;
            for (QTreeWidgetItem* item: qAsConst(unchanged_items)) {
                sl += QString("%1. \"%2\"    \"%3\"").arg(idx++).arg(item->text(0), item->text(1));
            }
            QString text = tr("Some entries page numbers were not changed as they "
"would point out of document boundaries. They are listed below. "
"Would you like to clear contents tree selection and highlight only "
"these entries?:\n%1").arg(sl.join('\n'));
            if (QMessageBox::critical(this, tr("Error"), text, QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes ) {
                ui->treeContents->clearSelection();
                for (QTreeWidgetItem* const item: qAsConst(unchanged_items)) {
                    item->setSelected(true);
                }
            }
        }
    }
}

void ContentsManagerDialog::on_btnPageInc_clicked()
{
    changePagesNumDlg(true);
}

void ContentsManagerDialog::on_btnPageDec_clicked()
{
    changePagesNumDlg(false);
}

void ContentsManagerDialog::on_buttonBox_accepted()
{
    accept();
}

void ContentsManagerDialog::on_buttonBox_rejected()
{
    reject();
}

void ContentsManagerDialog::on_actionRemove_triggered()
{
    const QList<QTreeWidgetItem*> items = ui->treeContents->selectedItems();

    if (!items.isEmpty()) {

        if ( QMessageBox::question(this, tr("Deletion"),
                                   tr("You are going to remove %1 entry(ies). Continue?", "", items.size()).arg(items.size())
                                   ) != QMessageBox::StandardButton::Yes ) {
            return;
        }

        for (QTreeWidgetItem* i: items) {
            if (i->parent()) {
                i->parent()->removeChild(i);
            } else {
                ui->treeContents->takeTopLevelItem(ui->treeContents->indexOfTopLevelItem(i));
            }
            delete i;
        }
        dumpAndUpdate();
    }
}

}

