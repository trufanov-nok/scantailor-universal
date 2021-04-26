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

#include "Jp2Reader.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "Dpi.h"
#include "Dpm.h"
#include <QtGlobal>
#include <QSysInfo>
#include <QIODevice>
#include <QImage>
#include <QColor>
#include <QSize>
#include <QDebug>
#include <algorithm>
#include <new>
#include <assert.h>
#include <cmath>

extern "C" {
#include <openjpeg.h>
}



// https://github.com/uclouvain/openjpeg/blob/master/src/bin/jp2/opj_decompress.c#L493
#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

bool
Jp2Reader::canRead(QIODevice& device)
{
    if (!device.isReadable() || device.isSequential()) {
        return false;
    }

    unsigned char header[12];
    static int const header_size = sizeof(header);

    if (device.peek((char*)header, header_size) != header_size) {
        return false;
    }

    if (memcmp(header, JP2_RFC3745_MAGIC, 12) == 0 ||
            memcmp(header, JP2_MAGIC, 4) == 0) {
        return true;
    } else if (memcmp(header, J2K_CODESTREAM_MAGIC, 4) == 0) {
        return true;
    }

    return false;
}


struct Jp2_resc
{
    Jp2_resc(char* mem)
    {
        VRcN = * (uint16_t*) mem; mem += sizeof (uint16_t);
        VRcD = * (uint16_t*) mem; mem += sizeof (uint16_t);
        HRcN = * (uint16_t*) mem; mem += sizeof (uint16_t);
        HRcD = * (uint16_t*) mem; mem += sizeof (uint16_t);
        VRcE = *mem; mem++;
        HRcE = *mem;
    }

    uint16_t VRcN;
    uint16_t VRcD;
    uint16_t HRcN;
    uint16_t HRcD;
    char VRcE;
    char HRcE;
};

Dpm lookforJP2Dpm(QIODevice& io_device)
{
    // libopenjp2 doesn't currently allow to read 'res '/'resc/'resd' boxes
    // that may change in future: https://github.com/uclouvain/openjpeg/issues/378
    // currently use ugly workaround.

    io_device.seek(0);
    char buf[128]; // assume that's enough
    io_device.peek(buf, sizeof(buf));

    int idx = QByteArray(buf, sizeof(buf)).indexOf("resc");
    if (idx != -1) {
        idx += 4;
        if ((sizeof(buf) - idx) >= (int)sizeof(Jp2_resc)) {
            Jp2_resc resc(buf+idx);
            int hDpm = qRound(( (double)resc.HRcN/resc.HRcD ) * std::pow(10, resc.HRcE));
            int vDpm = qRound(( (double)resc.VRcN/resc.VRcD ) * std::pow(10, resc.VRcE));
            if (hDpm > 0 && vDpm > 0) {
                return Dpm(hDpm, vDpm);
            }
        }
    }
    return Dpm();
}

Dpi lookforJP2Dpi(QIODevice& io_device)
{
    Dpm dpm = lookforJP2Dpm(io_device);
    if (!dpm.isNull()) {
        return Dpi(dpm);
    } else return Dpm();
}

OPJ_SIZE_T io_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
    OPJ_SIZE_T l_nb_read = ((QIODevice*) p_user_data)->read((char*)p_buffer, p_nb_bytes);
    return l_nb_read > 0 ? l_nb_read : (OPJ_SIZE_T) - 1;
}

OPJ_BOOL io_stream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
    return ((QIODevice*) p_user_data)->seek(p_nb_bytes);
}

void color_sycc_to_rgb(opj_image_t *img);
void color_cmyk_to_rgb(opj_image_t *img);
void color_esycc_to_rgb(opj_image_t *img);

