#ifndef MANAGEDISPLAYPREFERENCESDIALOG_H
#define MANAGEDISPLAYPREFERENCESDIALOG_H

#include "ui_ManageDisplayPreferencesDialog.h"
#include <QDialog>

namespace publish {

class DisplayPreferences;

class ManageDisplayPreferencesDialog : public QDialog, private Ui::ManageDisplayPreferencesDialog
{
    Q_OBJECT

public:
    explicit ManageDisplayPreferencesDialog(const DisplayPreferences& val, QWidget *parent = nullptr);
    DisplayPreferences preferences();

private slots:
    void on_btnSelectColor_clicked();

private:
    Ui::ManageDisplayPreferencesDialog *ui;
    QString m_background_clr;
};

}



#endif // MANAGEDISPLAYPREFERENCESDIALOG_H
