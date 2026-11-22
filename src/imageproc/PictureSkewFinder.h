/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2026  ScanTailor Universal contributors

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

#ifndef IMAGEPROC_PICTURESKEWFINDER_H_
#define IMAGEPROC_PICTURESKEWFINDER_H_

namespace imageproc
{

class GrayImage;
class BinaryImage;

class PictureSkew
{
public:
    static double const MIN_PICTURE_FRACTION;
    static double const GOOD_CONFIDENCE;

    PictureSkew() : m_angle(0.0), m_pictureFraction(0.0), m_confidence(0.0) {}

    PictureSkew(double angle, double picture_fraction, double confidence)
        : m_angle(angle), m_pictureFraction(picture_fraction), m_confidence(confidence) {}

    // The angle turns the image clockwise, as the deskew angle of a page does.
    double angle() const
    {
        return m_angle;
    }

    double pictureFraction() const
    {
        return m_pictureFraction;
    }

    double confidence() const
    {
        return m_confidence;
    }

    bool isReliable() const
    {
        return m_pictureFraction >= MIN_PICTURE_FRACTION && m_confidence >= GOOD_CONFIDENCE;
    }
private:
    double m_angle;
    double m_pictureFraction;
    double m_confidence;
};

class PictureSkewFinder
{
public:
    static int const TARGET_DPI;

    static double const DEFAULT_MAX_ANGLE;

    // The image is black on white at about TARGET_DPI, and only the black pixels of the mask are part of the page.
    static PictureSkew findSkew(
        GrayImage const& image, BinaryImage const& page_mask,
        double max_angle = DEFAULT_MAX_ANGLE);
};

} // namespace imageproc

#endif
