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

#include "RelinkingDialog.h"

#include "RelinkingSortingModel.h"
#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <assert.h>

RelinkingDialog::RelinkingDialog(QString const& project_file_path, QWidget* parent)
    :   QDialog(parent)
    ,   m_pSortingModel(new RelinkingSortingModel)
    ,   m_projectFileDir(QFileInfo(project_file_path).path())
    ,   m_pSummaryLabel(new QLabel(this))
    ,   m_pResultLabel(new QLabel(this))
    ,   m_pOnlyMissing(new QCheckBox(tr("Show only missing"), this))
    ,   m_pFindButton(new QPushButton(tr("Find in folder..."), this))
{
    ui.setupUi(this);
    resize(780, 500);

    // How to use it: the list alone does not tell.
    QLabel* help = new QLabel(
        tr("Red: not found, green: found. "
           "<b>Find in folder...</b> looks up every missing file by name in a folder you choose "
           "(and its subfolders) &mdash; the quick way when the book folder was moved. "
           "Or select a line and click a part of its path below to replace that part "
           "for every file under it."), this
    );
    help->setWordWrap(true);
    help->setTextFormat(Qt::RichText);

    QHBoxLayout* bar = new QHBoxLayout;
    bar->addWidget(m_pSummaryLabel, 1);
    bar->addWidget(m_pOnlyMissing);
    bar->addWidget(m_pFindButton);
    m_pFindButton->setToolTip(
        tr("Choose the folder that now holds the scans: missing files are found there by name.")
    );

    ui.verticalLayout->insertWidget(0, help);
    ui.verticalLayout->insertLayout(1, bar);
    m_pResultLabel->setWordWrap(true);
    m_pResultLabel->setVisible(false);
    ui.verticalLayout->insertWidget(2, m_pResultLabel);

    m_pSortingModel->setSourceModel(&m_model);
    ui.listView->setModel(m_pSortingModel);
    ui.listView->setTextElideMode(Qt::ElideMiddle);
    ui.errorLabel->setVisible(false);
    ui.undoButton->setVisible(false);
    ui.undoButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(
        ui.listView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        SLOT(selectionChanged(QItemSelection,QItemSelection))
    );

    connect(
        ui.pathVisualization, SIGNAL(clicked(QString,QString,int)),
        SLOT(pathButtonClicked(QString,QString,int))
    );

    connect(ui.undoButton, SIGNAL(clicked()), SLOT(undoButtonClicked()));
    connect(m_pFindButton, SIGNAL(clicked()), SLOT(findInFolderClicked()));
    connect(m_pOnlyMissing, SIGNAL(toggled(bool)), SLOT(onlyMissingToggled(bool)));
    connect(&m_model, &QAbstractItemModel::dataChanged, this, &RelinkingDialog::updateSummary);
    connect(&m_model, &QAbstractItemModel::rowsInserted, this, &RelinkingDialog::updateSummary);
    updateSummary();

    disconnect(ui.buttonBox, SIGNAL(accepted()));
    connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
}

void
RelinkingDialog::selectionChanged(QItemSelection const& selected, QItemSelection const& deselected)
{
    if (selected.isEmpty()) {
        ui.pathVisualization->clear();
        ui.pathVisualization->setVisible(false);
    } else {
        ui.undoButton->setVisible(false);

        QModelIndex const index(selected.front().topLeft());
        QString const path(index.data(m_model.UncommittedPathRole).toString());
        int const type = index.data(m_model.TypeRole).toInt();
        ui.pathVisualization->setPath(RelinkablePath(path, (RelinkablePath::Type)type), /*clickable=*/true);
        ui.pathVisualization->setVisible(true);

        if (ui.errorLabel->isVisible()) {
            m_model.rollbackChanges();
        } else {
            m_model.commitChanges();
        }
    }

    ui.errorLabel->setVisible(false);

}

