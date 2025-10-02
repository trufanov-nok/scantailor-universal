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

    File: PdfReader.cpp
    Copyright (C) mpcgd
    Purpose: Implementation of PdfReader class methods for reading and extracting content from PDF documents using MuPDF.
    Interfaces: Concrete implementations of the static methods declared in PdfReader.h.
    Dependencies: PdfReader.h, ImageMetadata, Dpi, Dpm, MuPDF library headers (mupdf/fitz.h, mupdf/pdf.h), Qt headers (QIODevice, QImage, etc.).
*/

#include "PdfReader.h"
#include "ImageMetadata.h"
#include "Dpi.h"
#include "Dpm.h"
#include <QIODevice>
#include <QImage>
#include <QSize>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QDebug>
#include <QFile>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include <QBuffer>

namespace {

void copyPixmapToQImage(unsigned char* samples, int width, int height, int stride, int n, QImage& image) {
    for (int y = 0; y < height; ++y) {
        unsigned char* row = samples + y * stride;
        for (int x = 0; x < width; ++x) {
            unsigned char* pixel = row + x * n;
            unsigned char r = pixel[0];
            unsigned char g = pixel[1];
            unsigned char b = pixel[2];
            image.setPixel(x, y, qRgb(r, g, b));
        }
    }
}

}

// Helper function to create appropriate stream (file-based or memory-based)
static fz_stream* createStream(QIODevice& device, fz_context* ctx) {
    if (QFile* file = dynamic_cast<QFile*>(&device)) {
        qDebug() << "createStream: Using QFile, filename:" << file->fileName();
        if (file->exists() && file->isReadable()) {
            qDebug() << "createStream: File exists and readable, attempting fz_open_file";
            fz_stream* stream = fz_open_file(ctx, file->fileName().toUtf8().constData());
            if (stream) {
                qDebug() << "createStream: fz_open_file succeeded";
                return stream;
            } else {
                qDebug() << "createStream: fz_open_file failed";
            }
        } else {
            qDebug() << "createStream: File not exists/readable";
        }
        // Fall back to memory if file opening failed
        qDebug() << "createStream: Falling back to memory";
    } else {
        qDebug() << "createStream: Using memory (not QFile)";
    }
    device.seek(0);
    QByteArray pdfData = device.readAll();
    return fz_open_memory(ctx, reinterpret_cast<unsigned char*>(pdfData.data()), pdfData.size());
}

// Helper function to check if a page has annotations
static bool pageHasAnnotations(fz_context* ctx, fz_document* doc, int page_num) {
    try {
        // Cast to PDF document to access PDF-specific features
        pdf_document* pdf_doc = pdf_document_from_fz_document(ctx, doc);
        if (!pdf_doc) {
            return false;
        }

        pdf_page* pdf_page = pdf_load_page(ctx, pdf_doc, page_num);
        if (!pdf_page) {
            return false;
        }

        // Check if there are annotations by getting the annotation list
        pdf_annot* annot = pdf_first_annot(ctx, pdf_page);
        bool hasAnnotations = (annot != NULL);

        pdf_drop_page(ctx, pdf_page);
        return hasAnnotations;
    } catch (...) {
        return false;
    }
}

bool
PdfReader::canRead(QIODevice& device)
{
    return isPdfFile(device);
}



ImageMetadataLoader::Status
PdfReader::readMetadata(
    QIODevice& device,
    VirtualFunction1<void, ImageMetadata const&>& out)
{
    if (!isPdfFile(device)) {
        return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
    }

    try {
        fz_context* ctx;
        fz_document* doc;

        ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
        if (!ctx) {
            return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
        }

        fz_register_document_handlers(ctx);

        fz_stream* stream = createStream(device, ctx);
        doc = fz_open_document_with_stream(ctx, "application/pdf", stream);

        if (!doc || fz_needs_password(ctx, doc)) {
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
        }

        int num_pages = fz_count_pages(ctx, doc);
        if (num_pages > 0) {
            fz_page* page = fz_load_page(ctx, doc, 0);
            fz_rect bounds = fz_bound_page(ctx, page);
            QSizeF pageSize(bounds.x1 - bounds.x0, bounds.y1 - bounds.y0);

            // Convert from points to pixels at 72 DPI
            QSize size(qRound(pageSize.width()), qRound(pageSize.height()));

            // Estimate DPI based on page size
            Dpi dpi = getDpi(pageSize);

            ImageMetadata metadata(size, dpi);
            out(metadata);

            fz_drop_page(ctx, page);
        }

        fz_drop_document(ctx, doc);
        fz_drop_stream(ctx, stream);
        fz_drop_context(ctx);
        return ImageMetadataLoader::LOADED;

    } catch (...) {
        return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
    }
}

