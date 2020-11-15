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

#include "ChangeDpiWidget.h"

#include "PageSelectionAccessor.h"
#include "Dpi.h"
#include <QButtonGroup>
#include <QVariant>
#include <QIntValidator>
#include <QMessageBox>
#include <QLineEdit>
#include <QDebug>
#include <algorithm>
#include "settings/ini_keys.h"

namespace output
{

ChangeDpiWidget::ChangeDpiWidget(QWidget* parent, Dpi const& dpi):  QWidget(parent)
{
    setupUi(this);

    dpiSelector->setValidator(new QIntValidator(dpiSelector));

    QStringList common_dpis = QSettings().value(_key_dpi_change_list, _key_dpi_change_list_def).toString().split(',');

    int const requested_dpi = std::max(dpi.horizontal(), dpi.vertical());
    m_customDpiString = QString::number(requested_dpi);

    int selected_index = -1;
    for (QString const& cdpi : qAsConst(common_dpis)) {
        if (cdpi.trimmed().toInt() == requested_dpi) {
            selected_index = dpiSelector->count();
        }
        dpiSelector->addItem(cdpi, cdpi);
    }

    m_customItemIdx = dpiSelector->count();
    dpiSelector->addItem(tr("Custom"), m_customDpiString);

    if (selected_index != -1) {
        dpiSelector->setCurrentIndex(selected_index);
    } else {
        dpiSelector->setCurrentIndex(m_customItemIdx);
        dpiSelector->setEditable(true);
        dpiSelector->lineEdit()->setText(m_customDpiString);
        // It looks like we need to set a new validator
        // every time we make the combo box editable.
        dpiSelector->setValidator(
            new QIntValidator(0, 9999, dpiSelector)
        );
    }

    connect(
        dpiSelector, SIGNAL(activated(int)),
        this, SLOT(dpiSelectionChanged(int))
    );
    connect(
        dpiSelector, SIGNAL(editTextChanged(QString)),
        this, SLOT(dpiEditTextChanged(QString))
    );
}

ChangeDpiWidget::~ChangeDpiWidget()
{
}

void
ChangeDpiWidget::dpiSelectionChanged(int const index)
{
    dpiSelector->setEditable(index == m_customItemIdx);
    if (index == m_customItemIdx) {
        dpiSelector->setEditText(m_customDpiString);
        dpiSelector->lineEdit()->selectAll();
        // It looks like we need to set a new validator
        // every time we make the combo box editable.
        dpiSelector->setValidator(
            new QIntValidator(0, 9999, dpiSelector)
        );
    }
}

void
ChangeDpiWidget::dpiEditTextChanged(QString const& text)
{
    if (dpiSelector->currentIndex() == m_customItemIdx) {
        m_customDpiString = text;
    }
}

bool
ChangeDpiWidget::validate()
{
    QString const dpi_str(dpiSelector->currentText());
    if (dpi_str.isEmpty()) {
        QMessageBox::warning(
            this, tr("Error"),
            tr("DPI is not set.")
        );
        return false;
    }

    int const dpi = dpi_str.toInt();
    if (dpi < 72) {
        QMessageBox::warning(
            this, tr("Error"),
            tr("DPI is too low!")
        );
        return false;
    }

    if (dpi > 1200) {
        QMessageBox::warning(
            this, tr("Error"),
            tr("DPI is too high!")
        );
        return false;
    }

    // We assume the default connection from accepted() to accept()
    // was removed.
    return true;
}

} // namespace output