ImageMetadataLoader::Status prepareMetadata(
        QIODevice& device,
        opj_stream_t** p_stream,
        opj_codec_t** p_codec,
        opj_image_t** p_image)
{

    opj_stream_t*& stream = *p_stream;
    opj_codec_t*& codec = *p_codec;
    opj_image_t*& image = *p_image;

    if (!device.isReadable() || device.isSequential()) {
        return ImageMetadataLoader::GENERIC_ERROR;
    }

    static int const header_size = 12;
    unsigned char header[header_size];

    if (device.peek((char*)header, header_size) != header_size) {
        return ImageMetadataLoader::GENERIC_ERROR;
    }

    OPJ_CODEC_FORMAT format = OPJ_CODEC_UNKNOWN;

    if (memcmp(header, JP2_RFC3745_MAGIC, 12) == 0 ||
            memcmp(header, JP2_MAGIC, 4) == 0) {
        format = OPJ_CODEC_JP2;
    } else if (memcmp(header, J2K_CODESTREAM_MAGIC, 4) == 0) {
        format = OPJ_CODEC_J2K;
    }

    if (format == OPJ_CODEC_UNKNOWN) {
        return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
    }

    stream = opj_stream_default_create(OPJ_TRUE);
    assert(stream);

    opj_stream_set_user_data(stream, (void *)&device, nullptr);
    opj_stream_set_user_data_length(stream, device.size());
    opj_stream_set_read_function(stream, io_stream_read);
    opj_stream_set_seek_function(stream, io_stream_seek);

    codec = opj_create_decompress (format);
    assert(codec);

    opj_set_info_handler(codec, [](const char *msg, void *){
        qDebug() << msg;
    }, nullptr);
    opj_set_warning_handler(codec, [](const char *msg, void *){
        qDebug() << msg;
    }, nullptr);
    opj_set_error_handler(codec, [](const char *msg, void *){
        qCritical() << msg;
    }, nullptr);

    // we don't set thread number for decoder - relying on defaults

    opj_dparameters_t  parameters;
    opj_set_default_decoder_parameters (&parameters);

    if (opj_setup_decoder (codec, &parameters) != OPJ_TRUE) {
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        return ImageMetadataLoader::GENERIC_ERROR;
    }

    image = nullptr;
    if (opj_read_header (stream, codec, &image) != OPJ_TRUE) {
        if (image) {
            opj_image_destroy(image);
        }
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        return ImageMetadataLoader::GENERIC_ERROR;
    }

    if (image->numcomps == 0) {
        return ImageMetadataLoader::NO_IMAGES;
    }

    // start color_space checks
    // https://github.com/uclouvain/openjpeg/blob/master/src/bin/jp2/opj_decompress.c#L1580
    if (image->color_space != OPJ_CLRSPC_SYCC
            && image->numcomps == 3 && image->comps[0].dx == image->comps[0].dy
            && image->comps[1].dx != 1) {
        image->color_space = OPJ_CLRSPC_SYCC;
    } else if (image->numcomps <= 2) {
        image->color_space = OPJ_CLRSPC_GRAY;
    }
    // end color_space checks



    return ImageMetadataLoader::LOADED;
}

ImageMetadataLoader::Status
Jp2Reader::readMetadata(
        QIODevice& device,
        VirtualFunction1<void, ImageMetadata const&>& out)
{
    opj_stream_t* stream = nullptr;
    opj_codec_t* codec = nullptr;
    opj_image_t* jp2_image = nullptr;
    ImageMetadataLoader::Status res =
            prepareMetadata(device, &stream, &codec, &jp2_image);
    if (res == ImageMetadataLoader::LOADED) {
        QSize const size(jp2_image->comps[0].w, jp2_image->comps[0].h); // that's what GIMP does - https://github.com/GNOME/gimp/blob/3b3404b03da30d1c534a85e74d23777d4c621853/plug-ins/common/file-jp2-load.c#L1295
        Dpi dpi = lookforJP2Dpi(device);
        out(ImageMetadata(size, dpi, jp2_image->color_space == OPJ_CLRSPC_GRAY));
    }

    if (jp2_image) {
        opj_image_destroy(jp2_image);
    }

    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    return res;
}

