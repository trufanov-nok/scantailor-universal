#include "DjVuPageGenerator.h"

#include <QDir>
#include <QTemporaryFile>
#include <iostream>

DjVuPageGenerator::DjVuPageGenerator(QObject *parent): QObject(parent), m_commandExecuter(nullptr)
{
}

void
DjVuPageGenerator::setFilename(const QString &file) {
    m_inputFile = file;
    QFileInfo fi(m_inputFile);

    m_tempFile = QDir::tempPath();
    QTemporaryFile tmp_file;
    tmp_file.setFileTemplate( QDir::tempPath() + "/" + fi.baseName() + "XXXXXX");
    if (tmp_file.open()) {
        m_tempFile = tmp_file.fileName();
        tmp_file.close();
    } else {
        std::cerr << "Can't create temporary file in " << QDir::tempPath().toStdString() << "\n";
    }

    m_outputFile = fi.path() + "/" + fi.completeBaseName()+".djv";
}

/*
 * Replace first occurance of %1 in m_commands
 * with input filename
 * other occurnces of %1 with temp filename
 * and %2 with output filename
 * Empty commands ignored
 */

QString
DjVuPageGenerator::updatedCommands() const
{
    QString cmds = m_commands.join('\n');
    int pos = cmds.indexOf("%1");
    if (pos != -1) {
        cmds.replace(pos, 2, m_inputFile);
    }

    do {
        pos = cmds.indexOf("%1");
        if (pos != -1) {
            cmds.replace(pos, 2, m_tempFile);
        }
    } while (pos != -1);

    return cmds.arg(m_outputFile);
}

void
DjVuPageGenerator::stop()
{
    if (m_commandExecuter) {

        if (m_commandExecuter->isRunning()) {
            disconnect(m_commandExecuter, &CommandExecuter::finished, this, &DjVuPageGenerator::executionComplete);
            m_commandExecuter->requestInterruption();
        }

        m_commandExecuter->deleteLater();
        m_commandExecuter = nullptr;
    }
}

void
DjVuPageGenerator::execute()
{
    stop();

    if (!m_inputFile.isEmpty() && !m_commands.isEmpty()) {
        m_executedCommands = updatedCommands();
        m_commandExecuter = new CommandExecuter(m_executedCommands.split('\n', QString::SkipEmptyParts));

        connect(m_commandExecuter, &CommandExecuter::finished, this, &DjVuPageGenerator::executionComplete);

        connect(m_commandExecuter, &CommandExecuter::finished, [this]() {
            QFile f(m_tempFile);
            if (f.exists() && !f.remove()) {
                std::cerr << "Can't remove temporary file: " << m_tempFile.toStdString() << "\n";
            }
            m_commandExecuter->deleteLater();
            m_commandExecuter = nullptr;
        });

        m_commandExecuter->start();
    } else {
        emit executionComplete();
    }
}

DjVuPageGenerator::~DjVuPageGenerator()
{
    stop();
}
