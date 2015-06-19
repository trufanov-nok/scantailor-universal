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

#include "Settings.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include "DistortionType.h"
#include "../../Utils.h"
#include <QMutexLocker>
#include <boost/foreach.hpp>

namespace deskew
{

Settings::Settings()
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	QMutexLocker locker(&m_mutex);
	m_perPageParams.clear();
}

void
Settings::performRelinking(AbstractRelinker const& relinker)
{
	QMutexLocker locker(&m_mutex);
	PerPageParams new_params;

	BOOST_FOREACH(PerPageParams::value_type const& kv, m_perPageParams) {
		RelinkablePath const old_path(kv.first.imageId().filePath(), RelinkablePath::File);
		PageId new_page_id(kv.first);
		new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
		new_params.insert(PerPageParams::value_type(new_page_id, kv.second));
	}

	m_perPageParams.swap(new_params);
}

void
Settings::setPageParams(PageId const& page_id, Params const& params)
{
	QMutexLocker locker(&m_mutex);
	Utils::mapSetValue(m_perPageParams, page_id, params);
}

std::auto_ptr<Params>
Settings::getPageParams(PageId const& page_id) const
{
	QMutexLocker locker(&m_mutex);
	
	PerPageParams::const_iterator it(m_perPageParams.find(page_id));
	if (it != m_perPageParams.end()) {
		return std::auto_ptr<Params>(new Params(it->second));
	} else {
		return std::auto_ptr<Params>();
	}
}

DistortionType
Settings::getDistortionType(PageId const& page_id) const
{
	QMutexLocker locker(&m_mutex);

	PerPageParams::const_iterator it(m_perPageParams.find(page_id));
	if (it != m_perPageParams.end()) {
		return it->second.distortionType();
	} else {
		return Params::defaultDistortionType();
	}
}

void
Settings::setDistortionType(
	std::set<PageId> const& pages, DistortionType const& distortion_type)
{
	QMutexLocker const locker(&m_mutex);

	for (PageId const& page_id : pages) {
		PerPageParams::iterator it = m_perPageParams.find(page_id);
		if (it != m_perPageParams.end()) {
			it->second.setDistortionType(distortion_type);
		} else {
			Params params((Dependencies()));
			params.setDistortionType(distortion_type);
			Utils::mapSetValue(m_perPageParams, page_id, params);
		}
	}
}

void
Settings::setDepthPerception(
	std::set<PageId> const& pages, dewarping::DepthPerception const& depth_perception)
{
	QMutexLocker const locker(&m_mutex);

	for (PageId const& page_id : pages) {
		PerPageParams::iterator it = m_perPageParams.find(page_id);
		if (it != m_perPageParams.end()) {
			it->second.dewarpingParams().setDepthPerception(depth_perception);
		} else {
			Params params((Dependencies()));
			params.dewarpingParams().setDepthPerception(depth_perception);
			Utils::mapSetValue(m_perPageParams, page_id, params);
		}
	}
}

} // namespace deskew
