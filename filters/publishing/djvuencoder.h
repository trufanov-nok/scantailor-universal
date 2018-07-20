#ifndef DJVUENCODER_H
#define DJVUENCODER_H

#include <QObject>
#include <QVector>
#include <QThread>
#include <QProcess>
#include <memory>

struct EncoderDependency {
    QString app_name;
    QString check_cmd;
    QStringList required_params;
};

class DjVuEncoder;
class DependencyChecker: public QThread
{
    Q_OBJECT
public:
    DependencyChecker(DjVuEncoder* enc);

    void run() override {
        bool res = checkDependencies();
        emit dependenciesChecked(res);
    }

private:
    bool checkDependencies();
signals:
    void dependenciesChecked(bool);
private:
    QString m_name;
    QVector<EncoderDependency> m_dependencies;
    QProcess m_proc;
};

class DjVuEncoder: public QObject
{
    Q_OBJECT
public:
    DjVuEncoder(QObject* obj);
    ~DjVuEncoder();

    QString name;
    int priority;
    QStringList supportedInput;
    QString prefferedInput;
    QString supportedOutputMode;
    QString description;
    QString missingAppHint;
    QVector<EncoderDependency> dependencies;
    QString filename;
    bool isMissing;
    bool isValid;

    bool operator<(const DjVuEncoder &rhs) const {
        return(this->priority < rhs.priority);
    }
    bool checkDependencies();

    static bool lessThan(const DjVuEncoder* enc1, const DjVuEncoder* enc2)
    {
        return *enc1 < *enc2;
    }

signals:
    void dependenciesChecked(bool);
private:
    std::unique_ptr<DependencyChecker> m_deps_checker;
};

#endif // DJVUENCODER_H
