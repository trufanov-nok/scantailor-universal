#include "DjVuPageGenerator.h"

#include <QDir>
#include <QTemporaryFile>
#include <iostream>

DjVuPageGenerator::DjVuPageGenerator(const QString& file, const QStringList &commands, QObject *parent):
    QObject(parent), m_inputFile(file), m_commands(commands)
{
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

    m_outputFile = fi.completeBaseName()+".djv";
}


/*
 * Replace first occurance of %1 in m_commands
 * with input filename
 * other occurnces of %1 with temp filename
 * and %2 with output filename
 * Empty commands ignored
 */

QStringList
DjVuPageGenerator::updatedCommands() const
{
    QString cmds = m_commands.join('\n');
    int pos = cmds.indexOf("%1");
    if (pos != -1) {
        cmds.replace(pos, 2, m_inputFile);
    }

    cmds = cmds.arg(m_tempFile).arg(m_outputFile);
    return cmds.split('\n', QString::SkipEmptyParts);
}

void
DjVuPageGenerator::execute()
{
    CommandExecuter* ce = new CommandExecuter(updatedCommands());
    connect(ce, &CommandExecuter::executionComplete, this, &DjVuPageGenerator::executionComplete);
    connect(ce, &CommandExecuter::executionComplete, [this, ce]() {
       ce->deleteLater();
       if (!QFile(m_tempFile).remove(m_tempFile)) {
           std::cerr << "Can't emove temporary file: " << m_tempFile.toStdString() << "\n";
       }
    });
}