// openjpegToQImage() is copied from
// https://github.com/dept2/qtjp2-plugin/blob/master/src/QtJP2OpenJPEGImageHandler.cpp#L108
// under LGPL-2.1
QImage openjpegToQImage(opj_image_t* source)
{
    const int width = source->comps[0].w;
    const int height = source->comps[0].h;

    if (source->numcomps != 3 && source->numcomps != 1)
    {
        qWarning() << "Unsupported components count\n";
        return QImage();
    }

    QImage::Format format = (source->numcomps == 3) ? QImage::Format_RGB32 : QImage::Format_Indexed8;
    QImage image(width, height, format);
    if (format == QImage::Format_Indexed8)
    {
        // Initialize palette
        QVector<QRgb> colors;
        for (int i = 0; i <= 255; ++i)
            colors << qRgb(i, i, i);

        image.setColorTable(colors);
    }

    for (int y = 0; y < height; ++y)
    {
        uchar* line = image.scanLine(y);
        for (int x = 0; x < width; ++x)
        {
            uchar* pixel = (source->numcomps == 1) ? (line + x) : (line + (x * 4));
            if (source->numcomps == 1)
            {
                *pixel = source->comps[0].data[y * width + x];
            }
            else if (source->numcomps == 3)
            {
                pixel[2] = source->comps[0].data[y * width + x];
                pixel[1] = source->comps[1].data[y * width + x];
                pixel[0] = source->comps[2].data[y * width + x];
            }
        }
    }

    return image;
}

QImage
Jp2Reader::readImage(QIODevice& device)
{
    opj_stream_t* stream = nullptr;
    opj_codec_t* codec = nullptr;
    opj_image_t* jp2_image = nullptr;
    ImageMetadataLoader::Status res =
            prepareMetadata(device, &stream, &codec, &jp2_image);
    if (res != ImageMetadataLoader::LOADED) {
        if (jp2_image) {
            opj_image_destroy(jp2_image);
        }
        if (stream) {
            opj_stream_destroy(stream);
        }
        if (codec) {
            opj_destroy_codec(codec);
        }
        return QImage();
    }

    /* Get the decoded image */
    if (!(opj_decode(codec, stream, jp2_image) &&
          opj_end_decompress(codec,   stream))) {
        fprintf(stderr, "ERROR -> opj_decompress: failed to decode image!\n");
        if (jp2_image) {
            opj_image_destroy(jp2_image);
        }
        if (stream) {
            opj_stream_destroy(stream);
        }
        if (codec) {
            opj_destroy_codec(codec);
        }
        return QImage();
    }

    // start color_space checks
    // https://github.com/uclouvain/openjpeg/blob/master/src/bin/jp2/opj_decompress.c#L1588
    if (jp2_image->color_space == OPJ_CLRSPC_SYCC) {
        color_sycc_to_rgb(jp2_image);
    } else if (jp2_image->color_space == OPJ_CLRSPC_CMYK) {
        color_cmyk_to_rgb(jp2_image);
    } else if (jp2_image->color_space == OPJ_CLRSPC_EYCC) {
        color_esycc_to_rgb(jp2_image);
    }
    // end color_space checks
    // I don't copied icc_profile_buf checks as
    // I don't want to add dependency to liblcms2 or liblcms
    // just for such rare case

    QImage image = openjpegToQImage(jp2_image);

    opj_image_destroy(jp2_image);
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);

    if (image.isNull())
    {
        qWarning("Image not converted");
        return QImage();
    }

    Dpi dpm = lookforJP2Dpm(device);
    if (!dpm.isNull()) {
        image.setDotsPerMeterX(dpm.horizontal());
        image.setDotsPerMeterY(dpm.vertical());
    }

    return image;
}


/*
 * All functions and comments below were copied from openjpeg:
 * https://github.com/uclouvain/openjpeg/blob/master/src/bin/common/color.c
 *
 */




