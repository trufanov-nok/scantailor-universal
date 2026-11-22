/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

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

#ifndef IMAGEPROC_ORIENTATION_DETECTOR_H_
#define IMAGEPROC_ORIENTATION_DETECTOR_H_

#include <QSize>
#include <QString>
#include <vector>

class QImage;

namespace imageproc
{

class OrientationResult
{
public:
    OrientationResult() : m_angle(0), m_confidence(0.0), m_decided(false) {}

    OrientationResult(int angle, double confidence, bool decided, QString const& reason)
        : m_angle(angle), m_confidence(confidence), m_decided(decided), m_reason(reason) {}

    // A clockwise angle matches OrthogonalRotation and the CLI options, so callers need no conversion.
    int angle() const
    {
        return m_angle;
    }

    double confidence() const
    {
        return m_confidence;
    }

    // A page without a decision must keep its current rotation.
    bool isDecided() const
    {
        return m_decided;
    }

    QString const& reason() const
    {
        return m_reason;
    }

    // The cue counts help to tune the thresholds, but the user interface does not show them.
    QString const& details() const
    {
        return m_details;
    }

    void setDetails(QString const& details)
    {
        m_details = details;
    }

    // The scan size tells a cover scan from the scans of the book pages, which have one size.
    QSize const& imageSize() const
    {
        return m_imageSize;
    }

    // For a scan without a decision: 0 for 0 or 180, 90 for 90 or 270, and -1 if the shape is unknown.
    int shapeAxis() const
    {
        return m_shapeAxis;
    }

    void setShape(QSize const& image_size, int shape_axis)
    {
        m_imageSize = image_size;
        m_shapeAxis = shape_axis;
    }
private:
    int m_angle;
    double m_confidence;
    bool m_decided;
    QString m_reason;
    QString m_details;
    QSize m_imageSize;
    int m_shapeAxis = -1;
};

class OrientationDetector
{
public:
    // The detector looks only at the pixels, thus the caller must remove a rotation that it applied before.
    static OrientationResult detect(QImage const& image);

    // The EXIF value is only a hint, because scanners and phones often write a wrong tag.
    static OrientationResult detect(QImage const& image, int exif_hint);

    // Returns the clockwise rotation that the EXIF or TIFF orientation tag asks for, or -1 without a tag that asks for a rotation.
    static int exifRotation(QString const& file_path);

    static OrientationResult combine(OrientationResult const& content, int exif_hint);

    // A sideways table in a book scan must not turn its scan across the rotation of most other scans.
    static void applyBookRotation(std::vector<OrientationResult>& results);

    // The book rotation for one scan, so that the Fix Orientation stage can use the rule one scan at a time.
    static OrientationResult applyBookRotation(OrientationResult const& result, int book_angle, QSize const& book_size);

    // A fold across the middle shows an open spread, and its absence a single page; returns 0, 90 or -1 as shapeAxis().
    static int shapeAxis(QImage const& image);

    static double confidenceThreshold();
};

} // namespace imageproc

#endif
