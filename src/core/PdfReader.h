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

    File: PdfReader.h
    Copyright (C) mpcgd
    Purpose: Header file defining the PdfReader class for reading and extracting content from PDF documents.
    Interfaces: Static public methods including canRead, readMetadata, readImage, getPageCount, extractEmbeddedImages, isImageOnlyPage.
    Dependencies: QIODevice, QImage, QString, QList, ImageMetadata, Dpi, VirtualFunction.
*/

#ifndef PDFREADER_H_
#define PDFREADER_H_

#include "ImageMetadataLoader.h"
#include "VirtualFunction.h"
#include <QSize>
#include <QString>

class QIODevice;
class QImage;
class ImageMetadata;
class Dpi;

class PdfReader
{
public:
    static bool canRead(QIODevice& device);

    static ImageMetadataLoader::Status readMetadata(
        QIODevice& device,
        VirtualFunction1<void, ImageMetadata const&>& out);

    /**
     * \brief Reads a specific page from a PDF file to QImage.
     *
     * \param device The device to read from.  This device must be
     *        opened for reading and must be seekable.
     * \param page_num A zero-based page number within the PDF file.
     * \return The resulting image, or a null image in case of failure.
     */
    static QImage readImage(QIODevice& device, int page_num = 0);

    /**
     * \brief Get the total number of pages in the PDF.
     *
     * \param device The device to read from.
     * \return Number of pages, or -1 if unable to determine.
     */
    static int getPageCount(QIODevice& device);

    /**
     * \brief Extract embedded images from a specific page.
     *
     * \param device The device to read from.
     * \param page_num A zero-based page number within the PDF file.
     * \return List of embedded images found on the page.
     */
    static QList<QImage> extractEmbeddedImages(QIODevice& device, int page_num = 0);

    /**
     * \brief Check if a page contains only embedded images.
     *
     * \param device The device to read from.
     * \param page_num A zero-based page number within the PDF file.
     * \return True if the page appears to contain only embedded images.
     */
    static bool isImageOnlyPage(QIODevice& device, int page_num = 0);

public:
    static Dpi getDpi(QSizeF const& sizeF);

private:
    static bool isPdfFile(QIODevice& device);
};

#endif
