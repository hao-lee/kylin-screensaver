/**
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
**/

#ifndef SWITCHBUTTON_H
#define SWITCHBUTTON_H

#include <QObject>
#include <QTimer>
#include <QWidget>
#include <QPainter>

class SwitchButton : public QWidget
{
    Q_OBJECT
public:
    explicit SwitchButton(QWidget *parent = nullptr);
    void     setSwitchStatus(bool check);

private:
    int             m_bIsOn = 1;
    QTimer          *m_cTimer;
    float           m_fWidth;
    float           m_fHeight;
    float           m_fCurrentValue;
    void            paintEvent(QPaintEvent *event);
    void            mousePressEvent(QMouseEvent *event);

Q_SIGNALS:
    void clicked(int check);
private Q_SLOTS:
    void startAnimation();

};

#endif // SWITCHBUTTON_H
