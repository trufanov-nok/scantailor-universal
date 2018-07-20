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

#include "OptionsWidget.h"
#include "OptionsWidget.moc"
#include "Filter.h"
#include "ApplyToDialog.h"
#include "Settings.h"
#include <QFileInfo>
#include <QQuickItem>
#include <QQmlApplicationEngine>
#include <QDir>

namespace publishing
{

OptionsWidget::OptionsWidget(
        IntrusivePtr<Settings> const& settings,
        PageSelectionAccessor const& page_selection_accessor)
    :	m_ptrSettings(settings),
      m_pageSelectionAccessor(page_selection_accessor)
{
    setupUi(this);
    initEncodersSelector();
    setupQuickWidget(encoderWidget);
//    encoderWidget->setSource(QUrl(QStringLiteral("file:///home/truf/dev/pl/scantailor/filters/publishing/C44Settings.qml")));
    setupQuickWidget(conversionWidget);
    conversionWidget->setSource(QUrl(QStringLiteral("file:///home/truf/dev/pl/scantailor/filters/publishing/TiffPostprocessors.qml")));
    //    conversionWidget->setSource(QUrl(QStringLiteral("file:///tmp/1.qml")));
}

OptionsWidget::~OptionsWidget()
{
    for (int i = 0; i < m_encoders.size(); i++) {
        delete m_encoders[i];
    }
}

void
updateQuickWidgetHeight(QQuickWidget* w)
{
    if (w) {
        QQuickItem* obj = w->rootObject();
        if (obj && obj->height()) {
            w->setFixedHeight(obj->height());
            w->updateGeometry();
        }
    }
}

void
OptionsWidget::preUpdateUI(
        QString const& filename)
{
    m_filename = filename;

}

void
OptionsWidget::postUpdateUI()
{

}

void
OptionsWidget::setupQuickWidget(QQuickWidget* w)
{
    w->setAttribute(Qt::WA_AlwaysStackOnTop);
    w->setAttribute(Qt::WA_TranslucentBackground);
    w->setClearColor(Qt::transparent);
    QObject::connect(w, &QQuickWidget::statusChanged, this, &OptionsWidget::on_statusChanged);
}

#ifdef Q_OS_UNIX
#ifdef Q_OS_OSX
QString _qml_path;
#else
QString _qml_path = "./filters/publishing/";
#endif
#else
QString _qml_path = qApp->applicationDirPath()+"djvu/";
#endif


bool
OptionsWidget::initEncodersSelector()
{
    QStringList sl = _qml_path.split(';');
    foreach (QString d, sl) {
        const QDir dir(QFileInfo(d).absoluteFilePath());
        const QStringList files = dir.entryList(QStringList("*.qml"), QDir::Filter::Files);
        if (!files.isEmpty()) {
            QQmlEngine* engine = new QQmlEngine(this);
            foreach (QString qml_file, files) {
                if (qml_file.contains("ui.qml")) {
                    continue;
                }
                QString ff = dir.absoluteFilePath(qml_file);
                QQmlComponent component(engine, ff);

                //engine.load(ff);
                QObject *item = component.create();  //dynamic_cast<QObject *>(engine.rootObjects().at(0));
                if (item) {
                    DjVuEncoder* encoder = new DjVuEncoder(item);
                    if (encoder->isValid) {
                        encoder->filename = ff;
                        m_encoders.append(encoder);
                    } else {
                        delete encoder;
                    }

                    delete item;
                }
            }
            qSort(m_encoders.begin(), m_encoders.end(), &DjVuEncoder::lessThan);
            delete engine;
        }
    }
    displayEncoders();
}

void
OptionsWidget::displayEncoders()
{
    cbEncodersSelector->clear();

    for (const DjVuEncoder* enc: m_encoders) {
        cbEncodersSelector->addItem(enc->name, enc->filename);
    }
}

void
OptionsWidget::on_statusChanged(const QQuickWidget::Status &arg1)
{
    if (arg1 == QQuickWidget::Ready) {
        QQuickWidget* w = static_cast<QQuickWidget*>(sender());
        updateQuickWidgetHeight(w);
    }
}

} // namespace publishing


void publishing::OptionsWidget::on_cbEncodersSelector_currentIndexChanged(int index)
{
    const QString qml_file = cbEncodersSelector->itemData(index).toString();
    encoderWidget->setResizeMode(QQuickWidget::ResizeMode::SizeViewToRootObject);
    encoderWidget->setSource(qml_file);
    // trick to achieve qml contents actual height but resizable panel's width
    encoderWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
}
