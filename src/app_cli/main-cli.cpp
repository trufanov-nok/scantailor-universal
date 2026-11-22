/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>
    Copyright (C) 2011  Petr Kovar <pejuko@gmail.com>

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

#include <QCoreApplication>
#include "settings/ini_keys.h"
#include <QString>
#include <QStringList>
#include <iostream>
#include <cmath>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "CommandLine.h"
#include "ConsoleBatch.h"
#include "config.h"
#include "ImageLoader.h"
#include "imageproc/OrientationDetector.h"

// A check of many files must not change a project, thus this mode only reads the images and prints the results.
static int
detectOrientation(QString const& list_file)
{
    using imageproc::OrientationDetector;
    using imageproc::OrientationResult;

    QFile file(list_file);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "Cannot read the list file " << list_file.toLocal8Bit().constData() << std::endl;
        return 1;
    }
    QStringList const lines(QString::fromUtf8(file.readAll()).split(QLatin1Char('\n')));
    for (QString const& line : lines) {
        // A multi-page file needs the page number, thus a line can hold the path, a tab and the zero-based page.
        QStringList const parts(line.trimmed().split(QLatin1Char('\t')));
        QString const path(parts.value(0).trimmed());
        if (path.isEmpty()) {
            continue;
        }
        int const page = parts.size() > 1 ? parts[1].trimmed().toInt() : 0;
        OrientationResult result;
        int exif = -1;
        try {
            QImage const image(ImageLoader::load(path, page));
            exif = OrientationDetector::exifRotation(path);
            result = image.isNull() ? OrientationResult(0, 0.0, false, QStringLiteral("cannot load the image"))
                     : OrientationDetector::detect(image, exif);
        } catch (std::exception const& e) {
            result = OrientationResult(0, 0.0, false, QString::fromLocal8Bit(e.what()));
        }
        QJsonObject ev;
        ev["event"] = QStringLiteral("orientation");
        ev["image"] = path;
        ev["page"] = page;
        // An image without a decision gets null, so that a script can tell it from a decision for 0 degrees.
        ev["orientation_detected"] = result.isDecided() ? QJsonValue(result.angle()) : QJsonValue();
        ev["confidence"] = std::round(result.confidence() * 1000.0) / 1000.0;
        ev["reason"] = result.reason();
        if (exif >= 0) {
            ev["exif_rotation"] = exif;
        }
        if (result.shapeAxis() >= 0) {
            ev["shape_axis"] = result.shapeAxis();
        }
        std::cout << QJsonDocument(ev).toJson(QJsonDocument::Compact).constData() << std::endl;
    }
    return 0;
}

int main(int argc, char** argv)
{
    Q_INIT_RESOURCE(core_resources);
    QCoreApplication app(argc, argv);

#ifdef _WIN32
    // Get rid of all references to Qt's installation directory.
    app.setLibraryPaths(QStringList(app.applicationDirPath()));
#endif

    // This information is used by QSettings.
    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setOrganizationDomain(ORGANIZATION_DOMAIN);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // parse command line arguments
    CommandLine cli(app.arguments(), false);
    CommandLine::set(cli);

    if (cli.isError()) {
        cli.printHelp();
        return 1;
    }

    // The parser gives "true" for an option without a value, and that is not a list file.
    if (cli.hasDetectOrientation() && cli.getDetectOrientationList() == "true") {
        std::cerr << "Use --detect-orientation=<list file>." << std::endl;
        return 1;
    }
    if (cli.hasDetectOrientation() && !cli.hasHelp()) {
        return detectOrientation(cli.getDetectOrientationList());
    }

    if (cli.hasHelp() || cli.outputDirectory().isEmpty() || (cli.images().size() == 0 && cli.projectFile().isEmpty())) {
        cli.printHelp();
        return 0;
    }

    std::unique_ptr<ConsoleBatch> cbatch;

    try {
        if (!cli.projectFile().isEmpty()) {
            cbatch.reset(new ConsoleBatch(cli.projectFile()));
        } else {
            cbatch.reset(new ConsoleBatch(cli.images(), cli.outputDirectory(), cli.getLayoutDirection()));
        }
        cbatch->process();
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    if (cli.hasOutputProject()) {
        cbatch->saveProject(cli.outputProjectFile());
    }
}
