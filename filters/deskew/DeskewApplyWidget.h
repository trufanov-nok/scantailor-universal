#ifndef DESKEWAPPLYWIDGET_H
#define DESKEWAPPLYWIDGET_H

#include "ui_DeskewApplyWidget.h"
#include "AutoManualMode.h"
#include <QWidget>

class DeskewApplyWidget : public QWidget, private Ui::deskewApplyWidget
{
    Q_OBJECT
public:
    explicit DeskewApplyWidget(AutoManualMode mode, double angle, int orientationFix, QWidget *parent = nullptr);
    bool isOrientationOptionSelected() const { return orientationRB->isChecked(); }
    bool isModeOptionSelected() const { return modeRB->isChecked(); }
    bool isAllOptionSelected() const { return allRB->isChecked(); }
signals:

public slots:

private:
    AutoManualMode m_mode;
    double m_angle;
    int m_orientationFix;
};

#endif // DESKEWAPPLYWIDGET_H
