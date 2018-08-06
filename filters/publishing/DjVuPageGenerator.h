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
            system(cmd.toStdString().c_str());
            qDebug() << cmd.toStdString().c_str() << "\n";
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

    void setFilename(const QString &file);
    void setComands(const QStringList &commands) { m_commands = commands; }

    QString outputFileName() const { return m_outputFile; }
    QString executedCommands() const { return m_executedCommands; }

    void execute();

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
};

#endif // DJVUPAGEGENERATOR_H
