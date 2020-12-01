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

#ifndef OUTPUT_CHANGEDPIWIDGET_H_
#define OUTPUT_CHANGEDPIWIDGET_H_

#include "ui_OutputChangeDpiWidget.h"
#include "PageId.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QWidget>
#include <QString>
#include <set>
#include "ApplyToDialog.h"

namespace output
{

class ChangeDpiWidget : public QWidget,
    public ApplyToDialog::Validator,
    private Ui::widgetOutputChangeDpi
{
    Q_OBJECT
public:
    ChangeDpiWidget(QWidget* parent, Dpi const& dpi);

    virtual ~ChangeDpiWidget();

    int dpi() const
    {
        return dpiSelector->currentText().toInt();
    }

    virtual bool validate() override;
private slots:
    void dpiSelectionChanged(int index);

    void dpiEditTextChanged(QString const& text);
private:
    int m_customItemIdx;
    QString m_customDpiString;
};

} // namespace output

#endif
