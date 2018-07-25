#ifndef DJVUENCODER_H
#define DJVUENCODER_H

#include "AppDependency.h"

#include <QObject>
#include <QVector>

#ifdef Q_OS_UNIX
#ifdef Q_OS_OSX
QString _qml_path;
#else
static QString _qml_path = "./filters/publishing/";
#endif
#else
QString _qml_path = qApp->applicationDirPath()+"djvu/";
#endif


#ifdef Q_OS_UNIX
#ifdef Q_OS_OSX
QString _platform = "macos";
#else
static QString _platform = "linux";
#endif
#else
QString _platform = "windows";
#endif

class DjVuEncoder
{
public:
    DjVuEncoder(QObject * obj, AppDependencies &dependencies);

    QString m_name;
    int priority;
    QStringList supportedInput;
    QString prefferedInput;
    QString supportedOutputMode;
    QString description;
    QString missingAppHint;
    QStringList requiredApps;
    QString filename;
    bool isValid;

    bool operator<(const DjVuEncoder &rhs) const {
        return(this->priority < rhs.priority);
    }

    static bool lessThan(const DjVuEncoder* enc1, const DjVuEncoder* enc2)
    {
        return *enc1 < *enc2;
    }
};

#endif // DJVUENCODER_H
