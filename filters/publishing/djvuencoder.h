/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2018 Alexander Trufanov <trufanovan@gmail.com>

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

#ifndef DJVUENCODER_H
#define DJVUENCODER_H

#include "AppDependency.h"

#include <QObject>
#include <QVector>
#include <QQmlEngine>
#include <QQmlComponent>
#include <memory>

namespace publishing
{


class DjVuEncoder
{
public:
    DjVuEncoder(QQmlEngine* engine, const QString& fileName, AppDependencies &dependencies);

    QString name;
    int priority;
    QString supportedInput;
    QString prefferedInput;
    QString supportedOutputMode;
    QString description;
    QString missingAppHint;
    QStringList requiredApps;
    QString filename;
    QVariantMap defaultState;
    QString defaultCmd;
    bool isValid;
    std::unique_ptr<QQmlComponent> ptrComponent;
    std::unique_ptr<QObject> ptrInstance;

    bool operator<(const DjVuEncoder &rhs) const {
        return(this->priority < rhs.priority);
    }

    static bool lessThan(const DjVuEncoder* enc1, const DjVuEncoder* enc2)
    {
        return *enc1 < *enc2;
    }
};

}

#endif // DJVUENCODER_H
