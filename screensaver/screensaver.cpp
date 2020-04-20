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

#include <QWidget>
#include <QPalette>
#include <QPixmap>
#include <QPainter>
#include <QDebug>
#include <QDate>
#include <QDate>
#include <QApplication>
#include <QTextCodec>
#include <QKeyEvent>
#include <QSplitterHandle>
#include <QDateTime>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QX11Info>
#include <QDBusInterface>
#include <QDBusReply>
#include <QApplication>
#include "screensaver.h"
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

Screensaver::Screensaver(QWidget *parent):
  QWidget(parent),
  date(new ChineseDate()),
  centerWidget(nullptr),
  dateOfLunar(nullptr),
  flag(0),
  background(""),
  autoSwitch(nullptr),
  vboxFrame(nullptr),
  m_timer(nullptr)
{
    installEventFilter(this);
     setUpdateCenterWidget();
    initUI();
    m_background = new MBackground();

    settings = new QGSettings("org.mate.background","",this);
    defaultBackground = settings->get("picture-filename").toString();

    QString backgroundFile = defaultBackground;
    background = QPixmap(backgroundFile);

    QList<QLabel*> labelList = this->findChildren<QLabel *>();
    for(int i = 0;i<labelList.count();i++)
    {
        labelList.at(i)->setAlignment(Qt::AlignCenter);
    }

    setUpdateBackground();

}

Screensaver::~Screensaver()
{

}

bool Screensaver::eventFilter(QObject *obj, QEvent *event)
{

    if(event->type() == 6){
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() ==Qt::Key_Q || keyEvent->key() == Qt::Key_Escape){
            qApp->quit(); //需要 #include <QApplication> 头文件
        }
    }
    return false;
}

void Screensaver::mousePressEvent(QMouseEvent *event)
{
    if(vboxFrame && vboxFrame->isVisible()){
        vboxFrame->hide();
    }
}

void Screensaver::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0,0,this->width(),this->height(),background);
    painter.setBrush(QColor(0,0,0,178));
    painter.drawRect(0,0,this->width(),this->height());

}

void Screensaver::resizeEvent(QResizeEvent */*event*/)
{
    float scale = 1.0;
    scale = (float)width()/1920;
    if(width() < 600 || height()<400){
        if(flag == 0)
        {
            QList<QLabel*> labelList = this->findChildren<QLabel *>();
            for(int i = 0;i<labelList.count();i++)
            {
                int fontsize = labelList.at(i)->font().pixelSize();
                const QString SheetStyle = QString("font-size:%1px;").arg(fontsize/3);
                labelList.at(i)->setStyleSheet(SheetStyle);
            }
            QList<QWidget*> childList = timeLayout->findChildren<QWidget *>();
            for (int i = 0; i < childList.count(); ++i) {
                childList.at(i)->adjustSize();
            }
            timeLayout->adjustSize();
            if(centerWidget)
                centerWidget->adjustSize();
        }
        flag = 1;
        if(sleepTime)
            sleepTime->hide();
        if(settingsButton)
            settingsButton->hide();
        if(escButton)
            escButton->hide();
        scale = 0.1;
    }

    int x = (this->width()-timeLayout->geometry().width())/2;
    int y = 129*scale;

    timeLayout->setGeometry(x,y,timeLayout->geometry().width(),timeLayout->geometry().height());

    if(sleepTime){
        x = this->width() - sleepTime->geometry().width() - 26*scale;
        y = this->height() - sleepTime->geometry().height() - 26*scale;
        sleepTime->setGeometry(x,y,sleepTime->geometry().width(),sleepTime->geometry().height());
    }
    if(centerWidget){
        centerWidget->adjustSize();
        centerWidget->setGeometry((width()-centerWidget->width())/2,(height()-centerWidget->height())/2,
                              centerWidget->width(),centerWidget->height());

        if((height()-centerWidget->height())/2 < timeLayout->y() + timeLayout->height())
            centerWidget->setGeometry((width()-centerWidget->width())/2,timeLayout->y() + timeLayout->height(),
                                  centerWidget->width(),centerWidget->height());
    }


    ubuntuKylinlogo->setGeometry(40*scale,40*scale,127*scale,42*scale);

    if(escButton){
        escButton->setGeometry(width() - 40*scale - escButton->width(),40*scale,escButton->width(),escButton->height());
    }
    if(settingsButton);
         settingsButton->setGeometry(escButton->geometry().left() - 16*scale - settingsButton->width(),40*scale,settingsButton->width(),settingsButton->height());

    if(vboxFrame)
        vboxFrame->setGeometry(settingsButton->geometry().left(),
                                settingsButton->geometry().bottom() + 12*scale,
                                vboxFrame->width(),vboxFrame->height());
}

