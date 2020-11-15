#include "StartBatchProcessingDialog.h"

#include "ui_StartBatchProcessingDialog.h"

#include "settings/ini_keys.h"

StartBatchProcessingDialog::StartBatchProcessingDialog(QWidget* parent, bool isAllPages) :
    QDialog(parent),
    ui(new Ui::StartBatchProcessingDialog)
{
    ui->setupUi(this);

    ui->allPages->setChecked(isAllPages);
    ui->fromSelected->setChecked(!isAllPages);
}

StartBatchProcessingDialog::~StartBatchProcessingDialog()
{
    delete ui;
}

bool StartBatchProcessingDialog::isAllPagesChecked() const
{
    return ui->allPages->isChecked();
}

bool StartBatchProcessingDialog::isRememberChoiceChecked() const
{
    return ui->rememberChoice->isChecked();
}
