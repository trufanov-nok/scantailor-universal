#include "MissingApplicationsWidget.h"
#include "ui_MissingApplicationsWidget.h"

#include <QFileDialog>

MissingApplicationsWidget::MissingApplicationsWidget(const QString &app, const QString &hint, QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::MissingApplicationsWidget),
    m_application(app)
{
    ui->setupUi(this);
    setTitle(title().arg(m_application));

    ui->edPathToExecutable->setPlaceholderText(ui->edPathToExecutable->placeholderText().arg(m_application));
    ui->lblHint->setText(hint);
}

MissingApplicationsWidget::~MissingApplicationsWidget()
{
    delete ui;
}

void MissingApplicationsWidget::on_btnBrowseExecutable_clicked()
{
  QString exe; QString mask;
#ifdef Q_OS_WIN
  exe = ".exe";
  mask = ".*";
#endif
    QString filter = tr("%1 (%1%2);;All files (*%3)").arg(m_application).arg(exe).arg(mask);

    QString const executable(
                QFileDialog::getOpenFileName(
                    this, tr("Locate executable %1").arg(m_application), "",
                    filter
                    )
                );
    if (!executable.isEmpty()) {
        emit newPathToExecutable(m_application, executable);
        ui->edPathToExecutable->setText(executable);
    }
}

void MissingApplicationsWidget::on_edPathToExecutable_textEdited(const QString &new_path)
{
    QString executable = new_path;
    if (!executable.isEmpty()) {
        QFileInfo fi(new_path);
        if (fi.exists() && fi.isDir()) {
            executable = fi.dir().absoluteFilePath(m_application);
        }
        emit newPathToExecutable(m_application, executable);
    }
}