/*--------------------------------------------------------
Matrix for sYCC, Amendment 1 to IEC 61966-2-1
Y :   0.299   0.587    0.114   :R
Cb:  -0.1687 -0.3312   0.5     :G
Cr:   0.5    -0.4187  -0.0812  :B
Inverse:
R: 1        -3.68213e-05    1.40199      :Y
G: 1.00003  -0.344125      -0.714128     :Cb - 2^(prec - 1)
B: 0.999823  1.77204       -8.04142e-06  :Cr - 2^(prec - 1)
-----------------------------------------------------------*/
static void sycc_to_rgb(int offset, int upb, int y, int cb, int cr,
                        int *out_r, int *out_g, int *out_b)
{
    int r, g, b;

    cb -= offset;
    cr -= offset;
    r = y + (int)(1.402 * (float)cr);
    if (r < 0) {
        r = 0;
    } else if (r > upb) {
        r = upb;
    }
    *out_r = r;

    g = y - (int)(0.344 * (float)cb + 0.714 * (float)cr);
    if (g < 0) {
        g = 0;
    } else if (g > upb) {
        g = upb;
    }
    *out_g = g;

    b = y + (int)(1.772 * (float)cb);
    if (b < 0) {
        b = 0;
    } else if (b > upb) {
        b = upb;
    }
    *out_b = b;
}

static void sycc444_to_rgb(opj_image_t *img)
{
    int *d0, *d1, *d2, *r, *g, *b;
    const int *y, *cb, *cr;
    size_t maxw, maxh, max, i;
    int offset, upb;

    upb = (int)img->comps[0].prec;
    offset = 1 << (upb - 1);
    upb = (1 << upb) - 1;

    maxw = (size_t)img->comps[0].w;
    maxh = (size_t)img->comps[0].h;
    max = maxw * maxh;

    y = img->comps[0].data;
    cb = img->comps[1].data;
    cr = img->comps[2].data;

    d0 = r = (int*)opj_image_data_alloc(sizeof(int) * max);
    d1 = g = (int*)opj_image_data_alloc(sizeof(int) * max);
    d2 = b = (int*)opj_image_data_alloc(sizeof(int) * max);

    if (r == NULL || g == NULL || b == NULL) {
        goto fails;
    }

    for (i = 0U; i < max; ++i) {
        sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);
        ++y;
        ++cb;
        ++cr;
        ++r;
        ++g;
        ++b;
    }
    opj_image_data_free(img->comps[0].data);
    img->comps[0].data = d0;
    opj_image_data_free(img->comps[1].data);
    img->comps[1].data = d1;
    opj_image_data_free(img->comps[2].data);
    img->comps[2].data = d2;
    img->color_space = OPJ_CLRSPC_SRGB;
    return;

fails:
    opj_image_data_free(r);
    opj_image_data_free(g);
    opj_image_data_free(b);
}/* sycc444_to_rgb() */

