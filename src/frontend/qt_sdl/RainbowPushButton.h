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

#ifndef RAINBOWPUSHBUTTON_H
#define RAINBOWPUSHBUTTON_H

#include <QPushButton>
#include <QColor>

class RainbowPushButton : public QPushButton
{
    Q_OBJECT

public:
    explicit RainbowPushButton(QWidget* parent = nullptr);

    static QColor rainbowColor(int inc);
    static QString rainbowHtml(const QString& text, int offset = 0);

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // RAINBOWPUSHBUTTON_H
