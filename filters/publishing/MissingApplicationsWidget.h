#ifndef MISSINGAPPLICATIONSWIDGET_H
#define MISSINGAPPLICATIONSWIDGET_H

#include <QGroupBox>

namespace Ui {
class MissingApplicationsWidget;
}

class MissingApplicationsWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit MissingApplicationsWidget(QString const& app, const QString &hint, QWidget *parent = 0);
    ~MissingApplicationsWidget();

    QString getApplication() const { return m_application; }

signals:
    void newPathToExecutable(QString app, QString filename);

private slots:
    void on_btnBrowseExecutable_clicked();

    void on_edPathToExecutable_textEdited(const QString &new_path);

private:
    Ui::MissingApplicationsWidget *ui;
    QString m_application;
};

#endif // MISSINGAPPLICATIONSWIDGET_H
