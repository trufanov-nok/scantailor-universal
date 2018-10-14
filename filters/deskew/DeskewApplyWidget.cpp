#include "DeskewApplyWidget.h"

DeskewApplyWidget::DeskewApplyWidget(AutoManualMode mode, double angle, int orientationFix, QWidget *parent ) : QWidget(parent),
    m_mode(mode),
    m_angle(angle),
    m_orientationFix(orientationFix)
{
    setupUi(this);

    if (m_mode == AutoManualMode::MODE_MANUAL) {
        modeRB->setText(tr("Fixed deskew angle %1°").arg(m_angle));
    } else modeRB->setText(tr("Automatic deskew detection"));

    orientationRB->setText(tr("Adjust page orientation by %1°").arg((orientationFix)));
}
