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
#include "fullbackgroundwidget.h"

#include <QGuiApplication>
#include <QScreen>
#include <QDBusInterface>
#include <QDebug>
#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <QDesktopWidget>
#include <QCloseEvent>

#include <QX11Info>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <xcb/xcb.h>
#include "lockwidget.h"
#include "xeventmonitor.h"
#include "monitorwatcher.h"
#include "configuration.h"
#include "screensaver.h"
#include "screensaverwidget.h"
#include "grab-x11.h"

enum {
    SWITCH_TO_LINUX = 0,
    SWITCH_TO_ANDROID = 1,
    BACK_TO_DESKTOP = 2,
    TEST_CONNECTION = 3,
};

QT_BEGIN_NAMESPACE
extern void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

#define BLUR_RADIUS 300

int connect_to_switch(const char* path)
{
    int ret;
    int connect_fd;
    struct sockaddr_un srv_addr;

    connect_fd = socket(PF_UNIX,SOCK_STREAM,0);
    if(connect_fd < 0) {

        return -1;
    }

    srv_addr.sun_family=AF_UNIX;
    strcpy(srv_addr.sun_path, path);

    ret = connect(connect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if(ret < 0) {
        perror("cannot connect to the server");
        close(connect_fd);
        return -1;
    }

    return connect_fd;
}

int switch_to_linux(const char* container)
{
    int connect_fd;
    int32_t switch_to = BACK_TO_DESKTOP;

    char path[1024] = {0};
    sprintf(path, "/var/lib/kydroid/%s/sockets/%s", container, "desktop-switch");
    printf("path = %s\n",path);
    connect_fd = connect_to_switch(path);

    if(connect < 0)
        return -1;

    write(connect_fd, &switch_to, sizeof(switch_to));
    close(connect_fd);
    return 0;
}

void x11_get_screen_size(int *width,int *height)
{
    Display* display;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot connect to X server %s/n", "simey:0");
        exit (-1);
    }
    int screen_num;

    screen_num = DefaultScreen(display);

    *width = DisplayWidth(display, screen_num);
    *height = DisplayHeight(display, screen_num);
    XCloseDisplay(display);

}

QPixmap blurPixmap(QPixmap pixmap)
{
    QPainter painter(&pixmap);
    QImage srcImg = pixmap.toImage();
    qt_blurImage(&painter, srcImg, BLUR_RADIUS, false, false);
    painter.end();
    return pixmap;
}

FullBackgroundWidget::FullBackgroundWidget(QWidget *parent)
    : QWidget(parent),
      lockWidget(nullptr),
      xEventMonitor(new XEventMonitor(this)),
      monitorWatcher(new MonitorWatcher(this)),
      configuration(new Configuration(this)),
      isLocked(false),
      screenStatus(UNDEFINED)
{
    qDebug() << "init - screenStatus: " << screenStatus;
    setMouseTracking(true);
    connect(monitorWatcher, &MonitorWatcher::monitorCountChanged,
            this, &FullBackgroundWidget::onScreenCountChanged);
    QDesktopWidget *desktop = QApplication::desktop();

    connect(desktop, &QDesktopWidget::resized,
            this, &FullBackgroundWidget::onDesktopResized);
    connect(desktop, &QDesktopWidget::workAreaResized,
            this, &FullBackgroundWidget::onDesktopResized);


    QDBusInterface *iface = new QDBusInterface("org.freedesktop.login1",
                                               "/org/freedesktop/login1",
                                               "org.freedesktop.login1.Manager",
                                               QDBusConnection::systemBus(),
                                               this);
    connect(iface, SIGNAL(PrepareForSleep(bool)), this, SLOT(onPrepareForSleep(bool)));

    init();
     qApp->installNativeEventFilter(this);

    QTimer::singleShot(500,this,SLOT(switchToLinux()));
}

void FullBackgroundWidget::switchToLinux()
{
    struct passwd *pwd;
    pwd = getpwuid(getuid());
    char *username = pwd->pw_name;
    int uid = pwd->pw_uid;
    char container[100]= {0};

    sprintf(container,"kydroid2-%d-%s",uid,username);

    switch_to_linux(container);

}

void FullBackgroundWidget::paintEvent(QPaintEvent *event)
{
    QDesktopWidget *desktop = QApplication::desktop();
    if(!desktop->isVirtualDesktop())
    {
        int width=0,height = 0;
        x11_get_screen_size(&width,&height);
        QPainter painter(this);
        QRect rec(0,0,width,height);
        painter.drawPixmap(rec, background);
    }
    else
    {
        for(auto screen : QGuiApplication::screens())
        {
            QPainter painter(this);
            painter.drawPixmap(screen->geometry(), background);
        }
    }
    return QWidget::paintEvent(event);
}

void FullBackgroundWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "FullBackgroundWidget::closeEvent";
    for(auto obj: children())
    {
        QWidget *widget = dynamic_cast<QWidget*>(obj);
        if(widget)
            widget->close();
    }
    closeGrab();
    return QWidget::closeEvent(event);
}

void FullBackgroundWidget::showEvent(QShowEvent *event)
{
   // XSetWindowAttributes top_attrs;
   // top_attrs.override_redirect = False;
   // XChangeWindowAttributes(QX11Info::display(), this->winId(), CWOverrideRedirect, &top_attrs);
   // XRaiseWindow(QX11Info::display(), this->winId());
  //  raise();

    return QWidget::showEvent(event);
}


bool FullBackgroundWidget::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
        if (qstrcmp(eventType, "xcb_generic_event_t") != 0) {
            return false;
        }
        xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t*>(message);
        const uint8_t responseType = event->response_type & ~0x80;
        if (responseType == XCB_CONFIGURE_NOTIFY) {
                xcb_configure_notify_event_t *xc = reinterpret_cast<xcb_configure_notify_event_t*>(event);
                if (xc->event == QX11Info::appRootWindow())
                {
                    this->onDesktopResized();
                    raise();
                }
                return false;
        }
        else if(responseType == XCB_PROPERTY_NOTIFY)
        {
            raise();
        }
        return false;
}

void FullBackgroundWidget::mouseMoveEvent(QMouseEvent *e)
{
	onCursorMoved(cursor().pos()); 
	return QWidget::mouseMoveEvent(e);
}

void FullBackgroundWidget::init()
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint
                   | Qt::X11BypassWindowManagerHint);
//    setAttribute(Qt::WA_DeleteOnClose);
/*捕获键盘，如果捕获失败，则可能是由于弹出菜单项已经捕获，那么模拟一次esc按键来退出菜单，如果仍捕获失败，则放弃锁屏，避免密码无法输入*/
    if(establishGrab())
        qDebug()<<"establishGrab : true";
    else {
        qDebug()<<"establishGrab : false";
        XTestFakeKeyEvent(QX11Info::display(), XKeysymToKeycode(QX11Info::display(),XK_Escape), True, 1);
        XTestFakeKeyEvent(QX11Info::display(), XKeysymToKeycode(QX11Info::display(),XK_Escape), False, 1);
        XFlush(QX11Info::display());
        sleep(1);
        if(!establishGrab())
	{
		exit(1);
	}
    }
    // 监听session信号
    smInterface = new QDBusInterface(SM_DBUS_SERVICE,
                                     SM_DBUS_PATH,
                                     SM_DBUS_INTERFACE,
                                     QDBusConnection::sessionBus());
    connect(smInterface, SIGNAL(StatusChanged(uint)),
            this, SLOT(onSessionStatusChanged(uint)));

    connect(xEventMonitor, SIGNAL(keyPress(const QString &)),
            this, SLOT(onGlobalKeyPress(const QString &)));
    connect(xEventMonitor, SIGNAL(keyRelease(const QString &)),
            this, SLOT(onGlobalKeyRelease(const QString &)));
    connect(xEventMonitor, SIGNAL(buttonDrag(int, int)),
            this, SLOT(onGlobalButtonDrag(int, int)));

    int totalWidth = 0;
    int totalHeight = 0;
    for(auto screen : QGuiApplication::screens())
    {
        totalWidth += screen->geometry().width();
        totalHeight += screen->geometry().height();
    }
    setGeometry(0, 0, totalWidth, totalHeight);

    background.load(configuration->getBackground());
    background = blurPixmap(background);

    xEventMonitor->start();
}

void FullBackgroundWidget::onCursorMoved(const QPoint &pos)
{
    if(!lockWidget)
    {
        return;
    }
    for(auto screen : QGuiApplication::screens())
    {
       	if(screen->geometry().contains(pos))
       	{
          //  lockWidget->hide(); //避免闪屏，所以先隐藏，设置大小后再显示
    		lockWidget->setGeometry(screen->geometry());
           // lockWidget->show();
    		break;
       	}
    }
}

void FullBackgroundWidget::lock()
{
    showLockWidget();

    lockWidget->startAuth();;
}


void FullBackgroundWidget::showLockWidget()
{
    screenStatus = (ScreenStatus)(screenStatus | SCREEN_LOCK);
    qDebug() << "showLockWidget - screenStatus: " << screenStatus;

    if(!lockWidget)
    {

        lockWidget = new LockWidget(this);
        connect(lockWidget, &LockWidget::closed,
                this, &FullBackgroundWidget::close);
        onCursorMoved(cursor().pos());
    }
    lockWidget->setFocus();
    XSetInputFocus(QX11Info::display(),this->winId(),RevertToParent,CurrentTime);
}

