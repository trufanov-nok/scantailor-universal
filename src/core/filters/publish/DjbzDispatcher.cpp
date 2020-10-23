/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2020 Alexander Trufanov <trufanovan@gmail.com>

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

#include "DjbzDispatcher.h"
#include "settings/globalstaticsettings.h"
#include "PageSequence.h"
#include "Settings.h"
#include <QDateTime>
#include <QDomDocument>
#include <QDebug>

namespace publish {

DjbzParams::DjbzParams():
    m_usePrototypes(GlobalStaticSettings::m_djvu_djbz_use_prototypes),
    m_useAveraging(GlobalStaticSettings::m_djvu_djbz_use_averaging),
    m_aggression(GlobalStaticSettings::m_djvu_djbz_aggression),
    m_useErosion(GlobalStaticSettings::m_djvu_djbz_erosion),
    m_extension(GlobalStaticSettings::m_djvu_djbz_extension)
{
}

DjbzParams::DjbzParams(QDomElement const& el)
{
    m_usePrototypes = el.attribute("prototypes", QString::number(GlobalStaticSettings::m_djvu_djbz_use_prototypes)).toInt();
    m_useAveraging = el.attribute("averaging", QString::number(GlobalStaticSettings::m_djvu_djbz_use_averaging)).toInt();
    m_useErosion = el.attribute("erosion", QString::number(GlobalStaticSettings::m_djvu_djbz_erosion)).toInt();
    m_aggression = el.attribute("aggression", QString::number(GlobalStaticSettings::m_djvu_djbz_aggression)).toInt();
    m_extension = el.attribute("ext", GlobalStaticSettings::m_djvu_djbz_extension);
}

QDomElement
DjbzParams::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    el.setAttribute("prototypes", m_usePrototypes);
    el.setAttribute("averaging", m_useAveraging);
    el.setAttribute("erosion", m_useErosion);
    el.setAttribute("aggression", m_aggression);
    el.setAttribute("ext", m_extension);
    return el;
}

bool
DjbzParams::operator !=(const DjbzParams &b) const
{
    return (m_useErosion != b.m_useErosion ||
            m_useAveraging != b.m_useAveraging ||
            m_usePrototypes != b.m_usePrototypes ||
            m_aggression != b.m_aggression ||
            m_extension != b.m_extension);
}

////////////////////////////////////////////
///            DjbzDict
////////////////////////////////////////////

const QString DjbzDict::dummyDjbzId = "[none]";

DjbzDict::DjbzDict(const QString& id):
    m_locked(false),
    m_max_pages(GlobalStaticSettings::m_djvu_pages_per_djbz),

    m_output_file_size(0),
    m_output_file_last_changed(m_last_changed)
{
    m_is_dummy = id.isEmpty() || id == DjbzDict::dummyDjbzId;
    if (m_is_dummy) {
        m_id = DjbzDict::dummyDjbzId;
    } else {
        m_id = id;
        m_last_changed = QDateTime::currentDateTimeUtc();
    }

}

void
DjbzDict::addPage(const PageId& page, bool no_rev_change)
{
    m_pages += page;
    if (m_pages.count() > m_max_pages) {
        m_max_pages = m_pages.count();
    }

    if (!m_is_dummy && !no_rev_change) {
        m_last_changed = QDateTime::currentDateTimeUtc();
    }
}

void
DjbzDict::removePage(const PageId& page, bool no_rev_change)
{
    if (m_pages.remove(page)) {
        if (!m_is_dummy && !no_rev_change) {
            m_last_changed = QDateTime::currentDateTimeUtc();
        }
    }
}

void
DjbzDict::setParams(const DjbzParams& params, bool no_rev_change)
{
    if (!m_is_dummy) {
        if (m_params != params) {
            m_params = params;
            if (!no_rev_change) {
                m_last_changed = QDateTime::currentDateTimeUtc();
            }
        }
    }
}

void
DjbzDict::updateOutputFileInfo()
{
    if (!m_is_dummy && !m_output_file.isEmpty()) {
        QFileInfo fi;
        fi.setFile(m_output_file);
        m_output_file_size = fi.exists() ? fi.size() : 0;
        m_output_file_last_changed = fi.lastModified();
    }
}

////////////////////////////////////////////
///            DjbzDispatcher
////////////////////////////////////////////

void __add_new_dictionary(DjbzContent& map, DjbzDict* dict)
{
    if (dict) {
        if (map.contains(dict->id())) {
            delete map[dict->id()];
        }
        map[dict->id()] = dict;
    }
}

DjbzDispatcher::DjbzDispatcher():
    m_id_counter(0)
{
    __add_new_dictionary(m_dictionaries_by_id, new DjbzDict(DjbzDict::dummyDjbzId));
}

