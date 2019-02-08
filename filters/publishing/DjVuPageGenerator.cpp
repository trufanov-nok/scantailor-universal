#include "DjVuPageGenerator.h"

#include <QDir>
#include <QTemporaryFile>
#include <iostream>

DjVuPageGenerator::DjVuPageGenerator(QObject *parent): QObject(parent)
{
}

void
DjVuPageGenerator::setInputImageFile(const QString &file, const QByteArray &hash) {
    m_inputFile = file;
    m_inputFileHash = hash;

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
    for (ExecuterHndl ex: m_executers) {
        ex.first->requestInterruption();
    }
}

bool
DjVuPageGenerator::execute()
{
    if (m_outputFile.isEmpty()) {
        QFileInfo fi(m_inputFile);
        m_outputFile = fi.path() + "/" + fi.completeBaseName()+".djv";
    }

    QDir dir(QFileInfo(m_outputFile).absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        return false;
    }

    if (!m_inputFile.isEmpty() && !m_commands.isEmpty()) {

        m_executedCommands = updatedCommands();

        QSingleShotExec* commandExecuter = new QSingleShotExec(m_executedCommands.split('\n', QString::SkipEmptyParts));
        m_executers += ExecuterHndl(commandExecuter, m_tempFile);

        connect(commandExecuter, &QSingleShotExec::finished, this, &DjVuPageGenerator::executed, Qt::QueuedConnection);
        commandExecuter->start();
    } else {
        emit executionComplete(false);
    }

    return true;
}

void
DjVuPageGenerator::executed()
{
    qDebug() << QThread::currentThreadId();
    QSingleShotExec* sender = static_cast<QSingleShotExec*>(this->sender());
    QString tempFile;
    for (ExecuterHndl ex: m_executers) {
        if (ex.first == sender) {
            tempFile = ex.second;
            m_executers -= ex;
            break;
        }
    }

    emit this->executionComplete(!sender->isInterruptionRequested());


    if (!tempFile.isEmpty()) {
        QFile f(tempFile);
        if (f.exists() && !f.remove()) {
            std::cerr << "Can't remove temporary file: " << tempFile.toStdString() << "\n";
        }
    }

    sender->deleteLater();
}

DjVuPageGenerator::~DjVuPageGenerator()
{
    stop();
}
