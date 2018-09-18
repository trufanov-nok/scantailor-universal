#ifndef DJVUPAGEGENERATOR_H
#define DJVUPAGEGENERATOR_H

#include <QObject>
#include <QThread>
#include <QDebug>

class CommandExecuter: public QThread
{
    Q_OBJECT
public:
    CommandExecuter(const QStringList &commands): m_inputCommands(commands) {}

    void run() override {
        for (const QString& cmd: m_inputCommands) {

            if (isInterruptionRequested()) {
                break;
            }

            qDebug() << cmd.toStdString().c_str() << "\n";
            system(cmd.toStdString().c_str());
        }
    }

private:
    QStringList m_inputCommands;
};



class DjVuPageGenerator : public QObject
{
    Q_OBJECT
public:
    explicit DjVuPageGenerator(QObject *parent = nullptr);
    ~DjVuPageGenerator();

    void setFilename(const QString &file);
    void setComands(const QStringList &commands) { m_commands = commands; }

    QString outputFileName() const { return m_outputFile; }
    QString executedCommands() const { return m_executedCommands; }

    void execute();
    void stop();

private:
    QString updatedCommands() const;
signals:
    void executionComplete();
private:
    QString m_inputFile;
    QStringList m_commands;
    QString m_tempFile;
    QString m_outputFile;
    QString m_executedCommands;
    CommandExecuter* m_commandExecuter;
};

#endif // DJVUPAGEGENERATOR_H