DjbzDispatcher::DjbzDispatcher(const DjbzDispatcher& src)
{
    m_id_counter = src.m_id_counter;

    for (DjbzContent::const_iterator it = src.m_dictionaries_by_id.cbegin(); it != src.m_dictionaries_by_id.cend(); ++it) {
        const DjbzDict* dict = *it;
        DjbzDict* new_dict = new DjbzDict(dict->id());
        *new_dict = *dict;
        __add_new_dictionary(m_dictionaries_by_id, new_dict);
        for (const PageId& p: new_dict->pages()) {
            m_page_to_dict_id[p] = new_dict;
        }
    }
}


DjbzDispatcher::~DjbzDispatcher()
{
    for (DjbzContent::const_iterator it = m_dictionaries_by_id.cbegin(); it != m_dictionaries_by_id.cend(); ++it) {
        if (*it) {
            delete (*it);
        }
    }
    m_dictionaries_by_id.clear();
    m_page_to_dict_id.clear();
}

QString
DjbzDispatcher::dictionaryIdByPage(PageId const& page_id) const
{
    if (m_page_to_dict_id.contains(page_id)) {
        return m_page_to_dict_id[page_id]->id();
    }
    return QString();
}

DjbzDict *
DjbzDispatcher::dictionaryByPage(PageId const& page_id) const
{
    if (m_page_to_dict_id.contains(page_id)) {
        return m_page_to_dict_id[page_id];
    }
    return nullptr;
}

DjbzDict *
DjbzDispatcher::dictionary(QString const& dict_id)
{
    if (m_dictionaries_by_id.contains(dict_id)) {
        return m_dictionaries_by_id[dict_id];
    }
    return nullptr;
}

const DjbzDict *
DjbzDispatcher::dictionary(QString const& dict_id) const
{
    assert (m_dictionaries_by_id.contains(dict_id));
    return m_dictionaries_by_id[dict_id];
}

QStringList
DjbzDispatcher::listAllDjbz() const
{
    QStringList res = m_dictionaries_by_id.keys();
    // let dummy djbz be first in the list
    int idx = res.indexOf(DjbzDict::dummyDjbzId);
    if (idx != -1) {
        res.move(idx, 0);
    }
    return res;
}

QMap<QString, int>
DjbzDispatcher::listAllDjbzAndTheirSize() const
{
    QMap<QString, int> res;
    const QStringList sl = listAllDjbz();
    for(const QString& s: sl) {
        res[s] = m_dictionaries_by_id[s]->pageCount();
    }
    return res;
}


QString
DjbzDispatcher::nextDjbzId()
{
    QString dict_id;
    const QChar c('0');
    do {
        dict_id = QString("%1").arg(++m_id_counter, 4, 10, c);
    } while (m_dictionaries_by_id.contains(dict_id));

    return dict_id;
}

DjbzDict*
DjbzDispatcher::addNewPage(PageId const& page_id)
{
    if (GlobalStaticSettings::m_djvu_pages_per_djbz < 2) {
        DjbzDict* dummy = m_dictionaries_by_id[DjbzDict::dummyDjbzId];
        dummy->addPage(page_id);
        m_page_to_dict_id[page_id] = dummy;
        return dummy;
    }

    DjbzDict* dict = dictionaryByPage(page_id);
    if (!dict) {
        // Try to find any dictionary that is still not full
        for (DjbzContent::const_iterator it = m_dictionaries_by_id.cbegin(); it != m_dictionaries_by_id.cend(); ++it) {
            if (!(*it)->isDummy() && !(*it)->isFull()) {
                dict = *it;
                break;
            }
        }

        if (!dict) {
            // we need a new dictionary for this page
            const QString dict_id = nextDjbzId();
            dict = new DjbzDict(dict_id);
            __add_new_dictionary(m_dictionaries_by_id, dict);
        }

        dict->addPage(page_id);
        m_page_to_dict_id[page_id] = dict;
    }
    return dict;
}

DjbzDict*
DjbzDispatcher::addNewDictionary(QString const& id)
{
    if (id.isEmpty() || m_dictionaries_by_id.contains(id)) {
        return nullptr;
    }

    DjbzDict* dict = new DjbzDict(id);
    __add_new_dictionary(m_dictionaries_by_id, dict);
    return dict;
}

void
DjbzDispatcher::removeFromDictionary(PageId const& page_id)
{
    if (m_page_to_dict_id.contains(page_id)) {
        m_page_to_dict_id[page_id]->removePage(page_id);
        m_page_to_dict_id.remove(page_id);
    }
}

