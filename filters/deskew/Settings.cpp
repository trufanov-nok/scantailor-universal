/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
#include "Utils.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include <QMutexLocker>
#include "settings/ini_keys.h"
#include <cmath>
#include <iostream>

namespace deskew
{

Settings::Settings() :
    m_avg(0.0),
    m_sigma(0.0)
{
    m_maxDeviation = CommandLine::get().getSkewDeviation();
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

	for (PerPageParams::value_type const& kv: m_perPageParams) {
		RelinkablePath const old_path(kv.first.imageId().filePath(), RelinkablePath::File);
		PageId new_page_id(kv.first);
		new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
		new_params.insert(PerPageParams::value_type(new_page_id, kv.second));
	}

    m_perPageParams.swap(new_params);
}

void Settings::updateDeviation()
{
    m_avg = 0.0;
    for (PerPageParams::value_type const& kv: m_perPageParams) {
        m_avg += kv.second.deskewAngle();
    }
    m_avg = m_avg / m_perPageParams.size();
#ifdef DEBUG
    std::cout << "avg skew = " << m_avg << std::endl;
#endif

    double sigma2 = 0.0;
    for (PerPageParams::value_type & kv: m_perPageParams) {
        kv.second.computeDeviation(m_avg);
        sigma2 += kv.second.deviation() * kv.second.deviation();
    }
    sigma2 = sigma2 / m_perPageParams.size();
    m_sigma = sqrt(sigma2);
#ifdef DEBUG
    std::cout << "sigma2 = " << sigma2 << std::endl;
    std::cout << "sigma = " << m_sigma << std::endl;
#endif
}

void
Settings::setPageParams(PageId const& page_id, Params const& params)
{
	QMutexLocker locker(&m_mutex);
	Utils::mapSetValue(m_perPageParams, page_id, params);
}

void
Settings::clearPageParams(PageId const& page_id)
{
	QMutexLocker locker(&m_mutex);
	m_perPageParams.erase(page_id);
}

std::unique_ptr<Params>
Settings::getPageParams(PageId const& page_id) const
{
	QMutexLocker locker(&m_mutex);
	
	PerPageParams::const_iterator it(m_perPageParams.find(page_id));
	if (it != m_perPageParams.end()) {
		return std::unique_ptr<Params>(new Params(it->second));
	} else {
		return std::unique_ptr<Params>();
	}
}

void
Settings::applyParams(std::set<PageId> const& pages, Params const& cur_params, UpdateOpt opt)
{

    // we need to recalc angle in following cases:
    // * if target params gets Auto mode
    // * if target params gets OrientationFix and already have Auto mode

    bool enforce_recalc = cur_params.requireRecalc();

    if (!enforce_recalc) {
        if (cur_params.mode() == AutoManualMode::MODE_AUTO &&
                opt != UpdateOrientationFix) {
            enforce_recalc = true; // 1st case
        }
    }

    for (PageId const& page_id: pages) {

        std::unique_ptr<Params> target_params = getPageParams(page_id);
        if (!target_params) {
            target_params.reset(new Params(0, cur_params.dependencies(), AutoManualMode::MODE_AUTO, Params::OrientationFixNone, true));
        }

        if (enforce_recalc) {
            target_params->setRequireRecalc(true);
        }

        if (opt == UpdateModeAndAngle || opt == UpdateAll) {
            target_params->setDeskewAngle(cur_params.deskewAngle());
            target_params->setMode(cur_params.mode());
        }

        if (opt == UpdateOrientationFix || opt == UpdateAll) {
            if (page_id.subPage() != PageId::SINGLE_PAGE) { // we aren't allow user to apply orientation fix for single pages - they should change their rotrtion on 1st processing step
                if (target_params->orientationFix() != cur_params.orientationFix()) {
                    target_params->setOrientationFix(cur_params.orientationFix());
                    if (!enforce_recalc &&
                            target_params->mode() == AutoManualMode::MODE_AUTO) {
                        target_params->setRequireRecalc(true); // 2nd case
                    }
                }
            }
        }

        setPageParams(page_id, *target_params);
    }

}

} // namespace deskew
