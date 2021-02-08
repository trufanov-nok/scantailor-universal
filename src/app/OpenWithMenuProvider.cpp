#include "OpenWithMenuProvider.h"
#include <QFile>
#include <QDirIterator>
#include <QStringList>

inline QString get_val(QString const& str)
{
    return str.right(str.length() - str.indexOf('=') - 1).trimmed().remove('\n');
}

QMenu* OpenWithMenuProvider::getOpenWithMenu(const QString& mime_type)
{
    QMenu* res = new QMenu();

    const QString linux_mime_cache("/usr/share/applications/mimeinfo.cache");

    if (QFile::exists(linux_mime_cache)) {
        QStringList apps;
        QFile mc(linux_mime_cache);
        if (mc.open(QFile::ReadOnly)) {
            while (!mc.atEnd()) {
                QString s = mc.readLine().trimmed();
                if (s.startsWith(mime_type + "=")) {
                    apps = get_val(s).split(';', QString::SkipEmptyParts);
                    break;
                }
            }
            mc.close();
        }

        for (const QString& app : qAsConst(apps)) {
            QFile f("/usr/share/applications/" + app);
            if (f.open(QFile::ReadOnly)) {
                QString title, icon, exec;
                bool follow = false;
                while (!f.atEnd()) {
                    QString s = f.readLine().trimmed();
                    if (s.startsWith("[")) {
                        if (!follow && s.startsWith("[Desktop Entry]")) {
                            follow = true;
                            continue;
                        } else {
                            if (follow) {
                                break;
                            }
                        }
                    }

                    if (follow) {
                        if (s.startsWith("Name=")) {
                            title = get_val(s);
                        } else if (s.startsWith("Exec=")) {
                            exec = get_val(s);
                        } else if (s.startsWith("Icon=")) {
                            icon = get_val(s);
                        }
                    }
                }

                f.close();

                if (!exec.isEmpty()) {
                    if (title.isEmpty()) {
                        title = exec;
                    }

                    QStringList icons;
                    QDirIterator it("/usr/share/icons/hicolor/", QStringList() << icon + ".png", QDir::Files, QDirIterator::Subdirectories);
                    while (it.hasNext()) {
                        QString s = it.next();
                        if (s.contains("24x24")) {
                            icons.prepend(s);
                        } else {
                            icons.append(s);
                        }

                    }

                    QAction* act = !icons.isEmpty() ? res->addAction(QIcon(icons.first()), title)
                                                    : res->addAction(title);
                    act->setData(exec);
                }

            }

        }
    }

    if (res->actions().empty()) {
        delete res;
        return nullptr;
    }
    return res;
}