static void sycc422_to_rgb(opj_image_t *img)
{
    int *d0, *d1, *d2, *r, *g, *b;
    const int *y, *cb, *cr;
    size_t maxw, maxh, max, offx, loopmaxw;
    int offset, upb;
    size_t i;

    upb = (int)img->comps[0].prec;
    offset = 1 << (upb - 1);
    upb = (1 << upb) - 1;

    maxw = (size_t)img->comps[0].w;
    maxh = (size_t)img->comps[0].h;
    max = maxw * maxh;

    y = img->comps[0].data;
    cb = img->comps[1].data;
    cr = img->comps[2].data;

    d0 = r = (int*)opj_image_data_alloc(sizeof(int) * max);
    d1 = g = (int*)opj_image_data_alloc(sizeof(int) * max);
    d2 = b = (int*)opj_image_data_alloc(sizeof(int) * max);

    if (r == NULL || g == NULL || b == NULL) {
        goto fails;
    }

    /* if img->x0 is odd, then first column shall use Cb/Cr = 0 */
    offx = img->x0 & 1U;
    loopmaxw = maxw - offx;

    for (i = 0U; i < maxh; ++i) {
        size_t j;

        if (offx > 0U) {
            sycc_to_rgb(offset, upb, *y, 0, 0, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;
        }

        for (j = 0U; j < (loopmaxw & ~(size_t)1U); j += 2U) {
            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;
            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;
            ++cb;
            ++cr;
        }
        if (j < loopmaxw) {
            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;
            ++cb;
            ++cr;
        }
    }

    opj_image_data_free(img->comps[0].data);
    img->comps[0].data = d0;
    opj_image_data_free(img->comps[1].data);
    img->comps[1].data = d1;
    opj_image_data_free(img->comps[2].data);
    img->comps[2].data = d2;

    img->comps[1].w = img->comps[2].w = img->comps[0].w;
    img->comps[1].h = img->comps[2].h = img->comps[0].h;
    img->comps[1].dx = img->comps[2].dx = img->comps[0].dx;
    img->comps[1].dy = img->comps[2].dy = img->comps[0].dy;
    img->color_space = OPJ_CLRSPC_SRGB;
    return;

fails:
    opj_image_data_free(r);
    opj_image_data_free(g);
    opj_image_data_free(b);
}/* sycc422_to_rgb() */

static void sycc420_to_rgb(opj_image_t *img)
{
    int *d0, *d1, *d2, *r, *g, *b, *nr, *ng, *nb;
    const int *y, *cb, *cr, *ny;
    size_t maxw, maxh, max, offx, loopmaxw, offy, loopmaxh;
    int offset, upb;
    size_t i;

    upb = (int)img->comps[0].prec;
    offset = 1 << (upb - 1);
    upb = (1 << upb) - 1;

    maxw = (size_t)img->comps[0].w;
    maxh = (size_t)img->comps[0].h;
    max = maxw * maxh;

    y = img->comps[0].data;
    cb = img->comps[1].data;
    cr = img->comps[2].data;

    d0 = r = (int*)opj_image_data_alloc(sizeof(int) * max);
    d1 = g = (int*)opj_image_data_alloc(sizeof(int) * max);
    d2 = b = (int*)opj_image_data_alloc(sizeof(int) * max);

    if (r == NULL || g == NULL || b == NULL) {
        goto fails;
    }

    /* if img->x0 is odd, then first column shall use Cb/Cr = 0 */
    offx = img->x0 & 1U;
    loopmaxw = maxw - offx;
    /* if img->y0 is odd, then first line shall use Cb/Cr = 0 */
    offy = img->y0 & 1U;
    loopmaxh = maxh - offy;

    if (offy > 0U) {
        size_t j;

        for (j = 0; j < maxw; ++j) {
            sycc_to_rgb(offset, upb, *y, 0, 0, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;
        }
    }

    for (i = 0U; i < (loopmaxh & ~(size_t)1U); i += 2U) {
        size_t j;

        ny = y + maxw;
        nr = r + maxw;
        ng = g + maxw;
        nb = b + maxw;

        if (offx > 0U) {
            sycc_to_rgb(offset, upb, *y, 0, 0, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;
            sycc_to_rgb(offset, upb, *ny, *cb, *cr, nr, ng, nb);
            ++ny;
            ++nr;
            ++ng;
            ++nb;
        }

        for (j = 0; j < (loopmaxw & ~(size_t)1U); j += 2U) {
            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;
            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;

            sycc_to_rgb(offset, upb, *ny, *cb, *cr, nr, ng, nb);
            ++ny;
            ++nr;
            ++ng;
            ++nb;
            sycc_to_rgb(offset, upb, *ny, *cb, *cr, nr, ng, nb);
            ++ny;
            ++nr;
            ++ng;
            ++nb;
            ++cb;
            ++cr;
        }
        if (j < loopmaxw) {
            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);
            ++y;
            ++r;
            ++g;
            ++b;

            sycc_to_rgb(offset, upb, *ny, *cb, *cr, nr, ng, nb);
            ++ny;
            ++nr;
            ++ng;
            ++nb;
            ++cb;
            ++cr;
        }
        y += maxw;
        r += maxw;
        g += maxw;
        b += maxw;
    }
    if (i < loopmaxh) {
        size_t j;

        for (j = 0U; j < (maxw & ~(size_t)1U); j += 2U) {
            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);

            ++y;
            ++r;
            ++g;
            ++b;

            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);

            ++y;
            ++r;
            ++g;
            ++b;
            ++cb;
            ++cr;
        }
        if (j < maxw) {
            sycc_to_rgb(offset, upb, *y, *cb, *cr, r, g, b);
        }
    }

    opj_image_data_free(img->comps[0].data);
    img->comps[0].data = d0;
    opj_image_data_free(img->comps[1].data);
    img->comps[1].data = d1;
    opj_image_data_free(img->comps[2].data);
    img->comps[2].data = d2;

    img->comps[1].w = img->comps[2].w = img->comps[0].w;
    img->comps[1].h = img->comps[2].h = img->comps[0].h;
    img->comps[1].dx = img->comps[2].dx = img->comps[0].dx;
    img->comps[1].dy = img->comps[2].dy = img->comps[0].dy;
    img->color_space = OPJ_CLRSPC_SRGB;
    return;

fails:
    opj_image_data_free(r);
    opj_image_data_free(g);
    opj_image_data_free(b);
}/* sycc420_to_rgb() */

