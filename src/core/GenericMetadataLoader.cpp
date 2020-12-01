/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "GenericMetadataLoader.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "Dpi.h"
#include "Dpm.h"
#include <QIODevice>
#include <QSize>
#include <QImageReader>
#include <QImage>

void
GenericMetadataLoader::registerMyself()
{
    static bool registered = false;
    if (!registered) {
        ImageMetadataLoader::registerLoader(
            IntrusivePtr<ImageMetadataLoader>(new GenericMetadataLoader)
        );
        registered = true;
    }
}

ImageMetadataLoader::Status
GenericMetadataLoader::loadMetadata(
    QIODevice& io_device,
    VirtualFunction1<void, ImageMetadata const&>& out)
{
    if (!io_device.isReadable()) {
        return GENERIC_ERROR;
    }

    QImageReader reader(&io_device);

    if (!reader.canRead()) {
        return FORMAT_NOT_RECOGNIZED;
    }

    if (!reader.supportsOption(QImageIOHandler::Size)) {
        return FORMAT_NOT_RECOGNIZED;
    }

    QSize const size = reader.size();

    QImage image;
    reader.read(&image);

    Dpi dpi(Dpm(image.dotsPerMeterX(), image.dotsPerMeterY()));

    out(ImageMetadata(size, dpi));
    return LOADED;
}
