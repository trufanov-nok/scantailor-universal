/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2021 Alexander Trufanov <trufanovan@gmail.com>

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

#include "EncodingProgressWidget.h"
#include "ui_EncodingProgressWidget.h"
#include "EncodingProgressInfo.h"


namespace publish {

EncodingProgressWidget::EncodingProgressWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::EncodingProgressWidget())
{
    ui->setupUi(this);
    reset(); // should be by default, but just in case
}

EncodingProgressWidget::~EncodingProgressWidget()
{
    delete ui;
}

void EncodingProgressWidget::reset()
{
    ui->pbExport->setEnabled(true);
    ui->pbExport->setValue(0);
    ui->lblStateExport->setEnabled(false);
    ui->swExport->setCurrentIndex(0);

    ui->pbEncodePic->setEnabled(true);
    ui->pbEncodePic->setValue(0);
    ui->lblStateEncodePic->setEnabled(false);
    ui->swEncodePic->setCurrentIndex(0);

    ui->pbEncodeTxt->setEnabled(true);
    ui->pbEncodeTxt->setValue(0);
    ui->lblStateEncodeTxt->setEnabled(false);
    ui->swEncodeTxt->setCurrentIndex(0);

    ui->pbAssemble->setEnabled(true);
    ui->pbAssemble->setValue(0);
    ui->lblStateAssemble->setEnabled(false);
    ui->swAssemble->setCurrentIndex(0);

    ui->pbOCR->setEnabled(true);
    ui->pbOCR->setValue(0);
    ui->lblStateOCR->setEnabled(false);
    ui->swOCR->setCurrentIndex(0);
}

void
EncodingProgressWidget::displayInfo(float progress, int process, int state)
{
    QProgressBar* progress_bar = nullptr;
    QLabel* label1 = nullptr;
    QLabel* label2 = nullptr;
    QStackedWidget* sw = nullptr;

    switch (process) {
    case Export: {
        progress_bar = ui->pbExport;
        label1 = ui->lblExportDesc;
        label2 = ui->lblStateExport;
        sw = ui->swExport;
    } break;
    case EncodePic: {
        progress_bar = ui->pbEncodePic;
        label1 = ui->lblEncodePicDesc;
        label2 = ui->lblStateEncodePic;
        sw = ui->swEncodePic;
    } break;
    case EncodeTxt: {
        progress_bar = ui->pbEncodeTxt;
        label1 = ui->lblEncodeTxtDesc;
        label2 = ui->lblStateEncodeTxt;
        sw = ui->swEncodeTxt;
    } break;
    case Assemble: {
        progress_bar = ui->pbAssemble;
        label1 = ui->lblAssembleDesc;
        label2 = ui->lblStateAssemble;
        sw = ui->swAssemble;
    } break;
    case OCR: {
        progress_bar = ui->pbOCR;
        label1 = ui->lblOCRDesc;
        label2 = ui->lblStateOCR;
        sw = ui->swOCR;
    } break;
    default: return;
    }

    progress_bar->setValue(progress);
    label2->setEnabled(state == EncodingProgressState::Completed);
    label1->setEnabled(state != EncodingProgressState::Skipped);

    sw->setCurrentIndex(state != EncodingProgressState::InProgress ? 0 : 1);
}

}