void color_sycc_to_rgb(opj_image_t *img)
{
    if (img->numcomps < 3) {
        img->color_space = OPJ_CLRSPC_GRAY;
        return;
    }

    if ((img->comps[0].dx == 1)
            && (img->comps[1].dx == 2)
            && (img->comps[2].dx == 2)
            && (img->comps[0].dy == 1)
            && (img->comps[1].dy == 2)
            && (img->comps[2].dy == 2)) { /* horizontal and vertical sub-sample */
        sycc420_to_rgb(img);
    } else if ((img->comps[0].dx == 1)
               && (img->comps[1].dx == 2)
               && (img->comps[2].dx == 2)
               && (img->comps[0].dy == 1)
               && (img->comps[1].dy == 1)
               && (img->comps[2].dy == 1)) { /* horizontal sub-sample only */
        sycc422_to_rgb(img);
    } else if ((img->comps[0].dx == 1)
               && (img->comps[1].dx == 1)
               && (img->comps[2].dx == 1)
               && (img->comps[0].dy == 1)
               && (img->comps[1].dy == 1)
               && (img->comps[2].dy == 1)) { /* no sub-sample */
        sycc444_to_rgb(img);
    } else {
        fprintf(stderr, "%s:%d:color_sycc_to_rgb\n\tCAN NOT CONVERT\n", __FILE__,
                __LINE__);
        return;
    }
}/* color_sycc_to_rgb() */