void
DjbzDispatcher::addToDictionary(PageId const& page_id, QString const& dict_id, bool no_rev_change)
{
    if (m_dictionaries_by_id.contains(dict_id)) {
        DjbzDict* dict = m_dictionaries_by_id[dict_id];
        addToDictionary(page_id, dict, no_rev_change);
    }
}

void
DjbzDispatcher::addToDictionary(PageId const& page_id, DjbzDict* dict, bool no_rev_change)
{
    if (dict) {
        dict->addPage(page_id, no_rev_change);
        m_page_to_dict_id[page_id] = dict;
    }
}

void
DjbzDispatcher::moveToDjbz(PageId const& page_id, QString const& dict_id)
{
    removeFromDictionary(page_id);
    addToDictionary(page_id, dict_id);
}

void
DjbzDispatcher::moveToDjbz(PageId const& page_id, DjbzDict* dict)
{
    removeFromDictionary(page_id);
    addToDictionary(page_id, dict);
}

void
DjbzDispatcher::resetNonLockedDictionaries()
{
    DjbzContent locked_dicts;
    PageToDjbz locked_pages;

    for (DjbzContent::const_iterator it = m_dictionaries_by_id.cbegin(); it != m_dictionaries_by_id.cend(); ++it) {
        DjbzDict* dict = *it;
        if (dict->isLocked()) {
            locked_dicts[dict->id()] = dict;
            for (const PageId& p: dict->pages()) {
                locked_pages[p] = dict;
            }
        } else {
            delete dict;
        }
    }

    m_id_counter = 0;
    m_dictionaries_by_id = locked_dicts;
    if (!m_dictionaries_by_id.contains(DjbzDict::dummyDjbzId)) {
        m_dictionaries_by_id[DjbzDict::dummyDjbzId] = new DjbzDict();
    }
    m_page_to_dict_id = locked_pages;
}

void
DjbzDispatcher::autosetToDictionaries(const PageSequence& pages, const ExportSuggestions* export_suggestions, IntrusivePtr<Settings> settings)
{
    const bool single_page_project = pages.numPages() == 1;

    for (const PageInfo& p: pages) {
        PageId const& page_id = p.id();
        std::unique_ptr<Params> params_ptr(settings->getPageParams(page_id));
        const ExportSuggestion es = export_suggestions->value(page_id);

        DjbzDict* dict = nullptr;
        bool need_new_dict = (!params_ptr || params_ptr->djbzId().isEmpty());

        if (!need_new_dict) {
            dict = dictionary(params_ptr->djbzId());
            if (!dict) {
                need_new_dict = true;
            } else if (!dict->isLocked()) {
                const bool is_dummy = dict->isDummy();
                if ( (es.hasBWLayer && !single_page_project && is_dummy) ||
                     (!es.hasBWLayer && !is_dummy) ) {
                    removeFromDictionary(page_id);
                    need_new_dict = true;
                }
            }
        }

        if (need_new_dict) {
            QString new_dict_id;
            if (es.hasBWLayer && !single_page_project) {
                new_dict_id = addNewPage(page_id)->id();
            } else {
                new_dict_id = DjbzDict::dummyDjbzId;
                addToDictionary(page_id, new_dict_id);
            }

            Params params = params_ptr ? Params(*params_ptr.get()) : Params();
            params.setDjbzId(new_dict_id);
            settings->setPageParams(page_id, params);
        }

    }
}


QDomElement
DjbzDispatcher::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement root_el(doc.createElement(name));
    const QStringList keys = m_dictionaries_by_id.keys();
    for (QString const& id: keys) {
        QDomElement el(doc.createElement("djbz"));
        const DjbzDict * dict = m_dictionaries_by_id[id];
        el.setAttribute("id", dict->id());
        el.setAttribute("locked", dict->isLocked());
        if (!dict->isDummy()) {
            el.setAttribute("max", dict->maxPages());
            el.setAttribute("last_changed", dict->revision().toString("dd.MM.yyyy hh:mm:ss.zzz"));
            el.setAttribute("output_file", dict->outputFilename());
            el.setAttribute("output_file_size", dict->outputFileSize());
            el.setAttribute("output_file_last_changed", dict->outputLastChanged().toString("dd.MM.yyyy hh:mm:ss.zzz"));
            el.appendChild(dict->params().toXml(doc, "djbz_params"));
        }
        root_el.appendChild(el);
    }

    return root_el;
}

