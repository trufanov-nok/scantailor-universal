#include "DjVuBookmarkDispatcher.h"
#include <QStringList>
#include <QTextStream>

namespace publish {

DjVuBookmark::~DjVuBookmark()
{
    DjVuBookmark* next_child = child;
    while (next_child) {
      DjVuBookmark* next = next_child->next;
      delete next_child;
      next_child = next;
    }
}

DjVuBookmark* readBookmark(const QDomNode& node)
{
    if (!node.isElement() || node.nodeName() != "bookmark") {
        return nullptr;
    }
    QDomElement const el(node.toElement());

    DjVuBookmark* res = new DjVuBookmark();
    res->title = el.attribute("title", "");
    res->url = el.attribute("url", "");

    if (el.hasChildNodes()) {
        QDomNode child(el.firstChild());
        for (; !child.isNull(); child = child.nextSibling()) {
                res->addChild(readBookmark(child));
        }
    }
    return res;
}


/*
 *  DjVuBookmarkDispatcher
 */

DjVuBookmarkDispatcher::DjVuBookmarkDispatcher()
{
}

void
DjVuBookmarkDispatcher::clear()
{
    DjVuBookmark* child = m_root.child;
    while (child) {
        DjVuBookmark* next = child->next;
        delete child;
        child = next;
    }
    m_root.child = nullptr;
}

DjVuBookmarkDispatcher::~DjVuBookmarkDispatcher()
{
    clear();
}

DjVuBookmark*
cloneBookmark(DjVuBookmark* source)
{
    if (source) {
        DjVuBookmark* res = new DjVuBookmark;
        res->title = source->title;
        res->url = source->url;

        DjVuBookmark* next_child = source->child;
        while (next_child) {
            res->addChild(cloneBookmark(next_child));
            next_child = next_child->next;
        }
        return res;
    }
    return nullptr;
}

DjVuBookmarkDispatcher::DjVuBookmarkDispatcher(const DjVuBookmarkDispatcher& source)
{
    DjVuBookmark* next_child = source.m_root.child;
    while (next_child) {
        m_root.addChild(cloneBookmark(next_child));
        next_child = next_child->next;
    }
}


DjVuBookmarkDispatcher::DjVuBookmarkDispatcher(QDomElement const& el)
{
    clear();

    if (el.hasChildNodes()) {
        QDomNode node(el.firstChild());
        for (; !node.isNull(); node = node.nextSibling()) {
            m_root.addChild(readBookmark(node));
        }
    }
}

QDomElement saveBookmark(const DjVuBookmark& bookmark, QDomDocument& doc)
{
    QDomElement res(doc.createElement("bookmark"));
    res.setAttribute("title", bookmark.title);
    res.setAttribute("url", bookmark.url);

    DjVuBookmark* child = bookmark.child;
    while (child) {
      DjVuBookmark* next = child->next;
      res.appendChild( saveBookmark(*child, doc) );
      child = next;
    }

    return res;
}

QDomElement
DjVuBookmarkDispatcher::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement root_el(doc.createElement(name));
    DjVuBookmark* child = m_root.child;
    while (child) {
      DjVuBookmark* next = child->next;
      root_el.appendChild( saveBookmark(*child, doc) );
      child = next;
    }
    return root_el;
}


QStringList bookmark2DjVuSed(DjVuBookmark const * bookmark, int ident, const QStringList& page_uids)
{
    QString entry;
    entry.fill(' ', ident*4);

    QString url = bookmark->url;
    int page_no = page_uids.indexOf(url);
    if (page_no != -1) {
        url = "#" + QString::number(page_no);
    }

    entry += QString("( \"%1\"    \"%2\"").arg(bookmark->title, url);
    if (!bookmark->child) {
        return QStringList(entry + " )");
    }

    QStringList res;
    res.append(entry);
    DjVuBookmark* child = bookmark->child;
    while (child) {
        res.append(bookmark2DjVuSed(child, ident+1, page_uids));
        child = child->next;
    }
    entry.fill(' ', ident*4);
    res.append(entry + ")");
    return res;
}

QStringList
DjVuBookmarkDispatcher::toDjVuSed(const QStringList& page_uids) const
{
    QStringList res;
    res.append("(bookmarks");
    DjVuBookmark* child = m_root.child;
    while (child) {
        res += bookmark2DjVuSed(child, 1, page_uids);
        child = child->next;
    }
    res.append(")");
    return res;
}

QString unquote(const QString word)
{
    QString res = word;
    if (res.startsWith("\"") && res.endsWith("\"")) {
        res = res.mid(1, res.size()-2).trimmed();
    }
    return res;
}

