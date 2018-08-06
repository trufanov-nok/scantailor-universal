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
#include <QVariantMap>
#include <QFileInfo>
#include <QImage>

class QDomDocument;
class QDomElement;

namespace publishing
{

struct ImageInfo {

    enum ColorMode {
        BlackAndWhite,
        Grayscale,
        Color
    };

    ImageInfo():fileSize(0), imageHash(0), imageColorMode(BlackAndWhite) {}
    ImageInfo(const QString& filename, const QImage& image);

    QString fileName;
    qint64 fileSize;
    qint64 imageHash;
    ColorMode imageColorMode;
};

class Params: public RegenParams
{
public:
	Params();
	Params(QDomElement const& el);
    bool inNull() { return m_inputImageInfo.fileName.isEmpty(); }
	
	Dpi const& outputDpi() const { return m_dpi; }
    void setOutputDpi(Dpi const& dpi) { m_dpi = dpi; }
    QString const& encoderName() const { return m_encoderName; }
    void encoderName(QString const& val) { m_encoderName = val; }
    QVariantMap const& encoderState() const { return m_encoderState; }
    void setEncoderState(QVariantMap const& val) { m_encoderState = val; }
    QVariantMap const& converterState() const { return m_converterState; }
    void setConverterState(QVariantMap const& val) { m_converterState = val; }
    QString const inputFilename() const { return m_inputImageInfo.fileName; }
    void setInputFilename(QString const& val) { m_inputImageInfo.fileName = val; }
    QString const outputFilename() const { return  QFileInfo(m_inputImageInfo.fileName).completeBaseName() + ".djv"; }
    void setInputFileSize(uint val) { m_inputImageInfo.fileSize = val; }
    qint64 inputFileSize() const { return m_inputImageInfo.fileSize; }
    void setInputImageHash(qint64 val) { m_inputImageInfo.imageHash = val; }
    qint64 inputImageHash() const { return m_inputImageInfo.imageHash; }
    void setInputImageColorMode(ImageInfo::ColorMode val) { m_inputImageInfo.imageColorMode = val; }
    ImageInfo::ColorMode inputImageColorMode() const { return m_inputImageInfo.imageColorMode; }
    void setExecutedCommand(QString const& val) { m_executedCommand = val; }
    QString const& executedCommand() const { return m_executedCommand; }

    void setImageInfo(const ImageInfo& info) { m_inputImageInfo = info; }
    ImageInfo getImageInfo() const { return m_inputImageInfo; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;

    ~Params(){}
private:
    Dpi m_dpi;
    QVariantMap m_encoderState;
    QVariantMap m_converterState;
    ImageInfo m_inputImageInfo;
    QString m_executedCommand;
    QString m_encoderName;
};

} // namespace publishing

#endif