void
RelinkingDialog::pathButtonClicked(
    QString const& prefix_path, QString const& suffix_path, int const type)
{
    assert(!prefix_path.endsWith(QChar('/')) && !prefix_path.endsWith(QChar('\\')));
    assert(!suffix_path.startsWith(QChar('/')) && !suffix_path.startsWith(QChar('\\')));

    QString replacement_path;

    if (type == RelinkablePath::File) {
        QDir const dir(QFileInfo(prefix_path).dir());
        replacement_path = QFileDialog::getOpenFileName(
                               this, tr("Substitution File for %1").arg(QDir::toNativeSeparators(prefix_path)),
                               dir.exists() ? dir.path() : m_projectFileDir,
                               QString(), 0, QFileDialog::DontUseNativeDialog
                           );
    } else {
        QDir const dir(prefix_path);
        replacement_path = QFileDialog::getExistingDirectory(
                               this, tr("Substitution Directory for %1").arg(QDir::toNativeSeparators(prefix_path)),
                               dir.exists() ? prefix_path : m_projectFileDir,
                               QFileDialog::DontUseNativeDialog
                           );
    }
    // So what's wrong with native dialogs? The one for directory selection won't show files
    // at all (if you ask it to, the non-native dialog will appear), which is inconvenient
    // in this situation. So, if one of them has to be non-native, the other was made
    // non-native as well, for consistency reasons.

    replacement_path = RelinkablePath::normalize(replacement_path);

    if (replacement_path.isEmpty()) {
        return;
    }

    if (prefix_path == replacement_path) {
        return;
    }

    QString new_path(replacement_path);
    new_path += QChar('/');
    new_path += suffix_path;

    m_model.replacePrefix(prefix_path, replacement_path, (RelinkablePath::Type)type);

    if (m_model.checkForMerges()) {
        ui.errorLabel->setText(
            tr("This change would merge several files into one.")
        );
        ui.errorLabel->setVisible(true);
        ui.pathVisualization->clear();
        ui.pathVisualization->setVisible(false);
    } else {
        ui.pathVisualization->setPath(RelinkablePath(new_path, (RelinkablePath::Type)type), /*clickable=*/false);
        ui.pathVisualization->setVisible(true);
    }

    ui.undoButton->setVisible(true);
    ui.listView->update();
}

void
RelinkingDialog::undoButtonClicked()
{
    m_pResultLabel->setVisible(false);
    m_model.rollbackChanges(); // Has to go before selectionChanged()
    selectionChanged(ui.listView->selectionModel()->selection(), QItemSelection());
}

void
RelinkingDialog::commitChanges()
{
    m_model.commitChanges();
    accept();
}

void
RelinkingDialog::findInFolderClicked()
{
    QString const folder = QFileDialog::getExistingDirectory(
                               this, tr("Folder with the scans"), m_projectFileDir,
                               QFileDialog::DontUseNativeDialog
                           );
    if (folder.isEmpty()) {
        return;
    }

    // Commit the earlier change so that Undo reverts only this search, but drop a change with a merge error.
    if (ui.errorLabel->isVisible()) {
        m_model.rollbackChanges();
    } else {
        m_model.commitChanges();
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    RelinkingModel::FolderSearchResult const res(m_model.relinkMissingFromFolder(folder));
    QApplication::restoreOverrideCursor();

    m_pResultLabel->setVisible(false);
    if (m_model.checkForMerges()) {
        m_model.rollbackChanges();
        ui.errorLabel->setText(tr("This change would merge several files into one."));
        ui.errorLabel->setVisible(true);
    } else {
        ui.errorLabel->setVisible(false);
        QString msg(tr("Found %1 missing path(s) in %2.")
                    .arg(res.relinked).arg(QDir::toNativeSeparators(folder)));
        if (res.ambiguous) {
            msg += QChar(' ') + tr("%1 had several matches and were left as they were.").arg(res.ambiguous);
        }
        if (res.notFound) {
            msg += QChar(' ') + tr("%1 not found there.").arg(res.notFound);
        }
        m_pResultLabel->setText(msg);
        m_pResultLabel->setVisible(true);
    }
    ui.undoButton->setVisible(res.relinked > 0 && !ui.errorLabel->isVisible());
    ui.pathVisualization->clear();
    ui.pathVisualization->setVisible(false);
    ui.listView->clearSelection();
    ui.listView->update();
    updateSummary();
}

void
RelinkingDialog::onlyMissingToggled(bool const checked)
{
    m_pSortingModel->setOnlyMissing(checked);
}

void
RelinkingDialog::updateSummary()
{
    int const total = m_model.rowCount();
    int const missing = m_model.countWithStatus(RelinkingModel::Missing);
    int const pending = m_model.countWithStatus(RelinkingModel::StatusUpdatePending);

    QString text;
    if (pending > 0) {
        text = tr("Checking %1 of %2 paths...").arg(pending).arg(total);
    } else if (missing > 0) {
        text = tr("<b>%1 of %2</b> paths not found").arg(missing).arg(total);
    } else {
        text = tr("All %1 paths found").arg(total);
    }
    m_pSummaryLabel->setText(text);
}
