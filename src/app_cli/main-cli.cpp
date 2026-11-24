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
#include <QDir>
#include "settings/ini_keys.h"
#include <QString>
#include <QStringList>
#include <iostream>

#include "CommandLine.h"
#include "ConsoleBatch.h"
#include "config.h"

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

    // A project has its own output directory, so only a run on images needs one on the command line.
    bool const no_output = cli.outputDirectory().isEmpty() && cli.projectFile().isEmpty();
    if (cli.hasHelp() || no_output || (cli.images().size() == 0 && cli.projectFile().isEmpty())) {
        cli.printHelp();
        return 0;
    }

    // The check comes before the processing, so that a missing file name does not lose the result of a long run.
    if (cli.isProjectSaveRequested() && !cli.hasOutputProject()) {
        std::cerr << "No project file to save to: give a name, or use a project as the input." << std::endl;
        return 1;
    }

    // The parser only reads the options, so the CLI makes the directory of --output-dir here.
    if (!cli.outputDirectory().isEmpty() && !QDir().mkpath(cli.outputDirectory())) {
        std::cerr << "Cannot make the output directory: " << cli.outputDirectory().toStdString() << std::endl;
        return 1;
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

    if (cli.hasOutputProject() && !cbatch->saveProject(cli.outputProjectFile())) {
        return 1;
    }
    return 0;
}
