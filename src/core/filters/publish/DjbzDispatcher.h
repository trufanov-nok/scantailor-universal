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

#ifndef DJBZDISPATCHER_H
#define DJBZDISPATCHER_H

#include "PageId.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"

#include <QMap>
#include <QSet>
#include <QDateTime>

class QDomDocument;
class QDomElement;
class QDateTime;
class PageSequence;
class OutputFileNameGenerator;
class ExportSuggestions;

namespace publish {

class Settings;

class DjbzParams
{
public:

    DjbzParams();
    DjbzParams(QDomElement const& el);

    bool usePrototypes() const { return m_usePrototypes; }
    void setUsePrototypes(bool prototypes) { m_usePrototypes = prototypes; }

    bool useAveraging() const { return m_useAveraging; }
    void setUseAveraging(bool averaging) { m_useAveraging = averaging; }

    int agression() const { return m_aggression; }
    void setAgression(int agression) { m_aggression = agression; }

    bool useErosion() const { return m_useErosion; }
    void setUseErosion(bool erosion) { m_useErosion = erosion; }

    QString extension() const { return m_extension; }
    void setExtension(const QString & ext) { m_extension = ext; }

    QDomElement toXml(QDomDocument& doc, QString const& name) const;

    bool operator !=(const DjbzParams &b) const;

    ~DjbzParams(){}
private:
    bool m_usePrototypes;
    bool m_useAveraging;
    int  m_aggression;
    bool  m_useErosion;
    QString m_extension;
};


class DjbzDict
{
public:
    DjbzDict(const QString& id = QString());

    const QString& id() const { return m_id; }

    bool isLocked() const { return m_locked; }
    void setLocked(bool val) { m_locked = val; }
    bool isDummy() const { return m_id == DjbzDict::dummyDjbzId; }
    bool isFull() const { return m_pages.size() >= m_max_pages; }
    bool isMultipage() const { return !m_is_dummy && m_pages.size() > 1; }

    uint maxPages() const { return m_max_pages; }
    void setMaxPages(uint max) { m_max_pages = max; }
    uint pageCount() const { return m_pages.count(); }

    void addPage(const PageId& page, bool no_rev_change = false);
    void removePage(const PageId& page, bool no_rev_change = false);
    const QSet<PageId>& pages() const { return m_pages; }

    void setParams(const DjbzParams& params, bool no_rev_change = false);
    const DjbzParams& params() const { return m_params; }
    DjbzParams& paramsRef() { return m_params; }

    QDateTime revision() const { return m_last_changed; }
    void setRevision(QDateTime val) { if (!m_is_dummy) { m_last_changed = val; } }



    void setOutputFilename(const QString& val) { if (!m_is_dummy) { m_output_file = val; updateOutputFileInfo(); } }
    const QString& outputFilename() const { return m_output_file; }
    uint outputFileSize() const { return m_output_file_size; }
    void setOutputFileSize(uint val) { if (!m_is_dummy) { m_output_file_size = val;} }
    const QDateTime& outputLastChanged() const { return m_output_file_last_changed; }
    void setOutputLastChanged(const QDateTime& val) { if (!m_is_dummy) { m_output_file_last_changed = val; } }
    void updateOutputFileInfo();

    static const QString dummyDjbzId;
private:
    QString m_id;
    QSet<PageId> m_pages;
    DjbzParams m_params;
    bool m_is_dummy;
    bool m_locked;   // only user can add pages to this djbz
    uint m_max_pages;
    QDateTime m_last_changed; // last change made by user
    QString m_output_file;
    uint m_output_file_size;
    QDateTime m_output_file_last_changed; // time when djbz file was generated
};

typedef QMap<QString, DjbzDict*> DjbzContent;
typedef QMap<PageId, DjbzDict*> PageToDjbz;

class DjbzDispatcher: public RefCountable
{
public:
    DjbzDispatcher();
    DjbzDispatcher(QDomElement const& el);
    DjbzDispatcher(const DjbzDispatcher&);

    ~DjbzDispatcher();

    QString dictionaryIdByPage(PageId const& page_id) const;
    DjbzDict * dictionaryByPage(PageId const& page_id) const;
    DjbzDict * dictionary(const QString &dict_id);
    const DjbzDict * dictionary(const QString &dict_id) const;

    QStringList listAllDjbz() const;
    QMap<QString, int> listAllDjbzAndTheirSize() const;
    void ensurePageHasDjbz(const PageId& page_id);
    DjbzDict* addNewPage(PageId const& page_id);
    DjbzDict* addNewDictionary(QString const& id);
    void removeFromDictionary(PageId const& page_id);
    void addToDictionary(PageId const& page_id, QString const& dict_id, bool no_rev_change = false);
    void addToDictionary(PageId const& page_id, DjbzDict * dict, bool no_rev_change = false);
    void moveToDjbz(PageId const& page_id, QString const& dict_id);
    void moveToDjbz(PageId const& page_id, DjbzDict * dict);
    void resetNonLockedDictionaries();
    void autosetToDictionaries(const PageSequence& pages, const ExportSuggestions* export_suggestions, IntrusivePtr<Settings> settings);
    bool isInitialised() const {return !m_page_to_dict_id.isEmpty();}

    QSet<PageId> pagesOfDictionary(const QString& dict_id) const;
    QSet<PageId> pagesOfDictionary(const DjbzDict* dict) const;
    QSet<PageId> pagesOfSameDictionary(const PageId& page) const;
    void generateDjbzEncodingParams(const PageId& page, Settings const& page_settings, QStringList& encoder_settings) const;
    bool isDjbzEncodingRequired(const PageId& page) const;

    bool isDictionaryCached(const DjbzDict *dict) const;
    QDomElement toXml(QDomDocument& doc, QString const& name) const;

private:
    QString nextDjbzId();
private:
    DjbzContent m_dictionaries_by_id;
    PageToDjbz m_page_to_dict_id;
    int m_id_counter;
};

}

#endif // DJBZDISPATCHER_H
