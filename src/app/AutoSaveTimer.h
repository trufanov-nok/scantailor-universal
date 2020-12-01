#ifndef QAUTOSAVETIMER_H
#define QAUTOSAVETIMER_H

#include <QTimer>
#include <QString>
#include <functional>
#include "ProjectPages.h"

class MainWindow;

class QAutoSaveTimer : public QTimer
{
    Q_OBJECT
public:
    QAutoSaveTimer(MainWindow* obj);
public slots:
    void autoSaveProject();
private:
    bool copyFileTo(const QString& sFromPath, const QString& sToPath);
    const QString getAutoSaveInputDir();
private:
    MainWindow* m_MW;
};

#endif // QAUTOSAVETIMER_H
