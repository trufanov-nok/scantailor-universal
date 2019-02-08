/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef PUBLISHING_PARAMS_H_
#define PUBLISHING_PARAMS_H_

#include "Dpi.h"
#include "RegenParams.h"
#include "ImageInfo.h"
#include <QVariantMap>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <settings/globalstaticsettings.h>

class QDomDocument;
class QDomElement;

namespace publishing
{

class Params: public RegenParams
{
public:
    Params();
    Params(QDomElement const& el);
    bool isNull() { return m_inputImageInfo.fileName.isEmpty() ||
                m_inputImageInfo.imageColorMode == ImageInfo::ColorMode::Unknown; }

    Dpi const& outputDpi() const { return m_dpi; }
    void setOutputDpi(Dpi const& dpi) { m_dpi = dpi; }
    QString const& encoderId() const { return m_encoderId; }
    void setEncoderId(QString const& val) { m_encoderId = val; }
    QVariantMap const& encoderState() const { return m_encoderState; }
    void setEncoderState(QVariantMap const& val) { m_encoderState = val; }
    QVariantMap const& converterState() const { return m_converterState; }
    void setConverterState(QVariantMap const& val) { m_converterState = val; }
    QString const imageFilename() const { return m_inputImageInfo.fileName; }
    void setImageFilename(QString const& val) { m_inputImageInfo.fileName = val; }
    QString const djvuFilename() const {
        QFileInfo info(m_inputImageInfo.fileName);
        return info.path() + QDir::separator() + GlobalStaticSettings::m_djvu_pages_subfolder + QDir::separator() + info.completeBaseName() + ".djv";
    }
    void setInputImageHash(const QByteArray & val) { m_inputImageInfo.imageHash = val; }
    const QByteArray & inputImageHash() const { return m_inputImageInfo.imageHash; }
    void setInputImageColorMode(ImageInfo::ColorMode val) { m_inputImageInfo.imageColorMode = val; }
    ImageInfo::ColorMode inputImageColorMode() const { return m_inputImageInfo.imageColorMode; }
    void setCommandToExecute(QString const& val) { m_commandToExecute = val; }
    QString const& commandToExecute() const { return m_commandToExecute; }
    void setExecutedCommand(QString const& val) { m_executedCommand = val; }
    QString const& executedCommand() const { return m_executedCommand; }

    void setImageInfo(const ImageInfo& info) { m_inputImageInfo = info; }
    ImageInfo imageInfo() const { return m_inputImageInfo; }

    void setDjVuSize(int size) { m_djvuSize = size; }
    int djvuSize() const { return m_djvuSize; }

    QDomElement toXml(QDomDocument& doc, QString const& name) const;

    ~Params(){}
private:
    Dpi m_dpi;
    QVariantMap m_encoderState;
    QVariantMap m_converterState;
    ImageInfo m_inputImageInfo;
    QString m_commandToExecute;
    QString m_executedCommand;
    QString m_encoderId;
    int m_djvuSize;
};

} // namespace publishing

#endif
