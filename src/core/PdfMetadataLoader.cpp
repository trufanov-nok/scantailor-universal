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

    File: PdfMetadataLoader.cpp
    Copyright (C) mpcgd
    Purpose: Implementation file for the PdfMetadataLoader class to load metadata from PDF files using MuPDF.
    Interfaces: Implements the loadMetadata method from the base class.
    Dependencies: MuPDF library (mupdf/fitz.h), PdfReader, ImageMetadata, Dpi, Qt classes (QFile, QIODevice).
*/

#include "PdfMetadataLoader.h"
#include "PdfReader.h"
#include "ImageMetadata.h"
#include "Dpi.h"
#include <QFile>
#include <QIODevice>

// MuPDF headers
#include <mupdf/fitz.h>

void
PdfMetadataLoader::registerMyself()
{
    static bool registered = false;
    if (!registered) {
        ImageMetadataLoader::registerLoader(
            IntrusivePtr<ImageMetadataLoader>(new PdfMetadataLoader)
        );
        registered = true;
    }
}

ImageMetadataLoader::Status
PdfMetadataLoader::loadMetadata(
    QIODevice& io_device,
    VirtualFunction1<void, ImageMetadata const&>& out)
{
    if (!PdfReader::canRead(io_device)) {
        return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
    }

    try {
        // Reset device position
        io_device.seek(0);
        QByteArray pdfData = io_device.readAll();

        fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
        if (!ctx) {
            return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
        }

        fz_register_document_handlers(ctx);

        fz_stream* stream = fz_open_memory(ctx, reinterpret_cast<unsigned char*>(pdfData.data()), pdfData.size());
        fz_document* doc = fz_open_document_with_stream(ctx, "application/pdf", stream);

        if (!doc || fz_needs_password(ctx, doc)) {
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
        }

        // Get metadata from all pages
        int numPages = fz_count_pages(ctx, doc);
        for (int i = 0; i < numPages; ++i) {
            fz_page* page = fz_load_page(ctx, doc, i);
            if (page) {
                fz_rect bounds = fz_bound_page(ctx, page);
                QSizeF pageSize(bounds.x1 - bounds.x0, bounds.y1 - bounds.y0);
                
                // Convert from points to pixels at 72 DPI
                QSize size(qRound(pageSize.width()), qRound(pageSize.height()));
                
                // Use the same DPI calculation as PdfReader
                Dpi dpi = PdfReader::getDpi(pageSize);

                ImageMetadata metadata(size, dpi);
                out(metadata);

                fz_drop_page(ctx, page);
            }
        }

        fz_drop_document(ctx, doc);
        fz_drop_stream(ctx, stream);
        fz_drop_context(ctx);
        return ImageMetadataLoader::LOADED;

    } catch (...) {
        return ImageMetadataLoader::GENERIC_ERROR;
    }
}
