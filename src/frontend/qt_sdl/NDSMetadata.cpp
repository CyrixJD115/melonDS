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

#include "NDSMetadata.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>

#include <cstring>

using namespace NDSMetadataExtractor;

static QString extractTitleFromBanner(const char16_t* raw, int charCount)
{
    QString title = QString::fromUtf16(raw, charCount);
    title = title.replace(QChar('\0'), QChar(' ')).trimmed();

    QStringList lines = title.split('\n');
    QStringList displayLines;
    for (int i = 0; i < qMin(lines.size(), 2); i++)
    {
        QString line = lines[i].trimmed();
        if (!line.isEmpty())
            displayLines.append(line);
    }

    return displayLines.join(" - ");
}

static bool isTitleEmpty(const char16_t* raw, int charCount)
{
    for (int i = 0; i < charCount; i++)
    {
        if (raw[i] != 0 && raw[i] != ' ')
            return false;
    }
    return true;
}

bool NDSMetadataExtractor::extractFromROM(const QString& romPath, NDSMetadata& metadata, QImage* outIcon)
{
    QFile file(romPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    char headerTitle[13] = {};
    char gameCode[5] = {};
    if (file.read(headerTitle, 12) < 12) return false;
    if (!file.seek(0x0C) || file.read(gameCode, 4) < 4) return false;

    headerTitle[12] = '\0';
    gameCode[4] = '\0';

    metadata.romPath = romPath;
    metadata.gameCode = QString::fromLatin1(gameCode).trimmed();

    if (!file.seek(0x68))
        return false;

    quint32 bannerOffset = 0;
    stream >> bannerOffset;

    if (bannerOffset == 0)
    {
        metadata.title = QString::fromLatin1(headerTitle).trimmed();
        if (metadata.title.isEmpty())
            metadata.title = QFileInfo(romPath).completeBaseName();

        metadata.iconFile = "";
        return true;
    }

    if (!file.seek(bannerOffset))
        return false;

    quint16 bannerVersion = 0;
    stream >> bannerVersion;

    if (!file.seek(bannerOffset + 0x20))
        return false;

    unsigned char iconBitmap[512];
    if (file.read(reinterpret_cast<char*>(iconBitmap), 512) < 512)
        return false;

    unsigned short paletteData[16];
    if (file.read(reinterpret_cast<char*>(paletteData), 32) < 32)
        return false;

    if (!file.seek(bannerOffset + 0x240))
        return false;

    char16_t japaneseTitle[128] = {};
    char16_t englishTitle[128] = {};
    if (file.read(reinterpret_cast<char*>(japaneseTitle), 256) < 256)
        return false;
    if (file.read(reinterpret_cast<char*>(englishTitle), 256) < 256)
        return false;

    if (!isTitleEmpty(japaneseTitle, 128))
        metadata.title = extractTitleFromBanner(japaneseTitle, 128);
    else if (!isTitleEmpty(englishTitle, 128))
        metadata.title = extractTitleFromBanner(englishTitle, 128);
    else
        metadata.title = QString::fromLatin1(headerTitle).trimmed();

    if (metadata.title.isEmpty())
        metadata.title = QFileInfo(romPath).completeBaseName();

    QImage icon = decodeIcon(iconBitmap, paletteData);
    if (!icon.isNull())
    {
        QString iconFileName = QString("%1.png").arg(metadata.gameCode.isEmpty()
            ? QFileInfo(romPath).completeBaseName()
            : metadata.gameCode);
        metadata.iconFile = iconFileName;
        if (outIcon)
            *outIcon = icon;
    }
    else
    {
        metadata.iconFile = "";
    }

    return true;
}

QImage NDSMetadataExtractor::decodeIcon(const unsigned char* iconBitmap, const unsigned short* paletteData)
{
    unsigned int paletteRGBA[16];
    for (int i = 0; i < 16; i++)
    {
        unsigned char r = ((paletteData[i] >> 0)  & 0x1F) * 255 / 31;
        unsigned char g = ((paletteData[i] >> 5)  & 0x1F) * 255 / 31;
        unsigned char b = ((paletteData[i] >> 10) & 0x1F) * 255 / 31;
        unsigned char a = (i == 0) ? 0 : 255;
        paletteRGBA[i] = r | (g << 8) | (b << 16) | (static_cast<unsigned int>(a) << 24);
    }

    unsigned int pixels[32 * 32];
    int count = 0;
    for (int ytile = 0; ytile < 4; ytile++)
    {
        for (int xtile = 0; xtile < 4; xtile++)
        {
            for (int ypixel = 0; ypixel < 8; ypixel++)
            {
                for (int xpixel = 0; xpixel < 8; xpixel++)
                {
                    unsigned char palIndex = (count % 2)
                        ? (iconBitmap[count / 2] >> 4) & 0x0F
                        : iconBitmap[count / 2] & 0x0F;
                    pixels[ytile * 256 + ypixel * 32 + xtile * 8 + xpixel] = paletteRGBA[palIndex];
                    count++;
                }
            }
        }
    }

    QImage img(reinterpret_cast<unsigned char*>(pixels), 32, 32, QImage::Format_RGBA8888);
    return img.copy();
}

QString NDSMetadataExtractor::buildDisplayName(const NDSMetadata& metadata)
{
    return metadata.title;
}

bool NDSMetadataExtractor::saveIconPNG(const QImage& icon, const QString& path)
{
    if (icon.isNull())
        return false;

    return icon.save(path, "PNG");
}
