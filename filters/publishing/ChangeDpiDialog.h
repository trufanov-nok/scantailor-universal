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

#ifndef PUBLISHING_CHANGEDPIDIALOG_H_
#define PUBLISHING_CHANGEDPIDIALOG_H_

#include "ui_PublishingChangeDpiDialog.h"
#include "PageId.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QWidget>
#include <QString>
#include <set>

namespace publishing
{

class ChangeDpiDialog : public QDialog,
        private Ui::dialogPublishingChangeDpi
{
	Q_OBJECT
public:
    ChangeDpiDialog(QWidget* parent, Dpi const& dpi);
	
    virtual ~ChangeDpiDialog() override;

    int dpi() const { return dpiSelector->currentText().toInt(); }

    bool validate();
private slots:
	void dpiSelectionChanged(int index);
	
	void dpiEditTextChanged(QString const& text);
    void on_buttonBox_clicked(QAbstractButton *button);

private:
	int m_customItemIdx;
	QString m_customDpiString;
};

} // namespace publishing

#endif
