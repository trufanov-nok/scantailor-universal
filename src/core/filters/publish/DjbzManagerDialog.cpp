#include "DjbzManagerDialog.h"
#include "ui_DjbzManagerDialog.h"

#include <QScrollBar>
#include <QSettings>
#include <QGraphicsItem>
#include <QMimeData>
#include <cmath>
#include <QShortcut>
#include <QAbstractButton>
#include <QInputDialog>
#include <QMessageBox>

#include "settings/globalstaticsettings.h"
#include "ThumbnailSequence.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailFactory.h"

#include "ProjectPages.h"
#include "Filter.h"
#include "Settings.h"
#include "PageId.h"

#include <qdebug.h>

namespace publish {


QDjbzTreeWidget::QDjbzTreeWidget(QWidget *parent): QTreeWidget(parent)
{
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
}

QDjbzTreeWidget::~QDjbzTreeWidget()
{
}

QStringList
QDjbzTreeWidget::mimeTypes() const
{
    return QStringList() << PageId::mimeType;
}

void QDjbzTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event && reinterpret_cast<QGraphicsScene*>( event->source())) {
        if (QTreeWidgetItem* item = itemAt(event->pos())) {
            if (item != currentItem()) {
                qDebug() << item->text(0);
                event->accept();
            } else {
                event->ignore();
            }
        }

    }

    QTreeWidget::dragMoveEvent(event);

}

void QDjbzTreeWidget::dropEvent(QDropEvent *event)
{
    if (const QMimeData* mime = event->mimeData()) {
        QByteArray data = mime->data(PageId::mimeType);

        int pages_cnt;
        int read = sizeof(pages_cnt);
        assert(data.size() > read);
        memcpy(&pages_cnt, data.data(), read);

        QSet<PageId> pages;
        PageId pageId;

        for (int i = 0; i < pages_cnt; i++) {
            read += PageId::fromByteArray(QByteArray::fromRawData( data.data() + read, data.size() - read ), pageId);
            pages += pageId;
        }

        assert(!pages.isEmpty());

        if (QTreeWidgetItem* item = itemAt(event->pos())) {
            if (item != currentItem()) {
                emit movePages(pages, item);
            }
        }

        event->acceptProposedAction();
        return;
    }

    QTreeWidget::dropEvent(event);
}


DjbzManagerDialog::DjbzManagerDialog(Filter* filter, PageId const& page_id, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DjbzManagerDialog),
    m_filter(filter),
    m_PageId(page_id),
    m_lockedIcon(":/icons/document-encrypted.png")
{
    m_DjbzDispatcher_copy.reset(new DjbzDispatcher(*m_filter->settings()->djbzDispatcher()));

    QSettings settings;
    m_maxLogicalThumbSize = settings.value(_key_thumbnails_max_thumb_size, _key_thumbnails_max_thumb_size_def).toSizeF();
    m_maxLogicalThumbSize /= 3;
    m_ptrThumbSequence.reset(new ThumbnailSequence(m_maxLogicalThumbSize));
    m_ptrThumbSequence->setDraggingEnabled(true);
    m_ptrThumbSequence->setIsDjbzView(true);

    ui->setupUi(this);
    setupThumbView();
    ui->tabDjbz->setCurrentIndex(0);
    displayTree(m_PageId);
    ui->treeDjbz->addAction(ui->actionAddDict);

    connect (ui->treeDjbz, &QDjbzTreeWidget::movePages, this, [=](const QSet<PageId>& pages, QTreeWidgetItem* item_to){
        if (item_to) {
            const QString& dict_id = item_to->data(0, Qt::UserRole).toString();
            for (const PageId& p: pages) {
                m_DjbzDispatcher_copy->moveToDjbz(p, dict_id);
            }
            updateItemText(ui->treeDjbz->currentItem());
            updateItemText(item_to);
            // refresh page thumbnails view
            emit ui->treeDjbz->currentItemChanged(ui->treeDjbz->currentItem(), nullptr);
        }
    });

    connect(ui->cbLock, &QCheckBox::toggled, this, [=](bool val) {
        ui->sbMaxPages->setEnabled(!val);
        ui->treeDjbz->currentItem()->setIcon(0, !val ? QIcon() : m_lockedIcon);
    });

    on_treeDjbz_currentItemChanged(ui->treeDjbz->currentItem(), nullptr);
    m_ptrThumbSequence->setSelection(m_PageId);
    ui->thumbView->ensureVisible(m_ptrThumbSequence->selectionLeaderSceneRect(), 0, 0);
}

