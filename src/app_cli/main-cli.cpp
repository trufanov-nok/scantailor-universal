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

    if (cli.hasHelp() || cli.outputDirectory().isEmpty() || (cli.images().size() == 0 && cli.projectFile().isEmpty())) {
        cli.printHelp();
        return 0;
    }

    // The parser gives "true" for an option without a value, and that is not a directory.
    if (cli.renderPreviewGiven()
            && (!cli.hasRenderPreview() || cli.getRenderPreviewDirectory() == "true")) {
        std::cerr << "Use --render-preview=<directory>." << std::endl;
        return 1;
    }
    if (cli.hasRenderPreviewSize() && cli.getRenderPreviewSize() <= 0) {
        std::cerr << "Use --render-preview-size=<pixels> with a positive number." << std::endl;
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

    // Previews come before the save, so that the saved project holds what they read, but a preview error does not stop the save.
    bool preview_failed = false;
    if (cli.hasRenderPreview()) {
        try {
            preview_failed = cbatch->renderPreviews(cli.getRenderPreviewDirectory(), cli.getRenderPreviewSize()) > 0;
        } catch (std::exception const& e) {
            std::cerr << e.what() << std::endl;
            preview_failed = true;
        }
    }

    if (cli.hasOutputProject()) {
        cbatch->saveProject(cli.outputProjectFile());
    }

    if (preview_failed) {
        return 1;
    }
}
