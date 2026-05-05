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

#include "RainbowPushButton.h"

#include <QPainter>
#include <QStyleOptionButton>
#include <QStyle>

QColor RainbowPushButton::rainbowColor(int inc)
{
    if      (inc < 100) return QColor(0xFF, 0x9B + inc, 0x9B);
    else if (inc < 200) return QColor(0xFF - (inc - 100), 0xFF, 0x9B);
    else if (inc < 300) return QColor(0x9B, 0xFF, 0x9B + (inc - 200));
    else if (inc < 400) return QColor(0x9B, 0xFF - (inc - 300), 0xFF);
    else if (inc < 500) return QColor(0x9B + (inc - 400), 0x9B, 0xFF);
    else                return QColor(0xFF, 0x9B + (inc - 500), 0xFF);
}

QString RainbowPushButton::rainbowHtml(const QString& text, int offset)
{
    QString html;
    for (int i = 0; i < text.length(); i++)
    {
        int inc = (offset + i * 30) % 600;
        QColor c = rainbowColor(inc);
        html += QString("<span style=\"color:%1;\">%2</span>").arg(c.name(), text[i]);
    }
    return html;
}

RainbowPushButton::RainbowPushButton(QWidget* parent) : QPushButton(parent)
{
}

void RainbowPushButton::paintEvent(QPaintEvent* event)
{
    QStyleOptionButton opt;
    opt.initFrom(this);
    opt.text = QString();
    opt.icon = icon();
    opt.iconSize = iconSize();
    opt.features = QStyleOptionButton::None;
    if (isFlat())
        opt.features |= QStyleOptionButton::Flat;
    if (isDefault())
        opt.features |= QStyleOptionButton::DefaultButton;
    if (isDown())
        opt.state |= QStyle::State_Sunken;
    if (isChecked())
        opt.state |= QStyle::State_On;

    QPainter p(this);
    style()->drawControl(QStyle::CE_PushButton, &opt, &p, this);

    QString text = this->text();
    if (text.isEmpty())
        return;

    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(text);
    int x = (width() - textWidth) / 2;
    int y = (height() + fm.ascent() - fm.descent()) / 2;

    for (int i = 0; i < text.length(); i++)
    {
        int inc = (i * 30) % 600;
        QColor c = rainbowColor(inc);
        p.setPen(c);
        QString ch = text.mid(i, 1);
        p.drawText(x, y, ch);
        x += fm.horizontalAdvance(ch);
    }
}
