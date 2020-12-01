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

#ifndef EXPORTMODES_H
#define EXPORTMODES_H

#include <QFlags>

namespace exporting {

enum ExportMode {
    None = 0,
    Foreground = 1,
    Background = 2,
    Mask = 4,
    Zones = 8,
    WholeImage = 16,
    AutoMask = 32,
    ImageWithoutOutputStage = 64
};

Q_DECLARE_FLAGS(ExportModes, ExportMode)

}

#endif // EXPORTMODES_H
