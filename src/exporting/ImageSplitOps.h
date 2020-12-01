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

#ifndef IMAGESPLITOPS_H
#define IMAGESPLITOPS_H

#include <QImage>

namespace exporting {

class ImageSplitOps
{
public:
    static void initSplitImage(const QImage& source_img, QImage& target_img, QImage::Format format, bool grayscale_allowed);
    static QImage GenerateBlankImage(QImage& out_img, QImage::Format format, uint pixel = 0xFFFFFFFF);

    template<typename MixedPixel>
    static bool GenerateSubscans(QImage& source_img, QImage* foreground_img, QImage* background_img, QImage* mask_img, bool keep_orig_fore_subscan, QImage* p_orig_fore_subscan)
    {

        if (!foreground_img && !background_img && !mask_img) {
            return false;
        }

        if (foreground_img) {
            QImage::Format format = keep_orig_fore_subscan ? source_img.format() : QImage::Format_Mono;
            initSplitImage(source_img, *foreground_img, format, keep_orig_fore_subscan);
        }

        if (background_img) {
            initSplitImage(source_img, *background_img, source_img.format(), true);
    //        background_img->fill(0xFFFFFFFF);
        }

        if (mask_img) {
            initSplitImage(source_img, *mask_img, QImage::Format_Mono, false);
    //        mask_img->fill(0x00000000);
        }

        MixedPixel* source_line = reinterpret_cast<MixedPixel*>(source_img.bits());
        int const source_stride = source_img.bytesPerLine() / sizeof(MixedPixel);

        MixedPixel* foreground_orig_line = nullptr;
        int foreground_orig_stride = 0;
        uchar* foreground_mono_line = nullptr;
        int foreground_mono_stride = 0;
        if (foreground_img) {
            foreground_orig_line = reinterpret_cast<MixedPixel*>(foreground_img->bits());
            foreground_orig_stride = foreground_img->bytesPerLine() / sizeof(MixedPixel);
            foreground_mono_line = foreground_img->bits();
            foreground_mono_stride = foreground_img->bytesPerLine();
        }

        uchar* mask_mono_line = nullptr;
        int  mask_mono_stride = 0;
        if (mask_img) {
            mask_mono_line = mask_img->bits();
            mask_mono_stride = mask_img->bytesPerLine();
        }

        MixedPixel* background_line = nullptr;
        int background_stride = 0;
        if (background_img) {
            background_line = reinterpret_cast<MixedPixel*>(background_img->bits());
            background_stride = background_img->bytesPerLine() / sizeof(MixedPixel);
        }

        MixedPixel* source_orig_line = nullptr;
        int source_orig_stride = 0;
        if (keep_orig_fore_subscan) {
            source_orig_line = reinterpret_cast<MixedPixel*>(p_orig_fore_subscan->bits());
            source_orig_stride = p_orig_fore_subscan->bytesPerLine() / sizeof(MixedPixel);
        }

        const MixedPixel white_pixel = static_cast<MixedPixel>((uint32_t) 0xffffffff);
        const MixedPixel mask_pixel = static_cast<MixedPixel>((uint32_t) 0x00ffffff);

        bool only_bw = true;

        for (int y = 0; y < source_img.height(); ++y) {
            for (int x = 0; x < source_img.width(); ++x) {
                //this line of code was suggested by Tulon:
                if ((source_line[x] & mask_pixel) == 0 || (source_line[x] & mask_pixel) == mask_pixel) {
                    // source_line[x] is pure black or pure white
                    if (keep_orig_fore_subscan && foreground_orig_line) {
                        foreground_orig_line[x] = source_orig_line[x];
                    } else if (!keep_orig_fore_subscan && foreground_mono_line) {
                        uint8_t value1 = static_cast<uint8_t>(source_line[x]);
                        // BW SetPixel from http://djvu-soft.narod.ru/bookscanlib/023.htm
                        // set white or black bit to mono image
                        value1 ? foreground_mono_line[x >> 3] |= (0x80 >> (x & 0x7)) : foreground_mono_line[x >> 3] &= (0xFF7F >> (x & 0x7));
                    }

                    if (background_line) {
                        background_line[x] = white_pixel;
                    }

                    if (mask_mono_line) {
    //                    uint8_t value1 = static_cast<uint8_t>(source_line[x]);
                        /*!value1 ? mask_mono_line[x >> 3] |= (0x80 >> (x & 0x7)) : */mask_mono_line[x >> 3] &= (0xFF7F >> (x & 0x7));
                    }

                } else {
                    only_bw = false;
                    // non-BW
                    if (keep_orig_fore_subscan && foreground_orig_line) {
                        foreground_orig_line[x] = white_pixel;
                    } else if (!keep_orig_fore_subscan && foreground_mono_line) {
                        // set white bit to mono image
                        foreground_mono_line[x >> 3] |= (0x80 >> (x & 0x7));
                    }

                    if (background_line) {
                        background_line[x] = source_line[x];
                    }

                    if (mask_mono_line) {
                        mask_mono_line[x >> 3] |= (0x80 >> (x & 0x7)); //white
                    }
                }
            }

            source_line += source_stride;
            if (background_line) {
                background_line += background_stride;
            }
            if (mask_mono_line) {
                mask_mono_line += mask_mono_stride;
            }
            if (keep_orig_fore_subscan) {
                if (foreground_orig_line) {
                    foreground_orig_line += foreground_orig_stride;
                }
                source_orig_line += source_orig_stride;
            } else if (foreground_mono_line) {
                foreground_mono_line += foreground_mono_stride;
            }
        }

        return only_bw;
    }
};

}
#endif // IMAGESPLITOPS_H
