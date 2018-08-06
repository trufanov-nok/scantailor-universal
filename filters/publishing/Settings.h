/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2018 Alexander Trufanov <trufanovan@gmail.com>

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

#ifndef PUBLISHING_SETTINGS_H_
#define PUBLISHING_SETTINGS_H_

#include "RefCountable.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "Params.h"
#include <QMutex>
#include <map>
#include <QJSValue>

class AbstractRelinker;

namespace publishing
{

class Settings : public RefCountable
{
	DECLARE_NON_COPYABLE(Settings)
public:
	Settings();
	
	virtual ~Settings();
	
	void clear();

    void performRelinking(AbstractRelinker const& relinker);

    Params getParams(PageId const& page_id) const;

    void setParams(PageId const& page_id, Params const& params);

    void setDpi(PageId const& page_id, Dpi const& dpi);

    Dpi dpi(PageId const& page_id) const;

    void setEncoderState(PageId const& page_id, QVariantMap const& val);

    QVariantMap encoderState(PageId const& page_id) const;

    void setConverterState(PageId const& page_id, QVariantMap const& val);

    QString inputFilename(PageId const& page_id) const;

    void setInputFilename(PageId const& page_id, QString const& fileName);

    QVariantMap converterState(PageId const& page_id) const;

    QVariantMap state(PageId const& page_id, bool const isEncoder) const {
        return (isEncoder)? encoderState(page_id) : converterState(page_id);
    }        


    void setState(PageId const& page_id, QVariantMap const& val, bool const isEncoder) {
        if (isEncoder) {
            setEncoderState(page_id, val);
        } else {
            setConverterState(page_id, val);
        }
    }

private:
    typedef std::map<PageId, Params> PerPageParams;
	mutable QMutex m_mutex;    
    PerPageParams m_perPageParams;
};

} // namespace publishing

#endif
