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
#include "lockwidget.h"
#include "ui_lockwidget.h"

#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <QMenu>
#include <QtX11Extras/QX11Info>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include "authdialog.h"
#include "virtualkeyboard.h"
#include "users.h"
#include "displaymanager.h"

LockWidget::LockWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::LockWidget),
      usersMenu(nullptr),
      users(new Users(this)),
      displayManager(new DisplayManager(this))
{
    ui->setupUi(this);

    UserItem user = users->getUserByName(getenv("USER"));
    authDialog = new AuthDialog(user, this);
    connect(authDialog, &AuthDialog::authenticateCompete,
            this, &LockWidget::closed);
    connect(this, &LockWidget::capsLockChanged,
            authDialog, &AuthDialog::onCapsLockChanged);
    this->installEventFilter(this);
    initUI();
}

LockWidget::~LockWidget()
{
    delete ui;
}

void LockWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "LockWidget::closeEvent";
    authDialog->close();
    return QWidget::closeEvent(event);
}

bool LockWidget::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == this){
        if(event->type() == 2){
                if(usersMenu->isVisible())
                    usersMenu->hide();
                return false;
         }
    }
    return false;
}

void LockWidget::startAuth()
{
    if(authDialog)
    {
        authDialog->startAuth();
    }
}

void LockWidget::stopAuth()
{
    if(authDialog)
    {
        authDialog->stopAuth();
    }
}

void LockWidget::initUI()
{
    setFocusProxy(authDialog);

    //显示系统时间
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [&]{
        QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
        ui->lblTime->setText(time);
	QString date = QDate::currentDate().toString("yyyy/MM/dd dddd");
	ui->lblDate->setText(date);
    });

    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->lblTime->setText(time);
    ui->lblTime->setStyleSheet("QLabel{color:white; font-size: 55px;}");
    ui->lblTime->adjustSize();
    timer->start(1000);

    QString date = QDate::currentDate().toString("yyyy/MM/dd dddd");
    qDebug() << "current date: " << date;
    ui->lblDate->setText(date);
    ui->lblDate->setStyleSheet("QLabel{color:white; font-size: 20px;}");
    ui->lblDate->adjustSize();

    //虚拟键盘
    vKeyboard = new VirtualKeyboard(this);
    vKeyboard->hide();
    connect(vKeyboard, &VirtualKeyboard::aboutToClose,
            vKeyboard, &VirtualKeyboard::hide);

    ui->btnKeyboard->setIcon(QIcon(":/image/assets/keyboard.png"));
    ui->btnKeyboard->setFixedSize(39, 39);
    ui->btnKeyboard->setIconSize(QSize(39, 39));
/*    connect(ui->btnKeyboard, &QPushButton::clicked,
            this, [&]{
        qDebug() << vKeyboard->isHidden();
        vKeyboard->setVisible(vKeyboard->isHidden());
    });
*/
    connect(ui->btnKeyboard, &QPushButton::clicked,
            this, &LockWidget::showVirtualKeyboard);

    //用户切换
    if(displayManager->canSwitch())
    {
        initUserMenu();
    }
}

void LockWidget::showVirtualKeyboard()
{
    vKeyboard->setVisible(vKeyboard->isHidden());
    setVirkeyboardPos();
}

void LockWidget::setVirkeyboardPos()
{
    if(vKeyboard)
    {
        vKeyboard->setGeometry(0,
                               height() - height()/3,
                               width(), height()/3);

    }
}


void LockWidget::initUserMenu()
{
    ui->btnSwitchUser->setIcon(QIcon(":/image/assets/avatar.png"));
    ui->btnSwitchUser->setIconSize(QSize(39, 39));
    ui->btnSwitchUser->setFixedSize(39, 39);
    if(!usersMenu)
    {
        usersMenu = new QMenu(this);

        //如果没有设置x11属性，则由于弹出菜单受窗口管理器管理，而主窗口不受，在点击菜单又点回主窗口会闪屏。
        usersMenu->setWindowFlags(Qt::X11BypassWindowManagerHint);
        usersMenu->hide();
        usersMenu->move(width() - 150, 60);
      //  ui->btnSwitchUser->setMenu(usersMenu);
        connect(usersMenu, &QMenu::triggered,
                this, &LockWidget::onUserMenuTrigged);
        connect(ui->btnSwitchUser, &QPushButton::clicked,
                this, [&]{
                if(usersMenu->isVisible())
                    usersMenu->hide();
                else
                    usersMenu->show();
        });
    }

    connect(users, &Users::userAdded, this, &LockWidget::onUserAdded);
    connect(users, &Users::userDeleted, this, &LockWidget::onUserDeleted);

    for(auto user : users->getUsers())
    {
        onUserAdded(user);
    }

    if(displayManager->hasGuestAccount())
    {
        QAction *action = new QAction(QIcon(users->getDefaultIcon()),
                                      tr("Guest"), this);
        action->setData("Guest");
        usersMenu->addAction(action);
    }

    {
        QAction *action = new QAction(QIcon(users->getDefaultIcon()),
                                      tr("SwitchUser"), this);
        action->setData("SwitchUser");
        usersMenu->addAction(action);
    }
	
}

/* lockscreen follows cursor */
void LockWidget::resizeEvent(QResizeEvent */*event*/)
{
    //认证窗口
    authDialog->setGeometry((width()-authDialog->geometry().width())/2, 0,
                            authDialog->width(), height());

    //系统时间
    ui->widgetTime->move(0, height() - 150);

    //虚拟键盘按钮
    ui->btnKeyboard->move(width() - 60, 20);

    ui->btnSwitchUser->move(width() - 120, 20);
    
    setVirkeyboardPos();
    usersMenu->move(width() - 150, 60);
     XSetInputFocus(QX11Info::display(),this->winId(),RevertToParent,CurrentTime);


}


void LockWidget::onUserAdded(const UserItem &user)
{
    QAction *action = new QAction(QIcon(user.icon), user.realName, this);
    action->setData(user.name);
    usersMenu->addAction(action);
}

void LockWidget::onUserDeleted(const UserItem &user)
{
    for(auto action : usersMenu->actions())
    {
        if(action->data() == user.name)
            usersMenu->removeAction(action);
    }
}

void LockWidget::onUserMenuTrigged(QAction *action)
{
    qDebug() << action->data().toString() << "selected";

    QString userName = action->data().toString();
    if(userName == "Guest")
    {
        displayManager->switchToGuest();
    }
    else if(userName == "SwitchUser")
    {
        displayManager->switchToGreeter();
    }
    else
    {
        displayManager->switchToUser(userName);
    }
    if(authDialog)
    {
        authDialog->stopAuth();
    }
}
