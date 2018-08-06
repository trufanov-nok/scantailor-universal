#ifndef DJVUENCODER_H
#define DJVUENCODER_H

#include "AppDependency.h"

#include <QObject>
#include <QVector>
#include <memory>

class DjVuEncoder
{
public:
    DjVuEncoder(QObject * componentInstance,  AppDependencies &dependencies);

    QString m_name;
    int priority;
    QStringList supportedInput;
    QString prefferedInput;
    QString supportedOutputMode;
    QString description;
    QString missingAppHint;
    QStringList requiredApps;
    QString filename;
    QString defaultCmd;
    bool isValid;
    std::unique_ptr<QObject> ptrComponentInstance;

    bool operator<(const DjVuEncoder &rhs) const {
        return(this->priority < rhs.priority);
    }

    static bool lessThan(const DjVuEncoder* enc1, const DjVuEncoder* enc2)
    {
        return *enc1 < *enc2;
    }
};

#endif // DJVUENCODER_H
