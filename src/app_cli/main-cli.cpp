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
#include <QFileInfo>
#include <QJsonObject>
#include <iostream>

#include "CommandLine.h"
#include "ConsoleBatch.h"
#include "config.h"

static void
printRunError(QString const& message)
{
    QJsonObject ev;
    ev["event"] = QStringLiteral("run_end");
    ev["status"] = QStringLiteral("error");
    ev["error"] = message;
    ConsoleBatch::printJsonLine(ev);
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

    // The parser can write text, thus send that text to stderr before parsing when stdout must carry only JSON lines.
    bool const json = app.arguments().contains(QStringLiteral("--json-progress"));
    if (json) {
        std::cout.rdbuf(std::cerr.rdbuf());
    }

    // parse command line arguments
    CommandLine cli(app.arguments(), false);
    CommandLine::set(cli);

    if (cli.isError()) {
        cli.printHelp();
        if (json) {
            printRunError(QStringLiteral("invalid command line"));
        }
        return 1;
    }

    if (cli.hasHelp() || cli.outputDirectory().isEmpty() || (cli.images().size() == 0 && cli.projectFile().isEmpty())) {
        cli.printHelp();
        if (json && !cli.hasHelp()) {
            printRunError(QStringLiteral("no input images or no output directory"));
        }
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
        if (cli.hasOutputProject() && !cbatch->saveProject(cli.outputProjectFile()) && json) {
            printRunError(QStringLiteral("cannot write the project file"));
            return 1;
        }
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        if (json) {
            printRunError(QString::fromLocal8Bit(e.what()));
        }
        exit(1);
    }

    if (json) {
        QJsonObject ev;
        ev["event"] = QStringLiteral("run_end");
        ev["status"] = QStringLiteral("ok");
        ev["pages"] = cbatch->pageCount();
        ev["page_errors"] = cbatch->pageErrorCount();
        if (cli.hasOutputProject()) {
            ev["project"] = QFileInfo(cli.outputProjectFile()).absoluteFilePath();
        }
        ConsoleBatch::printJsonLine(ev);
    }
}