void Screensaver::setUpdateCenterWidget()
{
    QString lang = qgetenv("LANG");
    if (!lang.isEmpty()){
        qDebug()<<"lang = "<<lang;
        if (lang.contains("zh_CN")){
        	qsettings = new QSettings("/usr/share/ukui-screensaver/screensaver.ini",QSettings::IniFormat);
	}
	else{
		qsettings = new QSettings("/usr/share/ukui-screensaver/screensaver-en.ini",QSettings::IniFormat);
	}
    }

    qsettings->setIniCodec(QTextCodec::codecForName("UTF-8"));
}

void Screensaver::updateCenterWidget(int index)
{
    if(!centerWidget )
        return ;

    QStringList qlist = qsettings->childGroups();
    if(qlist.count()<1)
        return;

    if(index<=1){
        qsrand((unsigned)time(0));
        index = qrand() % qlist.count() + 1;
    }
    qsettings->beginGroup(QString::number(index));
    if(qsettings->contains("OL")){
        centerlabel1->setText(qsettings->value("OL").toString());
        centerlabel2->hide();
        authorlabel->setText(qsettings->value("author").toString());
    }
    else if(qsettings->contains("FL"))
    {
        centerlabel1->setText(qsettings->value("FL").toString());
        centerlabel2->setText(qsettings->value("SL").toString());
        centerlabel2->show();
        authorlabel->setText(qsettings->value("author").toString());
    }
    
    centerWidget->adjustSize();
    centerWidget->setGeometry((width()-centerWidget->width())/2,(height()-centerWidget->height())/2,
                          centerWidget->width(),centerWidget->height());

    if((height()-centerWidget->height())/2 < timeLayout->y() + timeLayout->height())
        centerWidget->setGeometry((width()-centerWidget->width())/2,timeLayout->y() + timeLayout->height(),
                              centerWidget->width(),centerWidget->height());

    qsettings->endGroup();

}
void Screensaver::initUI()
{
    QFile qssFile(":/qss/assets/default.qss");
    if(qssFile.open(QIODevice::ReadOnly)) {
        setStyleSheet(qssFile.readAll());
    }
    qssFile.close();

    setDatelayout();
    setSleeptime();
  
    setCenterWidget();

    ubuntuKylinlogo = new QLabel(this);
    ubuntuKylinlogo->setObjectName("ubuntuKylinlogo");
    ubuntuKylinlogo->setPixmap(QPixmap(":/assets/logo.svg"));
    ubuntuKylinlogo->adjustSize();
    ubuntuKylinlogo->setScaledContents(true);

    escButton = new QPushButton(this);
    escButton->setObjectName("escButton");
    escButton->setText(tr("exit"));
    escButton->setFixedSize(152,48);
    connect(escButton,&QPushButton::clicked,this,[&]{
        XTestFakeKeyEvent(QX11Info::display(), XKeysymToKeycode(QX11Info::display(),XK_Escape), True, 1);
        XTestFakeKeyEvent(QX11Info::display(), XKeysymToKeycode(QX11Info::display(),XK_Escape), False, 1);
        XFlush(QX11Info::display());
        qApp->quit();
    });

    settingsButton = new QPushButton(this);
    settingsButton->setObjectName("settingsButton");
    settingsButton->setFixedSize(48,48);
    settingsButton->setIcon(QIcon(":/assets/settings.svg"));
    connect(settingsButton,&QPushButton::clicked,this,[&]{
        if(vboxFrame->isVisible())
            vboxFrame->hide();
        else
            vboxFrame->show();
    });

    WallpaperButton = new QPushButton(this);
    WallpaperButton->setObjectName("WallpaperButton");
    WallpaperButton->setFixedHeight(36);
    WallpaperButton->setMinimumWidth(160);
    WallpaperButton->setIcon(QIcon(":/assets/wallpaper.svg"));
    WallpaperButton->setText(tr("Set as desktop wallpaper"));
    connect(WallpaperButton,SIGNAL(clicked()),this,SLOT(setDesktopBackground()));

    QFrame *autoSwitch = new QFrame(this);
    autoSwitch->setObjectName("autoSwitch");
    autoSwitch->setFixedHeight(36);
    autoSwitch->setMinimumWidth(160);
    autoSwitchLabel = new QLabel(this);
    autoSwitchLabel->setObjectName("autoSwitchLabel");
    autoSwitchLabel->setText(tr("Automatic switching"));

    checkSwitch = new checkButton(this);

    defaultSettings = new QGSettings("org.ukui.screensaver-default","",this);
    isAutoSwitch = defaultSettings->get("automatic-switching-enabled").toBool();

    checkSwitch->setAttribute(Qt::WA_DeleteOnClose);
    checkSwitch->setChecked(isAutoSwitch);
    connect(checkSwitch, &checkButton::checkedChanged, [=](bool checked){
         defaultSettings->set("automatic-switching-enabled",QVariant(checked));
         isAutoSwitch = checked;
         setUpdateBackground();
    });


    QHBoxLayout *hlayout = new QHBoxLayout(autoSwitch);
    hlayout->addWidget(autoSwitchLabel);
    hlayout->addWidget(checkSwitch);

    vboxFrame = new QFrame(this);
    vboxFrame->setObjectName("vboxFrame");
    QVBoxLayout *vlayout = new QVBoxLayout(vboxFrame);
    vlayout->setContentsMargins(4,4,4,4);
    vlayout->setSpacing(4);
    vlayout->addWidget(WallpaperButton);
    vlayout->addWidget(autoSwitch);
    vlayout->setAlignment(autoSwitch,Qt::AlignCenter);
    vboxFrame->adjustSize();
    vboxFrame->hide();
}

