#include "QMLLoader.h"
#include <QDir>
#include <QQmlComponent>
#include <QQmlContext>


void
QMLPluginBase::checkDependencies(const AppDependency& dep)
{
    AppDependencies tmp;
    tmp[dep.app_name] = dep;
    checkDependencies(tmp);
}

void
QMLPluginBase::checkDependencies(const QStringList& apps)
{
    AppDependencies tmp;
    for (const QString& s: apps) {
        tmp[s] = m_dependencies[s];
    }
    checkDependencies(tmp);
}

void
QMLPluginBase::checkDependencies(const AppDependencies& deps)
{
    DependencyChecker* checker =  new DependencyChecker(deps);
    // we'll delete self at completion
    QObject::connect(checker, &DependencyChecker::dependenciesChecked, [checker](){checker->deleteLater();});

    QObject::connect(checker, &DependencyChecker::dependencyStateChanged, this, [this](const QString app_name, const AppDependencyState state) {
        m_dependencies[app_name].state = state;
        emit dependencyStateChanged();
    }, Qt::QueuedConnection); // QueuedConnection is required to create MissingAppDeps widgets in Ui thread.

    checker->start();
}


#ifdef Q_OS_UNIX
#ifdef Q_OS_OSX
QString _qml_path;
#else
static QString _qml_path = "./filters/publishing/";
#endif
#else
QString _qml_path = qApp->applicationDirPath()+"djvu/";
#endif

TiffConverters::TiffConverters(QQmlEngine* engine, QObject *parent):
    QMLPluginBase(engine, parent), m_isValid(false)
{
    const QString _convertor_qml_("TiffPostprocessors.qml");

    QStringList sl = _qml_path.split(';');
    for (const QString& d : sl) {
        const QDir dir(QFileInfo(d).absoluteFilePath());
        QString qml_file = dir.absoluteFilePath(_convertor_qml_);
        if (QFileInfo(qml_file).exists()) {

            m_component.reset(new QQmlComponent(m_engine, qml_file));
            if (QObject* instance = m_component->create()) {
                QuickWidgetAccessHelper ch(instance, _convertor_qml_);
                bool res = ch.init();
                if (res) {
                    res = ch.readAppDependencies(m_dependencies, &m_convertorRequiredApps);
                    if (res) {
                        res = ch.getCommand(m_defaultCmd);
                        if (res) {
                            m_instance.reset(instance);
                            m_access_helper.reset(new QuickWidgetAccessHelper(instance, _convertor_qml_));
                            checkDependencies(m_dependencies);
                        }
                        m_isValid = true;
                    }
                }
            }
        }
    }
}


void
DJVUEncoders::clearEncoders()
{
    for (int i = 0; i < m_encoders.size(); i++) {
        delete m_encoders[i];
    }
    m_encoders.clear();
}

DJVUEncoders::~DJVUEncoders()
{
    clearEncoders();
}

DJVUEncoders::DJVUEncoders(QQmlEngine* engine, QObject *parent):
    QMLPluginBase(engine, parent),
    m_currentEncoder(0)
{
    clearEncoders();

    const QStringList sl = _qml_path.split(';');
    for (const QString& d : sl) {
        const QDir dir(QFileInfo(d).absoluteFilePath());
        const QStringList files = dir.entryList(QStringList("*.qml"), QDir::Filter::Files);
        if (!files.isEmpty()) {

            for (const QString& qml_file : files) {
                if (qml_file.contains("ui.qml")) {
                    continue;
                }
                QString ff = dir.absoluteFilePath(qml_file);
                QQmlComponent component(m_engine, ff);

                if (QObject* instance = component.create()) {
                    DjVuEncoder* encoder = new DjVuEncoder(instance, m_dependencies);
                    if (encoder->isValid) {
                        encoder->filename = ff;
                        m_encoders.append(encoder);
                    } else {
                        delete encoder;
                    }
                }
            }
            qSort(m_encoders.begin(), m_encoders.end(), &DjVuEncoder::lessThan);
        }
    }
}

QMLLoader::QMLLoader(QObject *parent): QObject (parent)
{

    m_engine.reset(new QQmlEngine());
    if (QQmlContext* cntx = m_engine->rootContext()) {
        cntx->setContextProperty("mainApp", parent);
    }

    m_converters.reset(new TiffConverters(m_engine.get(), this));
    connect(m_converters.get(), &TiffConverters::dependencyStateChanged, this, &QMLLoader::dependencyStateChanged);
    m_encoders.reset(new DJVUEncoders(m_engine.get(), this));
    connect(m_encoders.get(), &TiffConverters::dependencyStateChanged, this, &QMLLoader::dependencyStateChanged);
}



