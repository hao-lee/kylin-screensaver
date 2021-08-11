 /*
  * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 3, or (at your option)
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, see <http://www.gnu.org/licenses/>.
  *
 **/

#ifndef POWERMANAGER_H
#define POWERMANAGER_H
#include <QWidget>
#include <QListWidget>
#include <QTime>
#include "config.h"

#ifdef USE_INTEL
#define ITEM_WIDTH 128
#define ITEM_HEIGHT (ITEM_WIDTH + 40)
#define ITEM_SPACING 72

class QWidget;
class PowerManager:public QWidget
#else
#define ITEM_WIDTH 168
#define ITEM_HEIGHT ITEM_WIDTH
#define ITEM_SPACING (ITEM_WIDTH/8)
class QListWidget;
class QListWidgetItem;
class QDBusInterface;
class PowerManager:public QListWidget
#endif
{
    Q_OBJECT

public:
    PowerManager(QWidget *parent = 0);
#ifdef USE_INTEL
    bool hibernate();
#else
    void showNormalSize();
    void showSmallSize();
#endif
private:
    void initUI();

#ifdef USE_INTEL
    void setQSS();
    QWidget *list;
#else
    QListWidget *list;
    QWidget *suspendWidget;
    QWidget *hibernateWidget;
    QDBusInterface *sessionInterface;
    QDBusInterface *loginInterface;
    bool    canSuspend;
    bool    canHibernate;
    int     m_count;
#endif
    QWidget *lockWidget;
    QWidget *switchWidget;
    QWidget *logoutWidget;
    QWidget *rebootWidget;
    QWidget *shutdownWidget;
    QTime lasttime;
private:
    void lockWidgetClicked();
    void switchWidgetClicked();
    void logoutWidgetCliced();
    void rebootWidgetClicked();
    void shutdownWidgetClicked();
#ifdef USE_INTEL
    bool reboot();
    bool powerOff();
#else
    void suspendWidgetClicked();
    void hibernateWidgetClicked();
#endif
private Q_SLOTS:
#ifndef USE_INTEL
    void powerClicked(QListWidgetItem *item);
#endif
Q_SIGNALS:
    void switchToUser();
    void lock();
};

#endif // POWERMANAGER_H