void Screensaver::setDatelayout()
{
    timeLayout = new QWidget(this);
    QVBoxLayout *vtimeLayout = new QVBoxLayout(timeLayout);

    this->dateOfWeek = new QLabel(this);
    this->dateOfWeek->setText(QDate::currentDate().toString("dddd"));
    this->dateOfWeek->setObjectName("dateOfWeek");
    this->dateOfWeek->setAlignment(Qt::AlignCenter);
    vtimeLayout->addWidget(dateOfWeek);

    this->dateOfLocaltime = new QLabel(this);
    this->dateOfLocaltime->setText(QDateTime::currentDateTime().toString("hh:mm"));
    this->dateOfLocaltime->setObjectName("dateOfLocaltime");
    this->dateOfLocaltime->setAlignment(Qt::AlignCenter);
    vtimeLayout->addWidget(dateOfLocaltime);

    QWidget *dateWidget = new QWidget(this);
    this->dateOfDay = new QLabel(this);
    this->dateOfDay->setText(QDate::currentDate().toString("yy/MM/dd"));
    this->dateOfDay->setObjectName("dateOfDay");
    this->dateOfDay->setAlignment(Qt::AlignCenter);
    this->dateOfDay->adjustSize();

    QHBoxLayout *hdateLayout = new QHBoxLayout(dateWidget);
    hdateLayout->addWidget(dateOfDay);

    QString lang = qgetenv("LANG");
    if (!lang.isEmpty()){
        qDebug()<<"lang = "<<lang;
        if (lang.contains("zh_CN")){
            this->dateOfLunar = new QLabel(this);
            this->dateOfLunar->setText(date->getDateLunar());
            this->dateOfLunar->setObjectName("dateOfLunar");
            this->dateOfLunar->setAlignment(Qt::AlignCenter);
            this->dateOfLunar->adjustSize();
            hdateLayout->addWidget(dateOfLunar);
        }
    }
    dateWidget->adjustSize();

    vtimeLayout->addWidget(dateWidget);

    timeLayout->adjustSize();
}

