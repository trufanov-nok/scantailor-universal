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


#ifndef OUTPUT_GRAYSCALE_DETECTOR_H_
#define OUTPUT_GRAYSCALE_DETECTOR_H_

class QImage;
class QRectF;

namespace output
{

// Finds gray pictures in color scans, for example halftones on yellow paper, so that "Convert to grayscale first" can work automatically.
class GrayscaleDetector
{
public:
    // Gray photos in the test books measured about 7 and color plates 20 or more, so this limit has a margin.
    static double chromaThreshold()
    {
        return 10.0;
    }

    // A negative result means that the area has too few mid-tone pixels, because a pale or dark plate in color must not become gray.
    static double measureChroma(QImage const& image, QRectF const& area);

    static bool isGrayscale(double chroma)
    {
        return chroma >= 0.0 && chroma < chromaThreshold();
    }
};

} // namespace output

#endif
