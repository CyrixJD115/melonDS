/*
    Copyright 2016-2026 melonDS team

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef NDSMETADATA_H
#define NDSMETADATA_H

#include <QString>
#include <QImage>

struct NDSMetadata
{
    QString romPath;
    QString title;
    QString gameCode;
    QString iconFile;
};

namespace NDSMetadataExtractor
{
    bool extractFromROM(const QString& romPath, NDSMetadata& metadata, QImage* outIcon = nullptr);
    QImage decodeIcon(const unsigned char* iconBitmap, const unsigned short* paletteData);
    QString buildDisplayName(const NDSMetadata& metadata);
    bool saveIconPNG(const QImage& icon, const QString& path);
}

#endif // NDSMETADATA_H