QString read_text(QTextStream& ts)
{
    ts.skipWhiteSpace();
    size_t pos = ts.pos();
    QString line;
    ts >> line;
    if (line.startsWith("(") ||
            line.startsWith(")")) {
        ts.seek(pos+1);
        return line.at(0);
    }
    if (line.endsWith(")")) {
        do {
            line = line.left(line.size()-1);
            ts.seek(ts.pos()-1);
        } while ( line.endsWith(")") );
        return unquote(line);
    }

    if (line.startsWith("\"")) {

        if (line.size() > 1 && line.endsWith("\"")) {
            return unquote(line);
        }

        QString word;
        do {
            int cur_pos = ts.pos();
            word = read_text(ts);
            // read_text ignores leading whitespaces
            int spaces = ts.pos() - word.size() - cur_pos;
            if (spaces > 0) {
                line += QString().fill(' ', spaces);
            }
            line += word;
        } while ( !word.isEmpty() && !word.endsWith("\"") );
    }
    return unquote(line);
}

DjVuBookmark*
readBookmark(QTextStream& ts, const QStringList& page_uids)
{
    QString word;
    do {
        word = read_text(ts);
        if (word.isEmpty()) {
            return nullptr;
        }
    } while (word != "(");

    word = read_text(ts); // text is "( )"
    if (word == ")") return nullptr;

    DjVuBookmark* item = new DjVuBookmark;
    item->title = word;

    word = read_text(ts);
    if (word == ")") return item;

    if (word.startsWith("#")) { // substitute page_no with page ref
        bool ok;
        int page_no = word.rightRef(word.size()-1).toUInt(&ok);
        if (ok && page_no < page_uids.size()) {
            word = page_uids[page_no];
        }
    }

    item->url = word;

    DjVuBookmark* child = nullptr;
    do {
        word = read_text(ts);
        if (word == "(") {
            ts.seek(ts.pos()-1);
            child = readBookmark(ts, page_uids);
            item->addChild(child);
        } else {
//            assert(word == ")");
            child = nullptr;
        }
    } while (child);

    return item;
}



void
DjVuBookmarkDispatcher::fromDjVuSed(const QStringList& vals, const QStringList& page_uids)
{
    clear();
    if (vals.isEmpty()) return;

    QString val = vals.join('\n');
    QTextStream ts(&val, QIODevice::ReadOnly);

    if (read_text(ts) == "(" &&
            read_text(ts) == "bookmarks") {
        while (DjVuBookmark* item = readBookmark(ts, page_uids)) {
            m_root.addChild(item);
        }
    }
}


DjVuBookmark* plainText2bookmark(const QString& src, const QStringList &page_uids)
{
    if (src.isEmpty()) {
        return nullptr;
    }

    QStringList vals = src.trimmed().split(' ', Qt::SkipEmptyParts);
    if (vals.isEmpty()) {
        return nullptr;
    }

    DjVuBookmark* res = new DjVuBookmark;

    if (vals.count() > 1) {
        res->url = vals.last();
        QString word = res->url;
        while (word.startsWith("#")) word = word.remove(0,1);
        bool ok;
        uint page_no = word.toUInt(&ok);
        if (ok && page_no < page_uids.size()) {
            res->url = page_uids[page_no];
        }
        vals.removeLast();
    }
    res->title = vals.join(' ').replace("\"", "\\\"");
    return res;
}

void
DjVuBookmarkDispatcher::fromPlainText(const QStringList &vals, const QStringList &page_uids)
{
    clear();
    if (vals.isEmpty()) return;

    QHash<DjVuBookmark*, int> levels;
    levels[&m_root] = -1;
    DjVuBookmark* parent = &m_root;

    for (const QString& s: vals) {
        int cnt = 0;
        while (cnt < s.size() && s[cnt] == ' ') cnt++;
        if (DjVuBookmark* bookmark = plainText2bookmark(s.trimmed(), page_uids)) {
            levels[bookmark] = cnt;
            while (levels[parent] >= cnt) {
                parent = parent->parent;
            }
            parent->addChild(bookmark);
            parent = bookmark;
        }
    }

}


QStringList bookmark2text(DjVuBookmark const * bookmark, int ident, const QStringList& page_uids)
{
    QString entry;
    entry.fill(' ', ident*4);

    QString url = bookmark->url;
    int page_no = page_uids.indexOf(url);
    if (page_no != -1) {
        url = QString::number(page_no);
    }

    QString title = bookmark->title;
    entry += QString("%1    %2").arg(title.replace("\\\"", "\""), url);

    QStringList res;
    res.append(entry);
    DjVuBookmark* child = bookmark->child;
    while (child) {
        res.append(bookmark2text(child, ident+1, page_uids));
        child = child->next;
    }
    return res;
}

QStringList
DjVuBookmarkDispatcher::toPlainText(const QStringList &page_uids) const
{
    QStringList res;
    DjVuBookmark* child = m_root.child;
    while (child) {
        res += bookmark2text(child, 1, page_uids);
        child = child->next;
    }
    return res;
}

} // namespace publish
