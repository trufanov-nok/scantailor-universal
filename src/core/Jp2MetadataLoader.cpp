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

#include "Jp2MetadataLoader.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "Dpi.h"
#include "Dpm.h"
#include <QIODevice>
#include <QSize>
#include <QDebug>
#include <string.h>
#include <assert.h>
#include <cmath>

#include "Jp2Reader.h"

/*============================= Jp2MetadataLoader ==========================*/

void
Jp2MetadataLoader::registerMyself()
{
    static bool registered = false;
    if (!registered) {
        ImageMetadataLoader::registerLoader(
                    IntrusivePtr<ImageMetadataLoader>(new Jp2MetadataLoader)
                    );
        registered = true;
    }
}


ImageMetadataLoader::Status
Jp2MetadataLoader::loadMetadata(
        QIODevice& io_device,
        VirtualFunction1<void, ImageMetadata const&>& out)
{
  return Jp2Reader::readMetadata(io_device, out);
}
