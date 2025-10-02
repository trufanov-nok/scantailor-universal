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

    File: PdfMetadataLoader.h
    Copyright (C) mpcgd
    Purpose: Header file defining the PdfMetadataLoader class for loading metadata from PDF files.
    Interfaces: Inherits from ImageMetadataLoader, implements loadMetadata virtual method.
    Dependencies: ImageMetadataLoader header, standard Qt includes for I/O.
*/

#ifndef PDFMETADATALOADER_H_
#define PDFMETADATALOADER_H_

#include "ImageMetadataLoader.h"

class PdfMetadataLoader : public ImageMetadataLoader
{
public:
    static void registerMyself();
protected:
    virtual Status loadMetadata(
        QIODevice& io_device,
        VirtualFunction1<void, ImageMetadata const&>& out);
};

#endif
