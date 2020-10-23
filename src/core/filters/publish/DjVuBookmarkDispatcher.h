#ifndef DJVUBOOKMARKDISPATCHER_H
#define DJVUBOOKMARKDISPATCHER_H

#include "RefCountable.h"

#include <QString>
#include <QDomElement>

namespace publish {

struct DjVuBookmark
{
public:
    DjVuBookmark(DjVuBookmark* p = nullptr)
    {
        parent = p;
        next = child = nullptr;
    }

    ~DjVuBookmark();

    void addChild(DjVuBookmark* new_child) {
        if (new_child) {
            new_child->parent = this;
            if (DjVuBookmark* last_child = child) {
                while (last_child->next) {
                    last_child = last_child->next;
                }
                last_child->next = new_child;
            } else {
                child = new_child;
            }
        }
    }

    QString title;
    QString url;

    DjVuBookmark* parent;
    DjVuBookmark* next;
    DjVuBookmark* child;
};


class DjVuBookmarkDispatcher: public RefCountable
{
public:
    DjVuBookmarkDispatcher();
    DjVuBookmarkDispatcher(const DjVuBookmarkDispatcher&);
    ~DjVuBookmarkDispatcher();
    void clear();
    DjVuBookmarkDispatcher(QDomElement const& el);
    QDomElement toXml(QDomDocument& doc, QString const& name) const;

    void fromDjVuSed(const QStringList &vals, const QStringList &page_uids);
    QStringList toDjVuSed(const QStringList &page_uids) const;
    void fromPlainText(const QStringList &vals, const QStringList &page_uids);
    QStringList toPlainText(const QStringList &page_uids) const;

    bool isEmpty() const { return !m_root.child; }

    DjVuBookmark const * bookmarks() const { return &m_root; }
private:
    DjVuBookmark m_root;
};

} // namespace publish

#endif // DJVUBOOKMARKDISPATCHER_H