void
DjbzManagerDialog::setupThumbView()
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


    new QShortcut(
        QKeySequence(Qt::ControlModifier + Qt::Key_A),
        this,
        SLOT(on_actionSelectAll_triggered()),
        SLOT(on_actionSelectAll_triggered()),
        Qt::WindowShortcut);

}

void
DjbzManagerDialog::addTreeItem(const QString& dict_id, bool is_selected)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeDjbz);
    item->setData(0, Qt::UserRole, dict_id);
    item->setFlags(item->flags().setFlag(Qt::ItemIsDropEnabled, true));
    updateItemText(item);
    if (is_selected) {
        item->setSelected(true);
        ui->treeDjbz->setCurrentItem(item);
    }
}

void
DjbzManagerDialog::displayTree(const PageId& page_id)
{
    ui->treeDjbz->clear();    

    const QString djbz_to_select = m_DjbzDispatcher_copy->dictionaryIdByPage(page_id);
    const QStringList djbzs = m_DjbzDispatcher_copy->listAllDjbz();

    for (const QString& dict_id: djbzs) {
        addTreeItem(dict_id, dict_id == djbz_to_select);
    }

//    ui->treeDjbz->expandAll();
}

static int _wheel_val_sum_thumbs = 0;
bool
DjbzManagerDialog::eventFilter(QObject* obj, QEvent* ev)
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
DjbzManagerDialog::resetThumbSequence()
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

DjbzManagerDialog::~DjbzManagerDialog()
{
    delete ui;
}

bool page_cmp(const PageInfo &a, const PageInfo &b)
{
    const PageId& id_a = a.id();
    const PageId& id_b = b.id();
    if (id_a.imageId().filePath() != id_b.imageId().filePath()) {
        return id_a.imageId().filePath() < id_b.imageId().filePath();
    }
    if (id_a.imageId().page() != id_b.imageId().page()) {
        return id_a.imageId().page() < id_b.imageId().page();
    }

    return id_a.subPage() < id_b.subPage();
}

void
DjbzManagerDialog::on_treeDjbz_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem */*previous*/)
{
    if (!current) {
        return;
    }

    const QString dict_id = current->data(0, Qt::UserRole).toString();
    const DjbzDict* dict = m_DjbzDispatcher_copy->dictionary(dict_id);
    const bool hide_settings = dict->isDummy();
    ui->gbDisablableSettings->setEnabled(!hide_settings);


    const DjbzParams& djbz_params = dict->params();

    ui->cbAveraging->setChecked(djbz_params.useAveraging());
    ui->cbErosion->setChecked(djbz_params.useErosion());
    ui->cbPrototypes->setChecked(djbz_params.usePrototypes());
    ui->sbAggression->setValue(djbz_params.agression());
    ui->cbExtension->setCurrentIndex(ui->cbExtension->findText(djbz_params.extension()));
    ui->sbMaxPages->setMinimum(dict->pageCount());
    ui->sbMaxPages->setValue(dict->maxPages());
    ui->cbLock->setChecked(dict->isLocked());

    const PageSequence all_pages = m_filter->pages()->toPageSequence(PageView::PAGE_VIEW);
    m_pageSequence.clear();
    for (const PageId& p: dict->pages()) {
        m_pageSequence.append( all_pages.pageAt(p) );
    }

    std::sort(m_pageSequence.begin(), m_pageSequence.end(), page_cmp);

    resetThumbSequence();
}

void DjbzManagerDialog::on_cbPrototypes_clicked(bool checked)
{
    const QString dict_id = ui->treeDjbz->currentItem()->data(0, Qt::UserRole).toString();
    m_DjbzDispatcher_copy->dictionary(dict_id)->paramsRef().setUsePrototypes(checked);
}

void DjbzManagerDialog::on_cbAveraging_clicked(bool checked)
{
    const QString dict_id = ui->treeDjbz->currentItem()->data(0, Qt::UserRole).toString();
    m_DjbzDispatcher_copy->dictionary(dict_id)->paramsRef().setUseAveraging(checked);
}

void DjbzManagerDialog::on_spinBox_valueChanged(int arg1)
{
    const QString dict_id = ui->treeDjbz->currentItem()->data(0, Qt::UserRole).toString();
    m_DjbzDispatcher_copy->dictionary(dict_id)->paramsRef().setAgression(arg1);
}

void DjbzManagerDialog::on_cbErosion_clicked(bool checked)
{
    const QString dict_id = ui->treeDjbz->currentItem()->data(0, Qt::UserRole).toString();
    m_DjbzDispatcher_copy->dictionary(dict_id)->paramsRef().setUseErosion(checked);
}

