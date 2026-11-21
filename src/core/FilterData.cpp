/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "FilterData.h"
#include "Dpm.h"
#include "Dpi.h"
#include "imageproc/Grayscale.h"

using namespace imageproc;

FilterData::FilterData(QString const& filename, QImage const& image)
    :   m_origImageFilename(filename),
        m_origImage(image),
        m_grayImage(toGrayscale(m_origImage)),
        m_xform(image.rect(), Dpm(image)),
        m_bwThreshold(BinaryThreshold::otsuThreshold(m_grayImage)),
        m_blackOnWhite(true)
{
}

FilterData::FilterData(FilterData const& other, ImageTransformation const& xform)
    :   m_origImageFilename(other.m_origImageFilename),
        m_origImage(other.m_origImage),
        m_grayImage(other.m_grayImage),
        m_xform(xform),
        m_bwThreshold(other.m_bwThreshold),
        m_blackOnWhite(other.m_blackOnWhite)
{
}

static GrayImage invertGray(GrayImage const& src)
{
    GrayImage dst(src);
    int const width = dst.width();
    int const height = dst.height();
    int const stride = dst.stride();
    uint8_t* line = dst.data();
    for (int y = 0; y < height; ++y, line += stride) {
        for (int x = 0; x < width; ++x) {
            line[x] = static_cast<uint8_t>(255 - line[x]);
        }
    }
    return dst;
}

static QImage invertImage(QImage const& src)
{
    QImage dst(src);
    if (dst.format() == QImage::Format_Indexed8 && dst.colorTable() != createGrayscalePalette()) {
        // Inversion of the palette indices is correct only for the standard grayscale palette.
        dst = dst.convertToFormat(dst.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    }
    dst.invertPixels();
    return dst;
}

BinaryThreshold
FilterData::bwThresholdBlackOnWhite() const
{
    return m_blackOnWhite ? m_bwThreshold : BinaryThreshold(256 - int(m_bwThreshold));
}

GrayImage
FilterData::grayImageBlackOnWhite() const
{
    return m_blackOnWhite ? m_grayImage : invertGray(m_grayImage);
}

FilterData
FilterData::inverted() const
{
    FilterData data(*this);
    data.m_origImage = invertImage(m_origImage);
    data.m_grayImage = invertGray(m_grayImage);
    data.m_bwThreshold = BinaryThreshold(256 - int(m_bwThreshold));
    data.m_blackOnWhite = !m_blackOnWhite;
    return data;
}