void FullBackgroundWidget::showScreensaver()
{
    screenStatus = (ScreenStatus)(screenStatus | SCREEN_SAVER);
    qDebug() << "showScreensaver - screenStatus: " << screenStatus;

    for(auto screen : QGuiApplication::screens())
    {
        ScreenSaver *saver = configuration->getScreensaver();
        ScreenSaverWidget *saverWidget = new ScreenSaverWidget(saver, this);
        widgetXScreensaverList.push_back(saverWidget);
        saverWidget->setGeometry(screen->geometry());
    }
    setCursor(Qt::BlankCursor);

    //显示屏保时，停止认证（主要针对生物识别）
    if(lockWidget)
    {
        lockWidget->stopAuth();
    }
  //  XSetInputFocus(QX11Info::display(),this->winId(),RevertToNone,CurrentTime);

}

void FullBackgroundWidget::clearScreensavers()
{
    screenStatus = (ScreenStatus)(screenStatus & ~SCREEN_SAVER);

    for(auto widget : widgetXScreensaverList)
    {
        widget->close();
    }
    widgetXScreensaverList.clear();

    qDebug() << "clearScreensavers - screenStatus: " << screenStatus;


    unsetCursor();
    if(screenStatus == UNDEFINED)
    {
        close();
    }
    else
    {
        lock();
    }

}

int FullBackgroundWidget::onSessionStatusChanged(uint status)
{
    qDebug() << "session status changed: " << status;
    if(status != SESSION_IDLE)
    {
        //当前session没有处于空闲状态
        return -1;
    }
    qDebug() << "onSessionStatusChanged - screenStatus: " << screenStatus;

    if(!configuration->xscreensaverActivatedWhenIdle())
    {
	return -1;
    }

    if(screenStatus & SCREEN_SAVER)
    {
        return -1;
    }
    else if(screenStatus & SCREEN_LOCK)
    {
        showScreensaver();
    }
    else if(screenStatus == UNDEFINED)
    {
        if(configuration->xscreensaverActivatedWhenIdle() &&
                configuration->lockWhenXScreensaverActivated())
        {
            //显示锁屏和屏保
            showLockWidget();
            showScreensaver();
        }
        else if(configuration->xscreensaverActivatedWhenIdle())
        {
            //只显示屏保
            showScreensaver();
        }
    }
    return 0;
}

void FullBackgroundWidget::onGlobalKeyPress(const QString &key)
{
}

void FullBackgroundWidget::onGlobalKeyRelease(const QString &key)
{
    if(key == "Caps_Lock")
    {
        lockWidget->capsLockChanged();
    }
    else if(screenStatus & SCREEN_SAVER)
    {
        clearScreensavers();
    }

}

void FullBackgroundWidget::onGlobalButtonDrag(int xPos, int yPos)
{
   // if(screenStatus & SCREEN_SAVER)
   // {
   //     clearScreensavers();
   // }
}


void FullBackgroundWidget::onScreenCountChanged(int)
{
    QSize newSize = monitorWatcher->getVirtualSize();
    setGeometry(0, 0, newSize.width(), newSize.height());
    //repaint();
    update();
    clearScreensavers();
}

void FullBackgroundWidget::onDesktopResized()
{
    QDesktopWidget *desktop = QApplication::desktop();
    if(!desktop->isVirtualDesktop())
    {
        int width=0,height = 0;
        x11_get_screen_size(&width,&height);
        if(width==0||height==0)
        {
            setGeometry(desktop->geometry());
            if(lockWidget)
	    	lockWidget->setGeometry(QApplication::primaryScreen()->geometry());
        }
        else
        {
            qDebug()<<"width = "<<width<<"height  = "<<height;
            setGeometry(0,0,width,height);
            if(lockWidget)
	    	lockWidget->setGeometry(0,0,width,height);
        }
    }
    else{
        qDebug()<<"desktop"<<desktop->geometry();
        setGeometry(desktop->geometry());
        if(lockWidget)
    		onCursorMoved(cursor().pos());
    }

//    clearScreensavers();
   //repaint();
	update();

}

void FullBackgroundWidget::onPrepareForSleep(bool sleep)
{
    ///系统休眠时，会关闭总线，导致设备不可用，发生错误
    ///在系统休眠之前停止认证，在系统唤醒后重新开始认证
    if(sleep)
    {
        lockWidget->stopAuth();
    }
    else
    {
        if(screenStatus & SCREEN_SAVER)
        {
            lockWidget->startAuth();
        }
    }
}
