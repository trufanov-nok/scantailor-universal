/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FileNameDisambiguator.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include <QString>
#include <QFileInfo>
#include <QDomDocument>
#include <QDomElement>
#include <QMutex>
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#endif

using namespace boost::multi_index;

class FileNameDisambiguator::Impl
{
public:
    Impl();

    Impl(QDomElement const& disambiguator_el,
         boost::function<QString(QString const&)> const& file_path_unpacker);

    QDomElement toXml(QDomDocument& doc, QString const& name,
                      boost::function<QString(QString const&)> const& file_path_packer) const;

    bool getLabelAndOverridenFilename(QString const& file_path, int page, int& label, QString& overriden_filename) const;

    int registerFile(QString const& file_path, int page_num = 0, const QString* overriden_filename = nullptr);

    void performRelinking(AbstractRelinker const& relinker);
private:
    class ItemsByFilePathTag;
    class ItemsByFileNameLabelTag;
    class UnorderedItemsTag;

    struct Item {
        QString filePath;
        QString fileName;
        int label;
        int page;
        QString overridenFileName;

        Item(QString const& file_path, int lbl, int page_num = 0, QString const& overriden_filename = QString());

        Item(QString const& file_path, QString const& file_name, int lbl, int page_num = 0, QString const& overriden_filename = QString());
    };

    typedef multi_index_container <
    Item,
    indexed_by <
    ordered_unique <
    tag<ItemsByFilePathTag>,
//              member<Item, QString, &Item::filePath>
    composite_key <
    Item,
    member<Item, QString, &Item::filePath>,
    member<Item, int, &Item::page>
    >
    >,
    ordered_unique <
    tag<ItemsByFileNameLabelTag>,
    composite_key <
    Item,
    member<Item, QString, &Item::fileName>,
    member<Item, int, &Item::page>,
    member<Item, int, &Item::label>
    >
    >,
    sequenced<tag<UnorderedItemsTag> >
    >
    > Container;

    typedef Container::index<ItemsByFilePathTag>::type ItemsByFilePath;
    typedef Container::index<ItemsByFileNameLabelTag>::type ItemsByFileNameLabel;
    typedef Container::index<UnorderedItemsTag>::type UnorderedItems;

    mutable QMutex m_mutex;
    Container m_items;
    ItemsByFilePath& m_itemsByFilePath;
    ItemsByFileNameLabel& m_itemsByFileNameLabel;
    UnorderedItems& m_unorderedItems;
};

/*====================== FileNameDisambiguator =========================*/

FileNameDisambiguator::FileNameDisambiguator()
    :   m_ptrImpl(new Impl)
{
}

FileNameDisambiguator::FileNameDisambiguator(
    QDomElement const& disambiguator_el)
    :   m_ptrImpl(new Impl(disambiguator_el, [](QString const& path) -> QString { return path; }))
{
}

FileNameDisambiguator::FileNameDisambiguator(
    QDomElement const& disambiguator_el,
    boost::function<QString(QString const&)> const& file_path_unpacker)
    :   m_ptrImpl(new Impl(disambiguator_el, file_path_unpacker))
{
}

QDomElement
FileNameDisambiguator::toXml(QDomDocument& doc, QString const& name) const
{
    return m_ptrImpl->toXml(doc, name, [](QString const& path) -> QString { return path; });
}

QDomElement
FileNameDisambiguator::toXml(
    QDomDocument& doc, QString const& name,
    boost::function<QString(QString const&)> const& file_path_packer) const
{
    return m_ptrImpl->toXml(doc, name, file_path_packer);
}

bool
FileNameDisambiguator::getLabelAndOverridenFilename(QString const& file_path, int page, int& label, QString& overriden_filename) const
{
    return m_ptrImpl->getLabelAndOverridenFilename(file_path, page, label, overriden_filename);
}

int
FileNameDisambiguator::registerFile(QString const& file_path, int page_num, QString const* overriden_filename)
{
    return m_ptrImpl->registerFile(file_path, page_num, overriden_filename);
}

void
FileNameDisambiguator::performRelinking(AbstractRelinker const& relinker)
{
    m_ptrImpl->performRelinking(relinker);
}

/*==================== FileNameDisambiguator::Impl ====================*/

FileNameDisambiguator::Impl::Impl()
    :   m_items(),
        m_itemsByFilePath(m_items.get<ItemsByFilePathTag>()),
        m_itemsByFileNameLabel(m_items.get<ItemsByFileNameLabelTag>()),
        m_unorderedItems(m_items.get<UnorderedItemsTag>())
{
}