void color_cmyk_to_rgb(opj_image_t *image)
{
    float C, M, Y, K;
    float sC, sM, sY, sK;
    unsigned int w, h, max, i;

    w = image->comps[0].w;
    h = image->comps[0].h;

    if (
            (image->numcomps < 4)
            || (image->comps[0].dx != image->comps[1].dx) ||
            (image->comps[0].dx != image->comps[2].dx) ||
            (image->comps[0].dx != image->comps[3].dx)
            || (image->comps[0].dy != image->comps[1].dy) ||
            (image->comps[0].dy != image->comps[2].dy) ||
            (image->comps[0].dy != image->comps[3].dy)
            ) {
        fprintf(stderr, "%s:%d:color_cmyk_to_rgb\n\tCAN NOT CONVERT\n", __FILE__,
                __LINE__);
        return;
    }

    max = w * h;

    sC = 1.0F / (float)((1 << image->comps[0].prec) - 1);
    sM = 1.0F / (float)((1 << image->comps[1].prec) - 1);
    sY = 1.0F / (float)((1 << image->comps[2].prec) - 1);
    sK = 1.0F / (float)((1 << image->comps[3].prec) - 1);

    for (i = 0; i < max; ++i) {
        /* CMYK values from 0 to 1 */
        C = (float)(image->comps[0].data[i]) * sC;
        M = (float)(image->comps[1].data[i]) * sM;
        Y = (float)(image->comps[2].data[i]) * sY;
        K = (float)(image->comps[3].data[i]) * sK;

        /* Invert all CMYK values */
        C = 1.0F - C;
        M = 1.0F - M;
        Y = 1.0F - Y;
        K = 1.0F - K;

        /* CMYK -> RGB : RGB results from 0 to 255 */
        image->comps[0].data[i] = (int)(255.0F * C * K); /* R */
        image->comps[1].data[i] = (int)(255.0F * M * K); /* G */
        image->comps[2].data[i] = (int)(255.0F * Y * K); /* B */
    }

    opj_image_data_free(image->comps[3].data);
    image->comps[3].data = NULL;
    image->comps[0].prec = 8;
    image->comps[1].prec = 8;
    image->comps[2].prec = 8;
    image->numcomps -= 1;
    image->color_space = OPJ_CLRSPC_SRGB;

    for (i = 3; i < image->numcomps; ++i) {
        memcpy(&(image->comps[i]), &(image->comps[i + 1]), sizeof(image->comps[i]));
    }

}/* color_cmyk_to_rgb() */

/*
 * This code has been adopted from sjpx_openjpeg.c of ghostscript
 */
void color_esycc_to_rgb(opj_image_t *image)
{
    int y, cb, cr, sign1, sign2, val;
    unsigned int w, h, max, i;
    int flip_value = (1 << (image->comps[0].prec - 1));
    int max_value = (1 << image->comps[0].prec) - 1;

    if (
            (image->numcomps < 3)
            || (image->comps[0].dx != image->comps[1].dx) ||
            (image->comps[0].dx != image->comps[2].dx)
            || (image->comps[0].dy != image->comps[1].dy) ||
            (image->comps[0].dy != image->comps[2].dy)
            ) {
        fprintf(stderr, "%s:%d:color_esycc_to_rgb\n\tCAN NOT CONVERT\n", __FILE__,
                __LINE__);
        return;
    }

    w = image->comps[0].w;
    h = image->comps[0].h;

    sign1 = (int)image->comps[1].sgnd;
    sign2 = (int)image->comps[2].sgnd;

    max = w * h;

    for (i = 0; i < max; ++i) {

        y = image->comps[0].data[i];
        cb = image->comps[1].data[i];
        cr = image->comps[2].data[i];

        if (!sign1) {
            cb -= flip_value;
        }
        if (!sign2) {
            cr -= flip_value;
        }

        val = (int)
                ((float)y - (float)0.0000368 * (float)cb
                 + (float)1.40199 * (float)cr + (float)0.5);

        if (val > max_value) {
            val = max_value;
        } else if (val < 0) {
            val = 0;
        }
        image->comps[0].data[i] = val;

        val = (int)
                ((float)1.0003 * (float)y - (float)0.344125 * (float)cb
                 - (float)0.7141128 * (float)cr + (float)0.5);

        if (val > max_value) {
            val = max_value;
        } else if (val < 0) {
            val = 0;
        }
        image->comps[1].data[i] = val;

        val = (int)
                ((float)0.999823 * (float)y + (float)1.77204 * (float)cb
                 - (float)0.000008 * (float)cr + (float)0.5);

        if (val > max_value) {
            val = max_value;
        } else if (val < 0) {
            val = 0;
        }
        image->comps[2].data[i] = val;
    }
    image->color_space = OPJ_CLRSPC_SRGB;

}/* color_esycc_to_rgb() */