void Screensaver::setSleeptime()
{
    sleepTime = new SleepTime(this);
    sleepTime->adjustSize();
     updateDate();
}

void Screensaver::updateDate()
{
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateTime()));
    timer->start(1000);
}

void Screensaver::updateTime()
{
    this->dateOfWeek->setText(QDate::currentDate().toString("dddd"));
    this->dateOfLocaltime->setText(QDateTime::currentDateTime().toString("hh:mm"));
    this->dateOfDay->setText(QDate::currentDate().toString("yy/MM/dd"));
    if(sleepTime){
        if(!sleepTime->setTime()){
            timer->stop();
        }
    }
}

void Screensaver::setUpdateBackground()
{
    if(isAutoSwitch){
        if(!m_timer){
            m_timer = new QTimer(this);
            connect(m_timer, SIGNAL(timeout()), this, SLOT(updateBackground()));
        }

        m_timer->start(300000);
    }
    else{
        if(m_timer && m_timer->isActive())
            m_timer->stop();
    }
}

void Screensaver::updateBackground()
{
    QString path = m_background->getNext();
    if(!path.isEmpty()){
        background = QPixmap(path);
        repaint();
    }
    updateCenterWidget(-1);
}

void Screensaver::setCenterWidget()
{
    QStringList qlist = qsettings->childGroups();
    if(qlist.count()<1)
        return;

    QDate date = QDate::currentDate();
    int days = date.daysTo(QDate(2100,1,1));
    int index = days%qlist.count()+1;

    qsettings->beginGroup(QString::number(index));
    if(qsettings->contains("OL")){
        centerlabel1 = new QLabel(qsettings->value("OL").toString());
        centerlabel2 = new QLabel("");
        centerlabel2->hide();
        authorlabel = new QLabel(qsettings->value("author").toString());
    }
    else if(qsettings->contains("FL"))
    {
        centerlabel1 = new QLabel(qsettings->value("FL").toString());
        centerlabel2 = new QLabel(qsettings->value("SL").toString());
        centerlabel2->show();
        authorlabel = new QLabel(qsettings->value("author").toString());
    }
    centerlabel1->setObjectName("centerLabel");
    centerlabel2->setObjectName("centerLabel");
    authorlabel->setObjectName("authorLabel");

    qsettings->endGroup();

    centerWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centerWidget);

    QPushButton *line =new QPushButton(this);
    line->setWindowOpacity(0.08);
    line->setFocusPolicy(Qt::NoFocus);
    line->setMaximumHeight(1);

    layout->addWidget(centerlabel1);
    layout->addWidget(centerlabel2);
    layout->addWidget(line);
    layout->addWidget(authorlabel);

    adjustSize();
    centerWidget->setVisible(true);

}

void Screensaver::setDesktopBackground()
{
     vboxFrame->hide();
    if(m_background->getCurrent().isEmpty())
        return;

    settings->set("picture-filename",QVariant(m_background->getCurrent()));

    QDBusInterface * interface = new QDBusInterface("org.freedesktop.Accounts",
                                     "/org/freedesktop/Accounts",
                                     "org.freedesktop.Accounts",
                                     QDBusConnection::systemBus());

    if (!interface->isValid()){
        return;
    }

    QDBusReply<QDBusObjectPath> reply =  interface->call("FindUserByName", getenv("USER"));
    QString userPath;
    if (reply.isValid()){
        userPath = reply.value().path();
    }
    else {
        return;
    }

    QDBusInterface * useriFace = new QDBusInterface("org.freedesktop.Accounts",
                                                    userPath,
                                                    "org.freedesktop.Accounts.User",
                                                    QDBusConnection::systemBus());

    if (!useriFace->isValid()){
        return;
    }

    QDBusMessage msg = useriFace->call("SetBackgroundFile", m_background->getCurrent());
    if (!msg.errorMessage().isEmpty())
        qDebug() << "update user background file error: " << msg.errorMessage();

}
