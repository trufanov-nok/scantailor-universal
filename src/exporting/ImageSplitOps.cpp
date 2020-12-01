/*
    Scan Tailor Universal - Interactive post-processing tool for scanned
    pages. A fork of Scan Tailor by Joseph Artsimovich.
    Copyright (C) 2020 Alexander Trufanov <trufanovan@gmail.com>

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

#include "ImageSplitOps.h"

namespace exporting {

void ImageSplitOps::initSplitImage(const QImage& source_img, QImage& target_img, QImage::Format format, bool grayscale_allowed)
{
    target_img = QImage(source_img.width(), source_img.height(), format);
    target_img.setDotsPerMeterX(source_img.dotsPerMeterX());
    target_img.setDotsPerMeterY(source_img.dotsPerMeterY());

    if (grayscale_allowed) {
        if (format == QImage::Format_Indexed8) {
            QVector<QRgb> palette(256);
            for (int i = 0; i < 256; ++i) {
                palette[i] = qRgb(i, i, i);
            }
            target_img.setColorTable(palette);
        }
    } else {
        QVector<QRgb> bw_palette(2);
        bw_palette[0] = qRgb(0, 0, 0);
        bw_palette[1] = qRgb(255, 255, 255);
        target_img.setColorTable(bw_palette);
    }
}


QImage ImageSplitOps::GenerateBlankImage(QImage& out_img, QImage::Format format, uint pixel)
{
    QImage blank_img = QImage(out_img.width(), out_img.height(), format);

    blank_img.setDotsPerMeterX(out_img.dotsPerMeterX());
    blank_img.setDotsPerMeterY(out_img.dotsPerMeterY());

    blank_img.fill(pixel);

    if (format == QImage::Format_Mono) {
        QVector<QRgb> bw_palette(2);
        bw_palette[0] = qRgb(0, 0, 0);
        bw_palette[1] = qRgb(255, 255, 255);
        blank_img.setColorTable(bw_palette);

        return blank_img;
    }

    else if (format == QImage::Format_Indexed8) {
        QVector<QRgb> palette(256);
        for (int i = 0; i < 256; ++i) {
            palette[i] = qRgb(i, i, i);
        }
        blank_img.setColorTable(palette);

        return blank_img;
    } else {
        return blank_img;
    }
}


}