void DjbzManagerDialog::on_cbExtension_currentIndexChanged(const QString &arg1)
{
    const QString dict_id = ui->treeDjbz->currentItem()->data(0, Qt::UserRole).toString();
    m_DjbzDispatcher_copy->dictionary(dict_id)->paramsRef().setExtension(arg1);
}

void DjbzManagerDialog::on_cbLock_clicked(bool checked)
{
    const QString dict_id = ui->treeDjbz->currentItem()->data(0, Qt::UserRole).toString();
    m_DjbzDispatcher_copy->dictionary(dict_id)->setLocked(checked);
}

void DjbzManagerDialog::on_sbAggression_valueChanged(int arg1)
{
    const QString dict_id = ui->treeDjbz->currentItem()->data(0, Qt::UserRole).toString();
    m_DjbzDispatcher_copy->dictionary(dict_id)->paramsRef().setAgression(arg1);
}

void DjbzManagerDialog::updateItemText(QTreeWidgetItem* item)
{
    if (item) {
        const QString dict_id = item->data(0, Qt::UserRole).toString();
        const DjbzDict * dict = m_DjbzDispatcher_copy->dictionary(dict_id);
        const QString title = !dict->isDummy() ?
                    QObject::tr("%1.djbz (%2/%3)").arg(dict_id).arg(dict->pageCount()).arg(dict->maxPages()) :
                    QObject::tr("%1 (%2)").arg(dict_id).arg(dict->pageCount());
        item->setText(0, title);
        item->setIcon(0, dict->isLocked() ? m_lockedIcon : QIcon() );
    }
}

void DjbzManagerDialog::on_sbMaxPages_valueChanged(int arg1)
{
    const QString dict_id = ui->treeDjbz->currentItem()->data(0, Qt::UserRole).toString();
    DjbzDict* dict = m_DjbzDispatcher_copy->dictionary(dict_id);
    if (!dict->isDummy()) {
        dict->setMaxPages(arg1);
    }

    updateItemText(ui->treeDjbz->currentItem());
}

void DjbzManagerDialog::on_actionSelectAll_triggered()
{
    QSet<PageId> all_pages;
    for (const PageId& p: m_pageSequence.asPageIdSet()) {
        all_pages += p;
    }
    m_ptrThumbSequence->setSelection(all_pages);
}

void DjbzManagerDialog::on_buttonBox_accepted()
{
    m_filter->settings()->setDjbzDispatcher( new DjbzDispatcher(*m_DjbzDispatcher_copy.get()) );
    accept();
}

void DjbzManagerDialog::on_buttonBox_rejected()
{
    reject();
}

void DjbzManagerDialog::on_buttonBox_clicked(QAbstractButton *button)
{

    if (ui->buttonBox->buttonRole(button) ==
            QDialogButtonBox::ButtonRole::ResetRole) {

        bool ok;
        const int pages_per_dict = QInputDialog::getInt(this,
                                                        tr("Reset pages assignment to shared dictionaries"),
                                                        tr("A new set of shared dictionaries will be \n"
                                                           "created and all pages will be assigned to \n"
                                                           "them according to the max number of pages \n"
                                                           "per dictionary. Pages per dictionary:"),
                                                        GlobalStaticSettings::m_djvu_pages_per_djbz, 1, 99999, 1, &ok);
        if (ok) {
            int old_val = GlobalStaticSettings::m_djvu_pages_per_djbz;
            GlobalStaticSettings::m_djvu_pages_per_djbz = pages_per_dict;
            m_filter->reassignAllPagesExceptLocked(*m_DjbzDispatcher_copy.get());
            GlobalStaticSettings::m_djvu_pages_per_djbz = old_val;

            displayTree(m_PageId);
            ui->treeDjbz->setCurrentIndex(ui->treeDjbz->rootIndex());
            emit ui->treeDjbz->currentItemChanged(ui->treeDjbz->currentItem(), nullptr);
        }
    }
}

void DjbzManagerDialog::on_actionAddDict_triggered()
{
    QString id;
    do {
        id = QInputDialog::getText(this, tr("Enter id"), tr("Enter id for a new shared dictionary:"));
        if (id.isEmpty()) {
            return;
        }
        if (m_DjbzDispatcher_copy->dictionary(id)) {
            QMessageBox::warning(this, tr("Error"), tr("Dictionary with id \"%1\" already exists! Please enter unique id.").arg(id));
        } else {
            assert( m_DjbzDispatcher_copy->addNewDictionary(id) );
            addTreeItem(id, true);
            return;
        }
    } while (!id.isEmpty());
}

}


