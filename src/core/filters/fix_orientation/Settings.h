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

#ifndef FIX_ORIENTATION_SETTINGS_H_
#define FIX_ORIENTATION_SETTINGS_H_

#include "RefCountable.h"
#include "NonCopyable.h"
#include "OrthogonalRotation.h"
#include "ImageId.h"
#include "PageId.h"
#include "imageproc/OrientationDetector.h"
#include <QMutex>
#include <QString>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <QSize>

class AbstractRelinker;

namespace fix_orientation
{

class Settings : public RefCountable
{
    DECLARE_NON_COPYABLE(Settings)
public:
    Settings();

    virtual ~Settings();

    void clear();

    void performRelinking(AbstractRelinker const& relinker);

    void applyRotation(ImageId const& image_id, OrthogonalRotation rotation);

    void applyRotation(std::set<PageId> const& pages, OrthogonalRotation rotation);

    OrthogonalRotation getRotationFor(ImageId const& image_id) const;

    enum AutoState { AUTO_NONE, AUTO_PENDING, AUTO_DONE };

    void requestAutoRotation(ImageId const& image_id);

    void requestAutoRotation(std::set<PageId> const& pages);

    AutoState autoState(ImageId const& image_id) const;

    void setAutoState(ImageId const& image_id, AutoState state);

    imageproc::OrientationResult applyAutoResult(ImageId const& image_id, imageproc::OrientationResult const& result);

    QString autoResultText(ImageId const& image_id) const;

    std::vector<ImageId> pendingImages() const;

    bool hasBookRotation() const;

    void cacheDetection(ImageId const& image_id, imageproc::OrientationResult const& result);

    bool cachedDetection(ImageId const& image_id, imageproc::OrientationResult* result) const;
private:
    typedef std::map<ImageId, OrthogonalRotation> PerImageRotation;

    void setImageRotationLocked(
        ImageId const& image_id, OrthogonalRotation const& rotation);

    mutable QMutex m_mutex;
    PerImageRotation m_perImageRotation;
    std::map<ImageId, AutoState> m_autoState;
    std::map<ImageId, QString> m_autoResultText;
    // Confident automatic results of this session, so that a sideways table can take the rotation of the book.
    int m_bookVotes[4];
    // Results that a look-ahead found, so that the stage does not detect these scans again.
    std::map<ImageId, imageproc::OrientationResult> m_detected;
    std::set<ImageId> m_voted;
    std::map<std::pair<int, int>, int> m_sizeVotes;

    void addVoteLocked(ImageId const& image_id, imageproc::OrientationResult const& result);

    int bookAngleLocked() const;

    QSize bookSizeLocked() const;
};

} // namespace fix_orientation

#endif