QImage
PdfReader::readImage(QIODevice& device, int page_num)
{
#ifdef DEBUG
    qDebug() << "PdfReader::readImage called with page_num:" << page_num;
#endif
    if (!isPdfFile(device)) {
#ifdef DEBUG
        qDebug() << "Not a PDF file";
#endif
        return QImage();
    }

    try {
        fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
        if (!ctx) {
#ifdef DEBUG
            qDebug() << "Failed to create MuPDF context";
#endif
            return QImage();
        }

        fz_register_document_handlers(ctx);

        fz_stream* stream = createStream(device, ctx);
        fz_document* doc = fz_open_document_with_stream(ctx, "application/pdf", stream);

        if (!doc || fz_needs_password(ctx, doc)) {
#ifdef DEBUG
            qDebug() << "Failed to open PDF document or document is password protected";
#endif
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return QImage();
        }

        int numPages = fz_count_pages(ctx, doc);
#ifdef DEBUG
        qDebug() << "PDF has" << numPages << "pages";
#endif
        if (page_num < 0 || page_num >= numPages) {
#ifdef DEBUG
            qDebug() << "Invalid page number:" << page_num;
#endif
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return QImage();
        }

        // Check if the page has annotations - if it does, render to include them
        bool hasAnnotations = pageHasAnnotations(ctx, doc, page_num);
        if (hasAnnotations) {
#ifdef DEBUG
            qDebug() << "Page" << page_num << "has annotations, rendering to include them";
#endif
        } else if (isImageOnlyPage(device, page_num)) {
#ifdef DEBUG
            qDebug() << "Page" << page_num << "is image-only and has no annotations, attempting embedded image extraction";
#endif
            QList<QImage> embeddedImages = extractEmbeddedImages(device, page_num);
            if (embeddedImages.size() == 1) {
#ifdef DEBUG
                qDebug() << "Successfully extracted single embedded image, size:" << embeddedImages.first().size();
#endif
                fz_drop_document(ctx, doc);
                fz_drop_stream(ctx, stream);
                fz_drop_context(ctx);
                return embeddedImages.first();
            } else if (!embeddedImages.isEmpty()) {
#ifdef DEBUG
                qDebug() << "Found" << embeddedImages.size() << "embedded images, rendering instead";
#endif
            }
            // Fall back to rendering if extraction failed
#ifdef DEBUG
            qDebug() << "Embedded image extraction failed, falling back to rendering";
#endif
        } else {
#ifdef DEBUG
            qDebug() << "Rendering page to image (has text content or annotations)";
#endif
        }

        fz_page* page = fz_load_page(ctx, doc, page_num);
        if (!page) {
#ifdef DEBUG
            qDebug() << "Failed to load page";
#endif
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return QImage();
        }

        fz_rect bounds = fz_bound_page(ctx, page);
        QSizeF pageSize(bounds.x1 - bounds.x0, bounds.y1 - bounds.y0);
        Dpi dpi = getDpi(pageSize);

        // Create Pixmap for rendering
        fz_matrix ctm = fz_scale(dpi.horizontal() / 72.0f, dpi.vertical() / 72.0f);
        fz_pixmap* pix = fz_new_pixmap_from_page(ctx, page, ctm, fz_device_rgb(ctx), 0);

        if (!pix) {
#ifdef DEBUG
            qDebug() << "Failed to render page to pixmap";
#endif
            fz_drop_page(ctx, page);
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return QImage();
        }

        // Render annotations on top of the pixmap
        fz_device* dev = fz_new_draw_device(ctx, ctm, pix);
        fz_run_page_annots(ctx, page, dev, ctm, NULL);
        fz_close_device(ctx, dev);
        fz_drop_device(ctx, dev);

        // Create QImage from pixmap data
        int width = pix->w;
        int height = pix->h;
        int stride = pix->stride;
        int n = pix->n;
        QImage image(width, height, QImage::Format_RGB888);

        unsigned char* samples = fz_pixmap_samples(ctx, pix);
        copyPixmapToQImage(samples, width, height, stride, n, image);

        fz_drop_pixmap(ctx, pix);
        fz_drop_page(ctx, page);
        fz_drop_document(ctx, doc);
        fz_drop_stream(ctx, stream);
        fz_drop_context(ctx);

#ifdef DEBUG
        qDebug() << "Page rendered successfully, image size:" << image.size();
#endif
        return image;

    } catch (std::exception& e) {
#ifdef DEBUG
        qDebug() << "Exception in PdfReader::readImage:" << e.what();
#endif
        return QImage();
    } catch (...) {
#ifdef DEBUG
        qDebug() << "Unknown exception in PdfReader::readImage";
#endif
        return QImage();
    }
}