FileNameDisambiguator::Impl::Impl(
    QDomElement const& disambiguator_el,
    boost::function<QString(QString const&)> const& file_path_unpacker)
    :   m_items(),
        m_itemsByFilePath(m_items.get<ItemsByFilePathTag>()),
        m_itemsByFileNameLabel(m_items.get<ItemsByFileNameLabelTag>()),
        m_unorderedItems(m_items.get<UnorderedItemsTag>())
{
    QDomNode node(disambiguator_el.firstChild());
    for (; !node.isNull(); node = node.nextSibling()) {
        if (!node.isElement()) {
            continue;
        }
        if (node.nodeName() != "mapping") {
            continue;
        }
        QDomElement const file_el(node.toElement());

        QString const file_path_shorthand(file_el.attribute("file"));
        QString const file_path = file_path_unpacker(file_path_shorthand);
        if (file_path.isEmpty()) {
            // Unresolved shorthand - skipping this record.
            continue;
        }

        int const label = file_el.attribute("label").toInt();
        int const page = file_el.attribute("page").toInt();
        QString const overriden_filename(file_el.attribute("overriden"));
        m_items.insert(Item(file_path, label, page, overriden_filename));
    }
}

QDomElement
FileNameDisambiguator::Impl::toXml(
    QDomDocument& doc, QString const& name,
    boost::function<QString(QString const&)> const& file_path_packer) const
{
    QMutexLocker const locker(&m_mutex);

    QDomElement el(doc.createElement(name));

    for (Item const& item : m_unorderedItems) {
        QString const file_path_shorthand = file_path_packer(item.filePath);
        if (file_path_shorthand.isEmpty()) {
            // Unrepresentable file path - skipping this record.
            continue;
        }

        QDomElement file_el(doc.createElement("mapping"));
        file_el.setAttribute("file", file_path_shorthand);
        file_el.setAttribute("label", item.label);
        file_el.setAttribute("page", item.page);
        if (!item.overridenFileName.isEmpty()) {
            file_el.setAttribute("overriden", item.overridenFileName);
        }
        el.appendChild(file_el);
    }

    return el;
}

bool
FileNameDisambiguator::Impl::getLabelAndOverridenFilename(QString const& file_path, int page, int& label, QString& overriden_filename) const
{
    QMutexLocker const locker(&m_mutex);

    ItemsByFilePath::iterator const fp_it(m_itemsByFilePath.find(boost::make_tuple(file_path, page)));
    if (fp_it != m_itemsByFilePath.end()) {
        label = fp_it->label;
        overriden_filename = fp_it->overridenFileName;
        return true;
    }

    return false;
}

int
FileNameDisambiguator::Impl::registerFile(QString const& file_path, int page_num, QString const* overriden_filename)
{
    QMutexLocker const locker(&m_mutex);

    ItemsByFilePath::iterator fp_it(m_itemsByFilePath.find(boost::make_tuple(file_path, page_num)));
    if (fp_it != m_itemsByFilePath.end()) {
        if (overriden_filename && fp_it->overridenFileName != *overriden_filename) {
            fp_it.get_node()->value().overridenFileName = *overriden_filename;
        }
        return fp_it->label;
    }

    int label = 0;

    QString const file_name(QFileInfo(file_path).fileName());
    ItemsByFileNameLabel::iterator const fn_it(
        m_itemsByFileNameLabel.upper_bound(boost::make_tuple(file_name, page_num))
    );
    // If the item preceding fn_it has the same file name,
    // the new file belongs to the same disambiguation group.
    if (fn_it != m_itemsByFileNameLabel.begin()) {
        ItemsByFileNameLabel::iterator prev(fn_it);
        --prev;
        if (prev->fileName == file_name && prev->page == page_num) {
            label = prev->label + 1;
        }
    } // Otherwise, label remains 0.

    Item const new_item(file_path, file_name, label, page_num, overriden_filename ? *overriden_filename : QString());
    m_itemsByFileNameLabel.insert(fn_it, new_item);

    return label;
}

void
FileNameDisambiguator::Impl::performRelinking(AbstractRelinker const& relinker)
{
    QMutexLocker const locker(&m_mutex);
    Container new_items;

    for (Item const& item : m_unorderedItems) {
        RelinkablePath const old_path(item.filePath, RelinkablePath::File);
        Item new_item(relinker.substitutionPathFor(old_path), item.label, item.page, item.overridenFileName);
        new_items.insert(new_item);
    }

    m_items.swap(new_items);
}

/*============================ Impl::Item =============================*/

FileNameDisambiguator::Impl::Item::Item(QString const& file_path, int lbl, int page_num, QString const& overriden_filename)
    :   filePath(file_path),
        fileName(QFileInfo(file_path).fileName()),
        label(lbl),
        page(page_num),
        overridenFileName(overriden_filename)
{
}

FileNameDisambiguator::Impl::Item::Item(QString const& file_path, QString const& file_name, int lbl, int page_num, const QString& overriden_filename)
    :   filePath(file_path),
        fileName(file_name),
        label(lbl),
        page(page_num),
        overridenFileName(overriden_filename)
{
}
