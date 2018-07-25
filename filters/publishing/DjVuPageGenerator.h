#ifndef DJVUPAGEGENERATOR_H
#define DJVUPAGEGENERATOR_H

#include <QObject>
#include <QThread>

class CommandExecuter: public QThread
{
    Q_OBJECT
public:
    CommandExecuter(const QStringList &commands): m_commands(commands) {}
    void run() override {
        for (const QString& cmd: m_commands) {
            system(cmd.toStdString().c_str());
        }
        emit executionComplete();
    }
signals:
    void executionComplete();
private:
    QStringList m_commands;
};

class DjVuPageGenerator : public QObject
{
    Q_OBJECT
public:
    explicit DjVuPageGenerator(const QString& file, const QStringList &commands, QObject *parent = nullptr);
    void setComands(const QStringList &commands) {
        m_commands = commands;
        updatedCommands();
    }

    void execute();

private:
    QStringList updatedCommands() const;
signals:
    void executionComplete();
private:
    QString m_inputFile;
    QStringList m_commands;
    QString m_tempFile;
    QString m_outputFile;
};

#endif // DJVUPAGEGENERATOR_H
