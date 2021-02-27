/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2021  Alexander Trufanov <trufanovan@gmail.com>

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

#ifndef JP2READER_H_
#define JP2READER_H_

#include "ImageMetadataLoader.h"
#include "VirtualFunction.h"

class QIODevice;
class QImage;
class ImageMetadata;
class Dpi;

class Jp2Reader
{
public:
    static bool canRead(QIODevice& device);

    static ImageMetadataLoader::Status readMetadata(
        QIODevice& device,
        VirtualFunction1<void, ImageMetadata const&>& out);

    /**
     * \brief Reads the image from io device to QImage.
     *
     * \param device The device to read from.  This device must be
     *        opened for reading and must be seekable.
     * \param page_num A zero-based page number within a multi-page
     *        TIFF file.
     * \return The resulting image, or a null image in case of failure.
     */
    static QImage readImage(QIODevice& device);
};

#endif
