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
#include <QChar>
#include <QCoreApplication>
#include <algorithm>

namespace fix_orientation
{

Settings::Settings()
{
    std::fill(m_bookVotes, m_bookVotes + 4, 0);
}

Settings::~Settings()
{
}

void
Settings::clear()
{
    QMutexLocker locker(&m_mutex);
    m_perImageRotation.clear();
    m_autoState.clear();
    m_autoResultText.clear();
    m_detected.clear();
    m_voted.clear();
    m_sizeVotes.clear();
    std::fill(m_bookVotes, m_bookVotes + 4, 0);
}

void
Settings::performRelinking(AbstractRelinker const& relinker)
{
    QMutexLocker locker(&m_mutex);
    PerImageRotation new_rotations;

    for (PerImageRotation::value_type const& kv : m_perImageRotation) {
        RelinkablePath const old_path(kv.first.filePath(), RelinkablePath::File);
        ImageId new_image_id(kv.first);
        new_image_id.setFilePath(relinker.substitutionPathFor(old_path));
        new_rotations.insert(PerImageRotation::value_type(new_image_id, kv.second));
    }

    m_perImageRotation.swap(new_rotations);

    std::map<ImageId, AutoState> new_states;
    for (auto const& kv : m_autoState) {
        ImageId new_image_id(kv.first);
        new_image_id.setFilePath(relinker.substitutionPathFor(RelinkablePath(kv.first.filePath(), RelinkablePath::File)));
        new_states[new_image_id] = kv.second;
    }
    m_autoState.swap(new_states);
    m_autoResultText.clear();
    // These caches use the old file paths, so the stage detects the pending scans again.
    m_detected.clear();
    m_voted.clear();
    m_sizeVotes.clear();
    std::fill(m_bookVotes, m_bookVotes + 4, 0);
}

void
Settings::applyRotation(
    ImageId const& image_id, OrthogonalRotation const rotation)
{
    QMutexLocker locker(&m_mutex);
    setImageRotationLocked(image_id, rotation);
    // A rotation from the user replaces an automatic one.
    m_autoState.erase(image_id);
    m_autoResultText.erase(image_id);
}

void
Settings::applyRotation(
    std::set<PageId> const& pages, OrthogonalRotation const rotation)
{
    QMutexLocker locker(&m_mutex);

    for (PageId const& page : pages) {
        setImageRotationLocked(page.imageId(), rotation);
        m_autoState.erase(page.imageId());
        m_autoResultText.erase(page.imageId());
    }
}

void
Settings::requestAutoRotation(ImageId const& image_id)
{
    QMutexLocker locker(&m_mutex);
    m_autoState[image_id] = AUTO_PENDING;
    m_autoResultText.erase(image_id);
    // A new request must detect again, because a result from an older look-ahead can be out of date.
    m_detected.erase(image_id);
}

void
Settings::requestAutoRotation(std::set<PageId> const& pages)
{
    QMutexLocker locker(&m_mutex);
    for (PageId const& page : pages) {
        m_autoState[page.imageId()] = AUTO_PENDING;
        m_autoResultText.erase(page.imageId());
        m_detected.erase(page.imageId());
    }
}

Settings::AutoState
Settings::autoState(ImageId const& image_id) const
{
    QMutexLocker locker(&m_mutex);
    std::map<ImageId, AutoState>::const_iterator const it(m_autoState.find(image_id));
    return it == m_autoState.end() ? AUTO_NONE : it->second;
}

void
Settings::setAutoState(ImageId const& image_id, AutoState const state)
{
    QMutexLocker locker(&m_mutex);
    if (state == AUTO_NONE) {
        m_autoState.erase(image_id);
    } else {
        m_autoState[image_id] = state;
    }
}

void
Settings::addVoteLocked(ImageId const& image_id, imageproc::OrientationResult const& result)
{
    if (!m_voted.insert(image_id).second) {
        return;
    }
    if (result.isDecided() && result.confidence() >= imageproc::OrientationDetector::confidenceThreshold()) {
        ++m_bookVotes[result.angle() / 90];
        ++m_sizeVotes[std::make_pair(result.imageSize().width(), result.imageSize().height())];
    }
}

QSize
Settings::bookSizeLocked() const
{
    QSize size;
    int best = 0;
    for (auto const& kv : m_sizeVotes) {
        if (kv.second > best) {
            best = kv.second;
            size = QSize(kv.first.first, kv.first.second);
        }
    }
    return size;
}

int
Settings::bookAngleLocked() const
{
    // The same rule as OrientationDetector::applyBookRotation(): a clear majority of at least 5 confident scans.
    int decisions = 0;
    int book = 0;
    for (int i = 0; i < 4; ++i) {
        decisions += m_bookVotes[i];
        if (m_bookVotes[i] > m_bookVotes[book]) {
            book = i;
        }
    }
    if (m_bookVotes[book] < 5 || m_bookVotes[book] * 10 < decisions * 6) {
        return -1;
    }
    return book * 90;
}

bool
Settings::hasBookRotation() const
{
    QMutexLocker locker(&m_mutex);
    return bookAngleLocked() >= 0;
}

std::vector<ImageId>
Settings::pendingImages() const
{
    QMutexLocker locker(&m_mutex);
    std::vector<ImageId> images;
    for (auto const& kv : m_autoState) {
        if (kv.second == AUTO_PENDING && m_detected.find(kv.first) == m_detected.end()) {
            images.push_back(kv.first);
        }
    }
    return images;
}

void
Settings::cacheDetection(ImageId const& image_id, imageproc::OrientationResult const& result)
{
    QMutexLocker locker(&m_mutex);
    m_detected[image_id] = result;
    addVoteLocked(image_id, result);
}

bool
Settings::cachedDetection(ImageId const& image_id, imageproc::OrientationResult* result) const
{
    QMutexLocker locker(&m_mutex);
    std::map<ImageId, imageproc::OrientationResult>::const_iterator const it(m_detected.find(image_id));
    if (it == m_detected.end()) {
        return false;
    }
    *result = it->second;
    return true;
}

imageproc::OrientationResult
Settings::applyAutoResult(ImageId const& image_id, imageproc::OrientationResult const& result)
{
    using imageproc::OrientationResult;

    QMutexLocker locker(&m_mutex);

    addVoteLocked(image_id, result);
    m_detected.erase(image_id);

    OrientationResult const final_result(
        imageproc::OrientationDetector::applyBookRotation(result, bookAngleLocked(), bookSizeLocked())
    );

    if (final_result.isDecided()) {
        OrthogonalRotation rotation;
        for (int i = 0; i < final_result.angle() / 90; ++i) {
            rotation.nextClockwiseDirection();
        }
        setImageRotationLocked(image_id, rotation);
        m_autoResultText[image_id] = QCoreApplication::translate("fix_orientation::Settings", "Auto: %1%2 (%3)")
                                     .arg(final_result.angle()).arg(QChar(0x00B0)).arg(final_result.reason());
    } else {
        m_autoResultText[image_id] = QCoreApplication::translate("fix_orientation::Settings", "Auto: no decision (%1)")
                                     .arg(final_result.reason());
    }
    m_autoState[image_id] = AUTO_DONE;
    return final_result;
}

QString
Settings::autoResultText(ImageId const& image_id) const
{
    QMutexLocker locker(&m_mutex);
    std::map<ImageId, QString>::const_iterator const it(m_autoResultText.find(image_id));
    if (it != m_autoResultText.end()) {
        return it->second;
    }
    std::map<ImageId, AutoState>::const_iterator const st(m_autoState.find(image_id));
    if (st == m_autoState.end()) {
        return QString();
    }
    return st->second == AUTO_PENDING
           ? QCoreApplication::translate("fix_orientation::Settings", "Auto: the stage finds the rotation")
           : QCoreApplication::translate("fix_orientation::Settings", "Auto");
}

OrthogonalRotation
Settings::getRotationFor(ImageId const& image_id) const
{
    QMutexLocker locker(&m_mutex);

    PerImageRotation::const_iterator it(m_perImageRotation.find(image_id));
    if (it != m_perImageRotation.end()) {
        return it->second;
    } else {
        return OrthogonalRotation();
    }
}

void
Settings::setImageRotationLocked(
    ImageId const& image_id, OrthogonalRotation const& rotation)
{
    Utils::mapSetValue(m_perImageRotation, image_id, rotation);
}

} // namespace fix_orientation