int
PdfReader::getPageCount(QIODevice& device)
{
    if (!isPdfFile(device)) {
        return -1;
    }

    try {
        fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
        if (!ctx) {
            return -1;
        }

        fz_register_document_handlers(ctx);

        fz_stream* stream = createStream(device, ctx);
        fz_document* doc = fz_open_document_with_stream(ctx, "application/pdf", stream);

        if (!doc || fz_needs_password(ctx, doc)) {
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return -1;
        }

        int numPages = fz_count_pages(ctx, doc);

        fz_drop_document(ctx, doc);
        fz_drop_stream(ctx, stream);
        fz_drop_context(ctx);

        return numPages;

    } catch (...) {
        return -1;
    }
}

bool
PdfReader::isPdfFile(QIODevice& device)
{
    const int headerSize = 8;
    qint64 originalPos = device.pos(); // Save current position
    device.seek(0); // Seek to beginning to read header
    QByteArray header = device.read(headerSize);
    if (header.size() < headerSize) {
#ifdef DEBUG
        qDebug() << "PDF header too short, size:" << header.size();
#endif
        device.seek(originalPos); // Reset position
        return false;
    }

    // Check for PDF header: %PDF-
#ifdef DEBUG
    qDebug() << "PDF header bytes:" << header.toHex();
#endif
    if (header.startsWith("%PDF-")) {
#ifdef DEBUG
        qDebug() << "Valid PDF header detected";
#endif
        device.seek(originalPos); // Reset position
        return true;
    }

#ifdef DEBUG
    qDebug() << "Invalid PDF header";
#endif
    device.seek(originalPos); // Reset position
    return false;
}

Dpi
PdfReader::getDpi(QSizeF const& sizeF)
{
    // Convert page size from points to inches (1 point = 1/72 inch)
    double widthInches = sizeF.width() / 72.0;
    double heightInches = sizeF.height() / 72.0;

    // Target pixel range: 4000 to 11000 pixels for both dimensions
    const int minPixels = 4000;
    const int maxPixels = 11000;

    // Standard DPI options
    const double standardDpis[] = {1200.0, 600.0, 300.0, 150.0, 72.0};

    double bestDpi = 600.0; // Default fallback
    int bestWidth = 0;
    int bestHeight = 0;
    double bestFitness = 1e9; // Lower is better

    // Try each standard DPI and find which gives pixel sizes in the target range
    for (double dpi : standardDpis) {
        // Calculate pixel dimensions at this DPI
        int pixelWidth = qRound(widthInches * dpi);
        int pixelHeight = qRound(heightInches * dpi);

        // Skip if outside the acceptable range
        if (pixelWidth < minPixels || pixelWidth > maxPixels ||
            pixelHeight < minPixels || pixelHeight > maxPixels) {
            continue;
        }

        // Calculate fitness (how close to the center of the range we are)
        // Prefer DPI that centers both dimensions in the middle of the range
        double widthCenterOffset = qAbs(pixelWidth - (minPixels + maxPixels) / 2.0);
        double heightCenterOffset = qAbs(pixelHeight - (minPixels + maxPixels) / 2.0);
        double fitness = widthCenterOffset + heightCenterOffset;

        if (fitness < bestFitness) {
            bestFitness = fitness;
            bestDpi = dpi;
            bestWidth = pixelWidth;
            bestHeight = pixelHeight;
        }
    }

    // If no DPI gave pixel sizes in range, find the one that gives the smallest pixel size >= minPixels
    if (bestFitness == 1e9) {
        bestDpi = 72.0; // Lowest DPI as fallback
        bestFitness = 1e9;

        for (double dpi : standardDpis) {
            int pixelWidth = qRound(widthInches * dpi);
            int pixelHeight = qRound(heightInches * dpi);

            // Only consider DPI that gives at least minimum pixels
            if (pixelWidth < minPixels || pixelHeight < minPixels) {
                continue;
            }

            // Prefer smaller pixel sizes when overshooting minimum
            double overshoot = (pixelWidth + pixelHeight) - (minPixels * 2);
            if (overshoot < bestFitness) {
                bestFitness = overshoot;
                bestDpi = dpi;
                bestWidth = pixelWidth;
                bestHeight = pixelHeight;
            }
        }

        // If still no good option found, use the highest DPI that fits
        if (bestFitness == 1e9) {
            for (int i = 0; i < 5; ++i) {
                double dpi = standardDpis[i];
                int pixelWidth = qRound(widthInches * dpi);
                int pixelHeight = qRound(heightInches * dpi);

                // Take the first DPI that gives reasonable size
                if (pixelWidth >= 100 && pixelHeight >= 100) {
                    bestDpi = dpi;
                    bestWidth = pixelWidth;
                    bestHeight = pixelHeight;
                    break;
                }
            }
        }
    }

    qDebug() << "PDF page size:" << sizeF.width() << "x" << sizeF.height() << "points"
             << "(" << widthInches << "x" << heightInches << "inches)"
             << "- selected DPI:" << bestDpi
             << "-> pixel size:" << bestWidth << "x" << bestHeight;

    return Dpi(bestDpi, bestDpi);
}