DjbzDispatcher::DjbzDispatcher(QDomElement const& el):
    m_id_counter(0)
{
    qDebug() << el.toElement().isNull();
    QDomNode node(el.firstChild());
    for (; !node.isNull(); node = node.nextSibling()) {
        if (!node.isElement()) {
            continue;
        }
        if (node.nodeName() != "djbz") {
            continue;
        }
        QDomElement const el(node.toElement());

        DjbzDict* dict = new DjbzDict(el.attribute("id", ""));
        dict->setLocked(el.attribute("locked", "0").toUInt());
        if (!dict->isDummy()) {
            dict->setMaxPages(el.attribute("max", QString::number(GlobalStaticSettings::m_djvu_pages_per_djbz)).toUInt());
            dict->setRevision(QDateTime::fromString(el.attribute("last_changed", QDateTime::currentDateTimeUtc().toString("dd.MM.yyyy hh:mm:ss.zzz")), "dd.MM.yyyy hh:mm:ss.zzz"));
            dict->setOutputFilename(el.attribute("output_file"));
            dict->setOutputFileSize(el.attribute("output_file_size", 0).toUInt());
            el.attribute("output_file_last_changed", dict->outputLastChanged().toString("dd.MM.yyyy hh:mm:ss.zzz"));
            dict->setOutputLastChanged(QDateTime::fromString(el.attribute("output_file_last_changed", QDateTime::currentDateTimeUtc().toString("dd.MM.yyyy hh:mm:ss.zzz")), "dd.MM.yyyy hh:mm:ss.zzz"));
            DjbzParams params(node.namedItem("djbz_params").toElement());
            dict->setParams(params);
        }

        __add_new_dictionary(m_dictionaries_by_id, dict);
    }

    // should be ready, but just to make sure that project isn't damaged or created with old ST version
    if (!m_dictionaries_by_id.contains(DjbzDict::dummyDjbzId)) {
        __add_new_dictionary(m_dictionaries_by_id, new DjbzDict(DjbzDict::dummyDjbzId));
    }
}

bool
DjbzDispatcher::isDjbzEncodingRequired(const PageId& page) const
{
    return !m_page_to_dict_id[page]->isDummy();
}

QSet<PageId>
DjbzDispatcher::pagesOfDictionary(const QString& dict_id) const
{
    return pagesOfDictionary(m_dictionaries_by_id[dict_id]);
}

QSet<PageId>
DjbzDispatcher::pagesOfDictionary(const DjbzDict* dict) const
{
    if (dict) {
        return dict->pages();
    }
    return QSet<PageId>();
}

QSet<PageId>
DjbzDispatcher::pagesOfSameDictionary(const PageId& page) const
{
    const DjbzDict* dict = m_page_to_dict_id[page];

    if (dict->isDummy()) {
        return QSet<PageId> ({page});
    } else {
        return dict->pages();
    }
}

QString
getFileToEncode(SourceImagesInfo const & si)
{
    return !si.export_foregroundFilename().isEmpty() ? // check if has been exported as a layer
                                                       si.export_foregroundFilename() :
                                                       si.source_filename();
}

void
DjbzDispatcher::generateDjbzEncodingParams(const PageId& page, Settings const& page_settings, QStringList& encoder_settings) const
{
    assert(m_page_to_dict_id.contains(page));
    const DjbzDict * dict = m_page_to_dict_id[page];
    const DjbzParams & dict_params = dict->params();
    if (!dict->isDummy()) {
        encoder_settings << "(djbz "
                         << "  id            " + dict->id()
                         << "  xtension      " + dict_params.extension()
                         << "  averaging     " + QString(dict_params.useAveraging() ? "1" :"0")
                         << "  aggression    " + QString::number(dict_params.agression())
                         << "  no-prototypes " + QString(dict_params.usePrototypes() ? "0" :"1")
                         << "  erosion       " + QString(dict_params.useErosion() ? "1" :"0");

        encoder_settings << "      (files";
        for (const PageId& p : dict->pages()) {
            const SourceImagesInfo si = page_settings.getPageParams(p)->sourceImagesInfo();
            if (si.export_suggestion().hasBWLayer) {
                encoder_settings << "            \"" + getFileToEncode(si) + "\"";
            }
        }
        encoder_settings << "      ) #files"

                         << ") #djbz";
    }/* else {
        // single page djbz are ignored and pages are encoded without shared dictionary
        const std::unique_ptr<Params> params = ;
        encoder_settings << "(djbz"
                         << getFileToEncode(params->sourceImagesInfo())
                         << ") #djbz";
    }*/
}

bool DjbzDispatcher::isDictionaryCached(const DjbzDict* dict) const
{
    if (dict) {
        if (dict->isDummy()) {
            return true;
        }

        if (!dict->outputFilename().isEmpty()) {
            QFileInfo fi(dict->outputFilename());
            return (fi.exists() &&
                    fi.size() == dict->outputFileSize() &&
                    fi.lastModified() == dict->outputLastChanged());
        }
    }
    return false;
}

} // namespace