QList<QImage>
PdfReader::extractEmbeddedImages(QIODevice& device, int page_num)
{
    QList<QImage> images;

    if (!isPdfFile(device)) {
#ifdef DEBUG
        qDebug() << "Not a PDF file";
#endif
        return images;
    }

    try {
        fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
        if (!ctx) {
#ifdef DEBUG
            qDebug() << "Failed to create MuPDF context";
#endif
            return images;
        }

        fz_register_document_handlers(ctx);

        fz_stream* stream = createStream(device, ctx);
        fz_document* doc = fz_open_document_with_stream(ctx, "application/pdf", stream);

        if (!doc || fz_needs_password(ctx, doc)) {
#ifdef DEBUG
            qDebug() << "Failed to open PDF document or document is password protected";
#endif
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return images;
        }

        if (page_num < 0 || page_num >= fz_count_pages(ctx, doc)) {
#ifdef DEBUG
            qDebug() << "Invalid page number:" << page_num;
#endif
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return images;
        }

        // Cast to PDF document to access PDF objects
        pdf_document* pdf_doc = pdf_document_from_fz_document(ctx, doc);
        if (!pdf_doc) {
#ifdef DEBUG
            qDebug() << "Failed to get PDF document";
#endif
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return images;
        }

        pdf_page* pdf_page = pdf_load_page(ctx, pdf_doc, page_num);
        if (!pdf_page) {
#ifdef DEBUG
            qDebug() << "Failed to load PDF page" << page_num;
#endif
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return images;
        }

        // Get page resources
        pdf_obj* resources = pdf_page_resources(ctx, pdf_page);
        if (!resources) {
#ifdef DEBUG
            qDebug() << "No resources found on page" << page_num;
#endif
            pdf_drop_page(ctx, pdf_page);
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return images;
        }

        // Get XObject dictionary
        pdf_obj* xobj_dict = pdf_dict_get(ctx, resources, PDF_NAME(XObject));
        if (!xobj_dict || !pdf_is_dict(ctx, xobj_dict)) {
#ifdef DEBUG
            qDebug() << "No XObject dictionary found on page" << page_num;
#endif
            pdf_drop_page(ctx, pdf_page);
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return images;
        }

        // Iterate through all XObjects
        int len = pdf_dict_len(ctx, xobj_dict);
        for (int i = 0; i < len; ++i) {
            pdf_obj* key = pdf_dict_get_key(ctx, xobj_dict, i);
            pdf_obj* xobj = pdf_dict_get_val(ctx, xobj_dict, i);

            if (!pdf_is_stream(ctx, xobj)) {
                continue;
            }

            // Check if it's an image
            pdf_obj* subtype = pdf_dict_get(ctx, xobj, PDF_NAME(Subtype));
            if (!subtype || !pdf_name_eq(ctx, subtype, PDF_NAME(Image))) {
                continue;
            }

#ifdef DEBUG
            qDebug() << "Found embedded image XObject:" << pdf_to_str_buf(ctx, key);
#endif

            try {
                // Load image using MuPDF's built-in image loading (similar to PyMuPDF's extract_image)
                fz_image* img = pdf_load_image(ctx, pdf_doc, xobj);
                if (!img) {
#ifdef DEBUG
                    qDebug() << "Failed to load image from XObject";
#endif
                    continue;
                }

                // Convert image to pixmap for pixel access
                fz_pixmap* pix = fz_get_pixmap_from_image(ctx, img, NULL, NULL, NULL, NULL);
                if (!pix) {
#ifdef DEBUG
                    qDebug() << "Failed to create pixmap from image";
#endif
                    fz_drop_image(ctx, img);
                    continue;
                }

                // Create QImage from pixmap data
                QImage image(pix->w, pix->h, QImage::Format_RGB888);
                if (!image.isNull()) {
                    copyPixmapToQImage((unsigned char*)pix->samples, pix->w, pix->h, pix->stride, pix->n, image);
#ifdef DEBUG
                    qDebug() << "Extracted embedded image, size:" << image.size();
#endif
                    images << image;
                }

                fz_drop_pixmap(ctx, pix);
                fz_drop_image(ctx, img);

            } catch (std::exception& e) {
#ifdef DEBUG
                qDebug() << "Error extracting XObject image:" << e.what();
#endif
            }
        }

        // NOTE: Inline image extraction would require parsing the content stream
        // for BI/EI sequences and decoding the embedded image data. This is complex
        // and beyond the scope of basic PDF processing. The implementation currently
        // focuses on XObject images which cover most common embedded image usage.

#ifdef DEBUG
        qDebug() << "Total embedded images found:" << images.size();
#endif

        pdf_drop_page(ctx, pdf_page);
        fz_drop_document(ctx, doc);
        fz_drop_stream(ctx, stream);
        fz_drop_context(ctx);

        return images;

    } catch (std::exception& e) {
#ifdef DEBUG
        qDebug() << "Exception in extractEmbeddedImages:" << e.what();
#endif
        return images;
    } catch (...) {
#ifdef DEBUG
        qDebug() << "Unknown exception in extractEmbeddedImages";
#endif
        return images;
    }
}

bool
PdfReader::isImageOnlyPage(QIODevice& device, int page_num)
{
    if (!isPdfFile(device)) {
        return false;
    }

    try {
        fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
        if (!ctx) {
            return false;
        }

        fz_register_document_handlers(ctx);

        fz_stream* stream = createStream(device, ctx);
        fz_document* doc = fz_open_document_with_stream(ctx, "application/pdf", stream);

        if (!doc || fz_needs_password(ctx, doc)) {
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return false;
        }

        if (page_num < 0 || page_num >= fz_count_pages(ctx, doc)) {
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return false;
        }

        fz_page* page = fz_load_page(ctx, doc, page_num);
        if (!page) {
            fz_drop_document(ctx, doc);
            fz_drop_stream(ctx, stream);
            fz_drop_context(ctx);
            return false;
        }

        // Check for text content using MuPDF's text extraction
        fz_stext_options opts = { 0 };
        fz_stext_page* text_page = fz_new_stext_page_from_page(ctx, page, &opts);
        bool hasText = false;
        if (text_page) {
            fz_stext_block* block = text_page->first_block;
            while (block) {
                if (block->type == FZ_STEXT_BLOCK_TEXT) {
                    fz_stext_line* line = block->u.t.first_line;
                    while (line) {
                        fz_stext_char* ch = line->first_char;
                        while (ch) {
                            if (ch->c != ' ' && ch->c != '\t' && ch->c != '\n' && ch->c != '\r') {
                                hasText = true;
                                goto found_text2;
                            }
                            ch = ch->next;
                        }
                        line = line->next;
                    }
                }
                block = block->next;
            }
            found_text2:
            fz_drop_stext_page(ctx, text_page);
        }

        fz_drop_page(ctx, page);
        fz_drop_document(ctx, doc);
        fz_drop_stream(ctx, stream);
        fz_drop_context(ctx);

        // If no significant text content, consider it image-only
        bool isImageOnly = !hasText;
#ifdef DEBUG
        qDebug() << "Page" << page_num << "has text:" << hasText << "is image-only:" << isImageOnly;
#endif

        return isImageOnly;

    } catch (...) {
        return false;
    }
}
