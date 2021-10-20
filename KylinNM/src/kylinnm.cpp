/*
 * Copyright (C) 2020 Tianjin KYLIN Information Technology Co., Ltd.
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
 * along with this program; if not, see <http://www.gnu.org/licenses/&gt;.
 *
 */

#include <QTranslator>
#include <QLocale>
#include <QApplication>
#include <QMutex>

#include "kylinnm.h"
#include "ui_kylinnm.h"
#include "swipegesturerecognizer.h"

QString llname, lwname, hideWiFiConn;
int currentActWifiSignalLv, count_loop;

bool KylinNM::m_is_reflashWifiUi = false;
QMutex mutexIsReflashWifi;

KylinNM::KylinNM(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinNM)
{
    ui->setupUi(this);
    qDebug() << "99999999999999999999999999--------------9";
    //checkSingle();

    syslog(LOG_DEBUG, "Using the icon theme named 'ukui-icon-theme-default'");
    //QIcon::setThemeName("ukui-icon-theme-default");

    // 如果使用Qt::Popup 任务栏不显示且保留X事件如XCB_FOCUS_OUT, 但如果indicator点击鼠标右键触发，XCB_FOCUS_OUT事件依然会失效
    // 如果使用Qt::ToolTip, Qt::Tool + Qt::WindowStaysOnTopHint, Qt::X11BypassWindowManagerHint等flag则会导致X事件失效
    // this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    this->setWindowFlags(Qt::FramelessWindowHint);//QTool
    this->setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明

    //UseQssFile::setStyle("style.qss");

    QPainterPath path;
    auto rect = this->rect();
    rect.adjust(1, 1, -1, -1);
    path.addRoundedRect(rect, 6, 6);
    setProperty("blurRegion", QRegion(path.toFillPolygon().toPolygon()));

    this->setStyleSheet("QWidget{border:none;border-radius:16px;}");

    ui->centralWidget->setStyleSheet("#centralWidget{border:none;border-radius:16px;background:rgba(255,255,255,1);}");

    editQssString(); //编辑部分控件QSS
    createTopLanUI(); //创建顶部有线网item
    createTopWifiUI(); //创建顶部无线网item
    createOtherUI(); //创建上传下载控件，列表区无item时的说明控件
    createListAreaUI(); //创建列表区域的控件
    createLeftAreaUI(); //创建左侧区域控件

    lname = "-1";
    wname = "-1";
    llname = "-1";
    lwname = "-1";
    hideWiFiConn = "Connect to Hidden Wi-Fi Network";
    currentActWifiSignalLv = -1;
    count_loop = 0;

    createTrayIcon();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &KylinNM::iconActivated);
    connect(mShowWindow,SIGNAL(triggered()),this,SLOT(on_showWindowAction()));
    connect(mAdvConf,SIGNAL(triggered()),this,SLOT(on_btnAdvConf_clicked()));
    trayIcon->show();

    objKyDBus = new KylinDBus(this);
    objKyDBus->initBtnWifiGsetting();
    objKyDBus->initConnectionInfo();

    objKyDBus->initTaskbarGsetting();
//    objKyDBus->setWifiSignal(-1, "");

    objNetSpeed = new NetworkSpeed();

    this->confForm = new ConfForm(this->parentWidget());
    this->confForm->setMainWindow(this);
    this->confForm->hide();

    this->ksnm = new KSimpleNM();
    connect(ksnm, SIGNAL(getLanListFinished(QStringList)), this, SLOT(getLanListDone(QStringList)));
    connect(ksnm, SIGNAL(getWifiListFinished(QStringList)), this, SLOT(getWifiListDone(QStringList)));

    loading = new LoadingDiv(this);
    connect(loading, SIGNAL(toStopLoading() ), this, SLOT(on_checkOverTime() ));

    checkIsWirelessDeviceOn(); //检测无线网卡是否插入
    getInitLanSlist(); //初始化有线网列表
    initNetwork(); //初始化网络
    initTimer(); //初始化定时器

    connect(ui->btnNetList, &QPushButton::clicked, this, &KylinNM::onBtnNetListClicked);
    connect(btnWireless, &SwitchButton::clicked,this, &KylinNM::onBtnWifiClicked);

    //auto app = static_cast<QApplication*>(QCoreApplication::instance());
    //app->setStyle(new CustomStyle()); //设置自定义主题

    ui->btnNetList->setAttribute(Qt::WA_Hover,true);
    ui->btnNetList->installEventFilter(this);
    ui->btnWifiList->setAttribute(Qt::WA_Hover,true);
    ui->btnWifiList->installEventFilter(this);

    SwipeGestureRecognizer *fftRecognizer = new SwipeGestureRecognizer(this);
    Qt::GestureType fftType = QGestureRecognizer::registerRecognizer(fftRecognizer);
//    grabGesture(fftType);
    connect(fftRecognizer, &SwipeGestureRecognizer::onSwipeGesture, this, &KylinNM::onSwipeGesture);
}

KylinNM::~KylinNM()
{
    trayIcon->deleteLater();
    trayIconMenu->deleteLater();
    delete ui;
}

void KylinNM::onSwipeGesture(int dx, int dy)
{
    // qDebug() << "info: [KylinNM] onSwipeGesture dy=" << dy;
    if(scrollAreal->isVisible()) {
        // qDebug() << "info: [KylinNM] onSwipeGesture before moved " << scrollAreal->verticalScrollBar()->value();
        scrollAreal->verticalScrollBar()->setValue(scrollAreal->verticalScrollBar()->value() - dy/5);
        // qDebug() << "info: [KylinNM] onSwipeGesture after moved " << scrollAreal->verticalScrollBar()->value();
    } else if (scrollAreaw->isVisible()) {
        // qDebug() << "info: [KylinNM] onSwipeGesture before moved " << scrollAreaw->verticalScrollBar()->value();
        scrollAreaw->verticalScrollBar()->setValue(scrollAreaw->verticalScrollBar()->value() - dy/5);
        // qDebug() << "info: [KylinNM] onSwipeGesture after moved " << scrollAreaw->verticalScrollBar()->value();
    }
}

void KylinNM::checkSingle()
{
    int fd = 0;
    try {
        QStringList homePath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        QString lockPath = homePath.at(0) + "/.config/kylin-nm-lock";
        fd = open(lockPath.toUtf8().data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

        if (fd < 0) {
            throw -1;
        }
    } catch(...) {
        fd = open("/tmp/kylin-nm-lock", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            exit(0);
        }
    }


    if (lockf(fd, F_TLOCK, 0)) {
        syslog(LOG_ERR, "Can't lock single file, kylin-network-manager is already running!");
        qDebug()<<"Can't lock single file, kylin-network-manager is already running!";
        exit(0);
    }
}

bool KylinNM::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(result);
    if (eventType != "xcb_generic_event_t") {
        return false;
    }

    xcb_generic_event_t *event = (xcb_generic_event_t*)message;

    switch (event->response_type & ~0x80) {
    case XCB_FOCUS_OUT:
        this->hide();
        break;
    }

    return false;
}

bool KylinNM::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->btnNetList) {
        if (event->type() == QEvent::HoverEnter) {
            if (!is_btnNetList_clicked) {
                ui->lbNetListBG->setStyleSheet(btnBgHoverQss);
            }
            return true;
        } else if(event->type() == QEvent::HoverLeave) {
            if (!is_btnNetList_clicked) {
                ui->lbNetListBG->setStyleSheet(btnBgLeaveQss);
            }
            return true;
        }
    }

    if (obj == ui->btnWifiList) {
        if (event->type() == QEvent::HoverEnter) {
            if (!is_btnWifiList_clicked) {
                ui->lbWifiListBG->setStyleSheet(btnBgHoverQss);
            }
            return true;
        } else if(event->type() == QEvent::HoverLeave) {
            if (!is_btnWifiList_clicked) {
                ui->lbWifiListBG->setStyleSheet(btnBgLeaveQss);
            }
            return true;
        }
    }

    return QWidget::eventFilter(obj,event);
}

///////////////////////////////////////////////////////////////////////////////

void KylinNM::tabletStyle()//平板桌面模式特有设置
{
    this->setFixedWidth(446+194*isTabletStyle);
    ui->centralWidget->setFixedWidth(446+194*isTabletStyle);
    ui->btnNetList->setFixedWidth(223+97*isTabletStyle);
    ui->btnWifiList->setFixedWidth(223+97*isTabletStyle);
    ui->btnWifiList->move(223+97*isTabletStyle,0);
    ui->lbWifiListBG->move(223+97*isTabletStyle,0);
    ui->lbNetListImg->move(16+95*isTabletStyle,17);
    ui->lbWifiListImg->move(288+144*isTabletStyle,17);
    scrollAreal->resize(W_SCROLL_AREA+194*isTabletStyle, H_SCROLL_AREA);
    scrollAreaw->resize(W_SCROLL_AREA+194*isTabletStyle, H_SCROLL_AREA);
    lbNetListText->move(98+48*isTabletStyle,2);
    lbWifiListText->move(98+48*isTabletStyle,2);
    confForm->tabletStyle(isTabletStyle);
    btnWireless->move(385+194*isTabletStyle,73);
    lbNoItemTip->move(this->width()/2 - lbNoItemTip->width()/2, this->height()/2+30);
    if(isTabletStyle)
    {
        btnOffQss = "QLabel{min-width: 320px; min-height: 56px;max-width:320px; max-height: 56px;border-radius: 16px; background-color:rgba(255,255,255,0);}";
        btnOnQss = "QLabel{min-width: 320px; min-height: 56px;max-width:320px; max-height: 56px;border-radius: 16px; background-color:rgba(61,107,229,1);}";
    }
    else
    {
        btnOffQss = "QLabel{min-width: 221px; min-height: 56px;max-width:221px; max-height: 56px;border-radius: 16px; background-color:rgba(255,255,255,0)}";
        btnOnQss = "QLabel{min-width: 221px; min-height: 56px;max-width:221px; max-height: 56px;border-radius: 16px; background-color:rgba(61,107,229,1);}";
    }
    if(scrollAreal->isHidden())//首次运行初始化大按钮状态
    {
        ui->btnNetList->setStyleSheet(btnOffQss);
        ui->btnWifiList->setStyleSheet(btnOnQss);
        ui->lbNetListBG->setStyleSheet(btnOffQss);
        ui->lbWifiListBG->setStyleSheet(btnOnQss);
        ui->lbNetListImg->setStyleSheet("QLabel{border-image:url(:/res/l/pb-network-offline.png);background-position:center;background-repeat:no-repeat;}");
        ui->lbWifiListImg->setStyleSheet("QLabel{border-image:url(:/res/x/pb-wifi-y.png);background-position:center;background-repeat:no-repeat;}");
        lbNetListText->setStyleSheet("QLabel{color:rgba(38, 38, 38, 0.75);background-color:transparent;}");
        lbWifiListText->setStyleSheet("QLabel{color:rgba(47, 179, 232, 1);background-color:transparent;}");
    }
}

// 初始化控件、网络、定时器

// 初始化界面各控件
void KylinNM::editQssString()
{
    btnOffQss = "QLabel{min-width: 221px; min-height: 56px;max-width:221px; max-height: 56px;border-radius: 16px; background-color:rgba(38,38,38,0.15)}";
    btnOnQss = "QLabel{min-width: 221px; min-height: 56px;max-width:221px; max-height: 56px;border-radius: 16px; background-color:rgba(255,255,255,0.2)}";
    btnBgOffQss = "QLabel{min-width: 48px; min-height: 22px;max-width:48px; max-height: 22px;border-radius: 16px; background-color:rgba(38,38,38,0.15)}";
    btnBgOnQss = "QLabel{min-width: 48px; min-height: 22px;max-width:48px; max-height: 22px;border-radius: 16px; background-color:rgba(255,255,255,0.2);}";
    btnBgHoverQss = "QLabel{border-radius: 16px; background-color:rgba(38,38,38,0.2)}";
    btnBgLeaveQss = "QLabel{border-radius: 16px; background-color:rgba(38,38,38,0.15)}";
    leftBtnQss = "QPushButton{border:0px;border-radius:4px;background-color:rgba(255,255,255,0);}"
                 "QPushButton:Hover{border:0px solid rgba(255,255,255,0.2);border-radius:4px;background-color:rgba(255,255,255,0.12);}"
                 "QPushButton:Pressed{border-radius:4px;background-color:rgba(255,255,255,0.12);}";
//    funcBtnQss = "QPushButton{border:0px;border-radius:4px;background-color:rgba(255,255,255,0);color:rgba(107,142,235,0.97);font-size:14px;}"
//                 "QPushButton:Hover{border:0px;border-radius:4px;background-color:rgba(255,255,255,0);color:rgba(151,175,241,0.97);font-size:14px;}"
//                 "QPushButton:Pressed{border-radius:4px;background-color:rgba(255,255,255,0);color:rgba(61,107,229,0.97);font-size:14px;}";
    funcBtnQss="QPushButton{border:0px;border-radius:4px;background-color:rgba(255,255,255,0);color:rgba(38,38,38,1);font-size:14px;text-align:left;}";
}

void KylinNM::createTopLanUI()
{
    topLanListWidget = new QWidget(ui->centralWidget);
    topLanListWidget->move(W_LEFT_AREA, Y_TOP_ITEM);
    topLanListWidget->resize(W_TOP_LIST_WIDGET, H_NORMAL_ITEM + H_GAP_UP + X_ITEM);
    /*顶部的一个item*/
    lbTopLanList = new QLabel(topLanListWidget);
    lbTopLanList->setText(tr("Ethernet Networks"));//"可用网络列表"
    lbTopLanList->resize(W_MIDDLE_WORD, H_MIDDLE_WORD);
    lbTopLanList->move(X_MIDDLE_WORD, H_NORMAL_ITEM + H_GAP_UP);
    lbTopLanList->setStyleSheet("QLabel{font-size:14px;color:rgba(38, 38, 38, 0.45);}");
    lbTopLanList->show();
    /*新建有线网按钮*/
//    btnCreateNet = new QPushButton(ui->centralWidget);
//    btnCreateNet->resize(W_BTN_FUN, H_BTN_FUN);
//    btnCreateNet->move(X_BTN_FUN, Y_BTN_FUN);
//    btnCreateNet->setText("   "+tr("New LAN"));//"新建网络"
//    btnCreateNet->setIcon(QIcon(":/res/x/pb-newConn.png"));
//    btnCreateNet->setStyleSheet(funcBtnQss);
//    btnCreateNet->setFocusPolicy(Qt::NoFocus);
//    btnCreateNet->show();
//    connect(btnCreateNet,SIGNAL(clicked()),this,SLOT(onBtnCreateNetClicked()));
}

void KylinNM::createTopWifiUI()
{
    topWifiListWidget = new QWidget(ui->centralWidget);
    topWifiListWidget->move(W_LEFT_AREA, Y_TOP_ITEM);
    topWifiListWidget->resize(W_TOP_LIST_WIDGET+194*isTabletStyle, H_NORMAL_ITEM + H_GAP_UP + X_ITEM);
    /*顶部的一个item*/
    lbTopWifiList = new QLabel(topWifiListWidget);
    lbTopWifiList->setText(tr("Wifi Networks"));//"可用网络列表"
    lbTopWifiList->resize(W_MIDDLE_WORD, H_MIDDLE_WORD);
    lbTopWifiList->move(X_MIDDLE_WORD, H_NORMAL_ITEM + H_GAP_UP);
    lbTopWifiList->setStyleSheet("QLabel{font-size:14px;color:rgba(38, 38, 38, 0.45);}");
    lbTopWifiList->show();

//    /*新建无线网按钮*/
//    if(!btnAddNet){
//        btnAddNet = new QPushButton(ui->centralWidget);
//        connect(btnAddNet,SIGNAL(clicked()),this,SLOT(onBtnAddNetClicked()));
//    }
//    btnAddNet->resize(W_BTN_FUN, H_BTN_FUN);
//    btnAddNet->move(X_BTN_FUN, Y_BTN_FUN);
//    btnAddNet->setText("   "+tr("Hide WiFi"));//"加入网络"
//    btnAddNet->setIcon(QIcon(":/res/x/pb-newConn.png"));
//    btnAddNet->setStyleSheet(funcBtnQss);
//    btnAddNet->setFocusPolicy(Qt::NoFocus);
}

void KylinNM::createOtherUI()
{
    lbLoadDown = new QLabel(ui->centralWidget);
    lbLoadDown->move(X_ITEM + 129, Y_TOP_ITEM + 32);
    lbLoadDown->resize(65, 20);
    lbLoadDownImg = new QLabel(ui->centralWidget);
    lbLoadDownImg->move(X_ITEM + 112, Y_TOP_ITEM + 35);
    lbLoadDownImg->resize(16, 16);

    lbLoadUp = new QLabel(ui->centralWidget);
    lbLoadUp->move(X_ITEM + 187, Y_TOP_ITEM + 32);
    lbLoadUp->resize(65, 20);
    lbLoadUpImg = new QLabel(ui->centralWidget);
    lbLoadUpImg->move(X_ITEM + 170, Y_TOP_ITEM + 35);
    lbLoadUpImg->resize(16, 16);

//YYF    lbLoadDownImg->setStyleSheet("QLabel{background-image:url(:/res/x/load-down.png);}");
//YYF    lbLoadUpImg->setStyleSheet("QLabel{background-image:url(:/res/x/load-up.png);}");

    lbNoItemTip = new QLabel(ui->centralWidget);
    lbNoItemTip->resize(W_NO_ITEM_TIP, H_NO_ITEM_TIP);
    lbNoItemTip->move(this->width()/2 - W_NO_ITEM_TIP/2 + W_LEFT_AREA/2, this->height()/2);
    lbNoItemTip->setStyleSheet("QLabel{border:none;background:transparent;font-size:14px;color:rgba(38, 38, 38, 0.45);}");
    lbNoItemTip->setText(tr("No usable network in the list"));//列表暂无可连接网络
    lbNoItemTip->setAlignment(Qt::AlignCenter);
    lbNoItemTip->hide();
}

void KylinNM::createListAreaUI()
{
    scrollAreal = new QScrollArea(ui->centralWidget);
    scrollAreal->move(W_LEFT_AREA, Y_TOP_ITEM + H_NORMAL_ITEM + H_GAP_UP + X_ITEM + H_GAP_DOWN);
    scrollAreal->resize(W_SCROLL_AREA, H_SCROLL_AREA);
    scrollAreal->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollAreal->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    scrollAreaw = new QScrollArea(ui->centralWidget);
    scrollAreaw->move(W_LEFT_AREA, Y_TOP_ITEM + H_NORMAL_ITEM + H_GAP_UP + X_ITEM + H_GAP_DOWN);
    scrollAreaw->resize(W_SCROLL_AREA, H_SCROLL_AREA);
    scrollAreaw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollAreaw->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    lanListWidget = new QWidget(scrollAreal);
    wifiListWidget = new QWidget(scrollAreaw);
    lbLanList = new QLabel(lanListWidget);
    lbWifiList = new QLabel(wifiListWidget);

    ui->lbNetwork->setStyleSheet("QLabel{font-size:14px;color:rgba(38, 38, 38, 0.45);}");
    ui->lbNetwork->show();

    topLanListWidget->setStyleSheet("QWidget{border:none;}");
    topLanListWidget->setStyleSheet("background-color:transparent;");

    topWifiListWidget->setStyleSheet("QWidget{border:none;}");
    topWifiListWidget->setStyleSheet("background-color:transparent;");

    lbLoadUp->setStyleSheet("QLabel{font-size:14px;color:rgba(38, 38, 38, 0.45);}");
    lbLoadDown->setStyleSheet("QLabel{font-size:14px;color:rgba(38, 38, 38, 0.45);}");
//YYF    lbLoadUp->setText("0KB/s");
//YYF    lbLoadDown->setText("0KB/s ");
//YYF    this->on_setNetSpeed();
    scrollAreal->setStyleSheet("QScrollArea{border:none;}");
    scrollAreal->viewport()->setStyleSheet("background-color:transparent;");
    //scrollAreal->verticalScrollBar()->setStyleSheet(scrollBarQss);

    scrollAreaw->setStyleSheet("QScrollArea{border:none;}");
    scrollAreaw->viewport()->setStyleSheet("background-color:transparent;");
    //scrollAreaw->verticalScrollBar()->setStyleSheet(scrollBarQss);
}

void KylinNM::createLeftAreaUI()
{
    btnWireless = new SwitchButton(this);
  //  btnWireless->setStyleSheet("SwitchButton{border:none;background-color:rgba(255,255,255,0.12);}");
    ui->btnNetList->setFocusPolicy(Qt::NoFocus);
    QString txtEthernet(tr("Ethernet"));
    ui->btnNetList->setToolTip(txtEthernet);
    ui->lbNetListBG->setStyleSheet(btnOffQss);
    lbNetListText =new QLabel(ui->lbNetListBG);
    lbNetListText->setFocusPolicy(Qt::NoFocus);
    lbNetListText->setText(tr("Ethernet"));
    lbNetListText->move(98,2);
    lbNetListText->setStyleSheet("QLabel{color:rgba(47, 179, 232, 1);background-color:transparent;}");
    ui->lbNetListImg->setStyleSheet("QLabel{border-image:url(:/res/l/pb-network-online.png);background-position:center;background-repeat:no-repeat;}");

    ui->btnWifiList->setFocusPolicy(Qt::NoFocus);
    QString txtWifi(tr("Wifi"));
    ui->btnWifiList->setToolTip(txtWifi);
    ui->lbWifiListBG->setStyleSheet(btnOffQss);
    lbWifiListText =new QLabel(ui->lbWifiListBG);
    lbWifiListText->setFocusPolicy(Qt::NoFocus);
    lbWifiListText->setText(tr("Wifi"));
    lbWifiListText->move(98,2);
    lbWifiListText->setStyleSheet("QLabel{color:rgba(38, 38, 38, 0.75);background-color:transparent;}");
    ui->lbWifiListImg->setStyleSheet("QLabel{border-image:url(:/res/x/pb-wifi-n.png);background-position:center;background-repeat:no-repeat;}");

    ui->btnNet->hide();

    btnWireless->move(385,73);

    ui->btnHotspot->setStyleSheet(leftBtnQss);
    ui->btnHotspot->setFocusPolicy(Qt::NoFocus);
    QString txtHotSpot(tr("HotSpot"));
    ui->btnHotspot->setToolTip(txtHotSpot);
    ui->btnHotspot->hide();
    ui->lbHotImg->hide();
    ui->lbHotImg->setStyleSheet("QLabel{background-image:url(:/res/x/hot-spot-off.svg);}");
    ui->lbHotBG->hide();
    ui->lbHotBG->setStyleSheet(btnOffQss);

    ui->btnFlyMode->setStyleSheet(leftBtnQss);
    ui->btnFlyMode->setFocusPolicy(Qt::NoFocus);
    QString txtFlyMode(tr("FlyMode"));
    ui->btnFlyMode->setToolTip(txtFlyMode);
    ui->btnFlyMode->hide();
    ui->lbFlyImg->hide();
    ui->lbFlyImg->setStyleSheet("QLabel{background-image:url(:/res/x/fly-mode-off.svg);}");
    ui->lbFlyBG->hide();
    ui->lbFlyBG->setStyleSheet(btnOffQss);

    ui->btnAdvConf->setStyleSheet(leftBtnQss);
    ui->btnAdvConf->setFocusPolicy(Qt::NoFocus);
    QString txtAdvanced(tr("Advanced"));
    ui->btnAdvConf->setToolTip(txtAdvanced);
    //ui->lbBtnConfImg->setStyleSheet("QLabel{background-image:url(:/res/x/setup.png);}");
    ui->btnConfImg->setStyleSheet("QPushButton{background-image:url(:/res/x/setup.png);}");
    //ui->btnConfImg->setIcon(QIcon::fromTheme("settings-app-symbolic.svg", QIcon(":/res/x/setup.png")) );
}

// 初始化有线网列表
void KylinNM::getInitLanSlist()
{
    const int BUF_SIZE = 1024;
    char buf[BUF_SIZE];

    FILE * p_file = NULL;

    p_file = popen("export LANG='en_US.UTF-8';export LANGUAGE='en_US';nmcli -f type,device,name connection show", "r");
    if (!p_file) {
        syslog(LOG_ERR, "Error occurred when popen cmd 'nmcli connection show'");
        qDebug()<<"Error occurred when popen cmd 'nmcli connection show";
    }
    while (fgets(buf, BUF_SIZE, p_file) != NULL) {
        QString strSlist = "";
        QString line(buf);
        strSlist = line.trimmed();
        if (strSlist.indexOf("UUID") != -1 || strSlist.indexOf("NAME") != -1) {
            oldLanSlist.append(strSlist);
        }
        if (strSlist.indexOf("802-3-ethernet") != -1 || strSlist.indexOf("ethernet") != -1) {
            oldLanSlist.append(strSlist);
        }
    }
    pclose(p_file);
}

// 初始化网络
void KylinNM::initNetwork()
{
    BackThread *bt = new BackThread();
    IFace *iface = bt->execGetIface();

    wname = iface->wname;
    lwname = iface->wname;
    lname = iface->lname;
    llname = iface->lname;

    mwBandWidth = bt->execChkLanWidth(lname);

    // 开关状态
    qDebug()<<"===";
    qDebug()<<"state of network: '0' is connected, '1' is disconnected, '2' is net device switch off";
    syslog(LOG_DEBUG, "state of network: '0' is connected, '1' is disconnected, '2' is net device switch off");
    qDebug()<<"current network state:  lan state ="<<iface->lstate<<",  wifi state ="<<iface->wstate ;
    syslog(LOG_DEBUG, "current network state:  wired state =%d,  wifi state =%d", iface->lstate, iface->wstate);
    qDebug()<<"===";

    //ui->lbBtnNetBG->setStyleSheet(btnOnQss);
    if (iface->wstate == 0 || iface->wstate == 1) {
       // ui->lbBtnWifiBG->setStyleSheet(btnBgOnQss);
        //ui->lbBtnWifiBall->move(X_RIGHT_WIFI_BALL, Y_WIFI_BALL);
        btnWireless->setSwitchStatus(true);
    } else {
        btnWireless->setSwitchStatus(false);
        //ui->lbBtnWifiBG->setStyleSheet(btnBgOffQss);
        //ui->lbBtnWifiBall->move(X_LEFT_WIFI_BALL, Y_WIFI_BALL);
    }

    // 初始化网络列表
    if (iface->wstate != 2) {
        if (iface->wstate == 0) {
            connWifiDone(3);
        } else {
            if (iface->lstate == 0) {
                connLanDone(3);
            }
        }
        on_btnWifiList_clicked();

        ui->btnNetList->setStyleSheet("QPushButton{border:0px solid rgba(255,255,255,0);background-color:rgba(255,255,255,0);}");
        ui->btnWifiList->setStyleSheet("QPushButton{border:none;}");
    } else {
        objKyDBus->setWifiSwitchState(false); //通知控制面板wifi未开启
        if (iface->lstate != 2) {
            if (iface->lstate == 0) {
                connLanDone(3);
            }
            onBtnNetListClicked();

            ui->btnNetList->setStyleSheet("QPushButton{border:0px solid rgba(255,255,255,0);background-color:rgba(255,255,255,0);}");
            ui->btnWifiList->setStyleSheet("QPushButton{border:none;}");
        } else {
            /*没看懂这段断开连接是什么意思，暂时关闭这段操作，会导致页面卡顿、某些情景还会自动断开网络
//            BackThread *m_bt = new BackThread();
//            IFace *m_iface = m_bt->execGetIface();

//            m_bt->disConnLanOrWifi("ethernet");
//            sleep(1);
//            m_bt->disConnLanOrWifi("ethernet");
//            sleep(1);
//            m_bt->disConnLanOrWifi("ethernet");

//            delete m_iface;
//            m_bt->deleteLater();
            */

            char *chr = "nmcli networking on";
            Utils::m_system(chr);
        }
    }
    //第一次加载时，加载完lan列表会继续加载WIFI列表，先加载lan后加载WIFI是为了保证在加载WIFI时打开的计时器不会被直接关闭
    onBtnNetListClicked();
    //平板上默认展示wifi界面
//    on_btnWifiList_clicked();
}

// 初始化定时器
void KylinNM::initTimer()
{
    //循环检测wifi列表的变化，可用于更新wifi列表
    checkWifiListChanged = new QTimer(this);
    checkWifiListChanged->setTimerType(Qt::PreciseTimer);
    QObject::connect(checkWifiListChanged, SIGNAL(timeout()), this, SLOT(on_checkWifiListChanged()));
    checkWifiListChanged->start(7000);

    //网线插入时定时执行
    wiredCableUpTimer = new QTimer(this);
    wiredCableUpTimer->setTimerType(Qt::PreciseTimer);
    QObject::connect(wiredCableUpTimer, SIGNAL(timeout()), this, SLOT(onCarrierUpHandle()));

    //网线拔出时定时执行
    wiredCableDownTimer = new QTimer(this);
    wiredCableDownTimer->setTimerType(Qt::PreciseTimer);
    QObject::connect(wiredCableDownTimer, SIGNAL(timeout()), this, SLOT(onCarrierDownHandle()));

    //定时处理异常网络，即当点击Lan列表按钮时，若lstate=2，但任然有有线网连接的情况
    deleteLanTimer = new QTimer(this);
    deleteLanTimer->setTimerType(Qt::PreciseTimer);
    QObject::connect(deleteLanTimer, SIGNAL(timeout()), this, SLOT(onDeleteLan()));

    //定时获取网速
    setNetSpeed = new QTimer(this);
    setNetSpeed->setTimerType(Qt::PreciseTimer);
//YYF    QObject::connect(setNetSpeed, SIGNAL(timeout()), this, SLOT(on_setNetSpeed()));
    setNetSpeed->start(3000);
}


///////////////////////////////////////////////////////////////////////////////
// 任务栏托盘管理、托盘图标处理

void KylinNM::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon();
    trayIcon->setToolTip(QString(tr("kylin-nm")));

    trayIconMenu = new QMenu();

    mShowWindow = new QAction(tr("Show KylinNM"),this);
    mAdvConf = new QAction(tr("Advanced"),this);
    mAdvConf->setIcon(QIcon::fromTheme("document-page-setup", QIcon(":/res/x/setup.png")) );

    trayIconMenu->addAction(mShowWindow);
    //trayIconMenu->addSeparator();
    trayIconMenu->addAction(mAdvConf);
    //trayIconMenu->setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明
    //trayIconMenu->setWindowOpacity(0.8);
    trayIcon->setContextMenu(trayIconMenu);

    // 初始化托盘所有Icon
    iconLanOnline = QIcon::fromTheme("network-wired-symbolic");
    iconLanOffline = QIcon::fromTheme("network-wired-offline-symbolic");
    iconWifiFull = QIcon::fromTheme("network-wireless-signal-excellent-symbolic");
    iconWifiHigh = QIcon::fromTheme("network-wireless-signal-good-symbolic");
    iconWifiMedium = QIcon::fromTheme("network-wireless-signal-ok");
    iconWifiLow = QIcon::fromTheme("network-wireless-signal-low");

    loadIcons.append(QIcon::fromTheme("kylin-network-1"));
    loadIcons.append(QIcon::fromTheme("kylin-network-2"));
    loadIcons.append(QIcon::fromTheme("kylin-network-3"));
    loadIcons.append(QIcon::fromTheme("kylin-network-4"));
    loadIcons.append(QIcon::fromTheme("kylin-network-5"));
    loadIcons.append(QIcon::fromTheme("kylin-network-6"));
    loadIcons.append(QIcon::fromTheme("kylin-network-7"));
    loadIcons.append(QIcon::fromTheme("kylin-network-8"));
    loadIcons.append(QIcon::fromTheme("kylin-network-9"));
    loadIcons.append(QIcon::fromTheme("kylin-network-10"));
    loadIcons.append(QIcon::fromTheme("kylin-network-11"));
    loadIcons.append(QIcon::fromTheme("kylin-network-12"));

    iconTimer = new QTimer(this);
    connect(iconTimer, SIGNAL(timeout()), this, SLOT(iconStep()));

    setTrayIcon(iconLanOnline);
}

void KylinNM::iconStep()
{
    if (currentIconIndex < 0) {
        currentIconIndex = 11;
    }
    setTrayIcon(loadIcons.at(currentIconIndex));
    currentIconIndex --;
}

void KylinNM::setTrayIcon(QIcon icon)
{
    trayIcon->setIcon(icon);
}

void KylinNM::setTrayLoading(bool isLoading)
{
    if (isLoading) {
        currentIconIndex = 11;
        iconTimer->start(60);
    } else {
        iconTimer->stop();
    }
}

void KylinNM::updateNetList()
{
    QString strTrans;
    strTrans =  QString::number(1, 10, 2);
    //QString sty = "#centralWidget{background:rgba(19,19,20," + strTrans + ");}";   //YYF
    QString sty = "#centralWidget{background:rgba(255,255,255," + strTrans + ");}";
    ui->centralWidget->setStyleSheet(sty);

    this->showNormal();
    if (is_btnNetList_clicked == 1) {
        onBtnNetListClicked(0);
    }
    is_stop_check_net_state = 1;
    if (is_btnWifiList_clicked == 1) {
        BackThread *loop_bt = new BackThread();
        IFace *loop_iface = loop_bt->execGetIface();

        if (loop_iface->wstate != 2) {
            is_update_wifi_list = 1;
            this->ksnm->execGetWifiList(); //更新wifi列表
        }

        delete loop_iface;
        loop_bt->deleteLater();
    }
    is_stop_check_net_state = 0;
}

void KylinNM::updateWifiList()
{
    //每次展示都要显示无线
    on_btnWifiList_clicked();
}

void KylinNM::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::MiddleClick:

        handleIconClicked();

        if (this->isHidden()) {
            QString strTrans;
            strTrans =  QString::number(1, 10, 2);
            //QString sty = "#centralWidget{background:rgba(19,19,20," + strTrans + ");}";   //YYF
            QString sty = "#centralWidget{background:rgba(255,255,255," + strTrans + ");}";
            ui->centralWidget->setStyleSheet(sty);

            this->showNormal();
            if (is_btnNetList_clicked == 1) {
                onBtnNetListClicked(0);
            }
            is_stop_check_net_state = 1;
            if (is_btnWifiList_clicked == 1) {
                BackThread *loop_bt = new BackThread();
                IFace *loop_iface = loop_bt->execGetIface();

                if (loop_iface->wstate != 2) {
                    is_update_wifi_list = 1;
                    this->ksnm->execGetWifiList(); //更新wifi列表
                }

                delete loop_iface;
                loop_bt->deleteLater();
            }
            is_stop_check_net_state = 0;
        } else {
            this->hide();
        }
        break;
    case QSystemTrayIcon::DoubleClick:
        this->hide();
        break;
    case QSystemTrayIcon::Context:
        //右键点击托盘图标弹出菜单
        showTrayIconMenu();
        break;
    default:
        break;
    }
}

void KylinNM::handleIconClicked()
{
    tabletStyle();
    if(isTabletStyle)//平板桌面模式
    {
        //在屏幕中央显示
        QRect availableGeometry = qApp->primaryScreen()->availableGeometry();
        this->move((availableGeometry.width() - this->width())/2, (availableGeometry.height() - this->height())/2);
        return;
    }

    QRect availableGeometry = qApp->primaryScreen()->availableGeometry();
    QRect screenGeometry = qApp->primaryScreen()->geometry();

    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect deskMainRect = desktopWidget->availableGeometry(0);//获取可用桌面大小
    QRect screenMainRect = desktopWidget->screenGeometry(0);//获取设备屏幕大小
    QRect deskDupRect = desktopWidget->availableGeometry(1);//获取可用桌面大小
    QRect screenDupRect = desktopWidget->screenGeometry(1);//获取设备屏幕大小

    int n = objKyDBus->getTaskBarPos("position");
    int m = objKyDBus->getTaskBarHeight("height");
    int d = 2; //窗口边沿到任务栏距离

    if (screenGeometry.width() == availableGeometry.width() && screenGeometry.height() == availableGeometry.height()) {
        if (n == 0) {
            //任务栏在下侧
            this->move(availableGeometry.x() + availableGeometry.width() - this->width(), screenMainRect.y() + availableGeometry.height() - this->height() - m - d);
        } else if(n == 1) {
            //任务栏在上侧
            this->move(availableGeometry.x() + availableGeometry.width() - this->width(), screenMainRect.y() + screenGeometry.height() - availableGeometry.height() + m + d);
        } else if (n == 2) {
            //任务栏在左侧
            if (screenGeometry.x() == 0) {//主屏在左侧
                this->move(m + d, screenMainRect.y() + screenMainRect.height() - this->height());
            } else {//主屏在右侧
                this->move(screenMainRect.x() + m + d, screenMainRect.y() + screenMainRect.height() - this->height());
            }
        } else if (n == 3) {
            //任务栏在右侧
            if (screenGeometry.x() == 0) {//主屏在左侧
                this->move(screenMainRect.width() - this->width() - m - d, screenMainRect.y() + screenMainRect.height() - this->height());
            } else {//主屏在右侧
                this->move(screenMainRect.x() + screenMainRect.width() - this->width() - m - d, screenMainRect.y() + screenMainRect.height() - this->height());
            }
        }
    } else if(screenGeometry.width() == availableGeometry.width() ) {
        if (trayIcon->geometry().y() > availableGeometry.height()/2) {
            //任务栏在下侧
            this->move(availableGeometry.x() + availableGeometry.width() - this->width(), screenMainRect.y() + availableGeometry.height() - this->height() - d);
        } else {
            //任务栏在上侧
            this->move(availableGeometry.x() + availableGeometry.width() - this->width(), screenMainRect.y() + screenGeometry.height() - availableGeometry.height() + d);
        }
    } else if (screenGeometry.height() == availableGeometry.height()) {
        if (trayIcon->geometry().x() > availableGeometry.width()/2) {
            //任务栏在右侧
            this->move(availableGeometry.x() + availableGeometry.width() - this->width() - d, screenMainRect.y() + screenGeometry.height() - this->height());
        } else {
            //任务栏在左侧
            this->move(screenGeometry.width() - availableGeometry.width() + d, screenMainRect.y() + screenGeometry.height() - this->height());
        }
    }
}

void KylinNM::showTrayIconMenu()
{
    QRect availableGeometry = qApp->primaryScreen()->availableGeometry();
    QRect screenGeometry = qApp->primaryScreen()->geometry();

    QDesktopWidget* desktopWidget = QApplication::desktop();
    // QRect deskMainRect = desktopWidget->availableGeometry(0);//获取可用桌面大小
    QRect screenMainRect = desktopWidget->screenGeometry(0);//获取设备屏幕大小
    // QRect deskDupRect = desktopWidget->availableGeometry(1);//获取可用桌面大小
    QRect screenDupRect = desktopWidget->screenGeometry(1);//获取设备屏幕大小

    QPoint cursorPoint =  QCursor::pos();//返回相对显示器的全局坐标
    int cursor_x = cursorPoint.x();
    int cursor_y = cursorPoint.y();

    int n = objKyDBus->getTaskBarPos("position");
    int m = objKyDBus->getTaskBarHeight("height");
    int d = 0; //窗口边沿到任务栏距离
    int s = 80; //窗口边沿到屏幕边沿距离

    if (screenGeometry.width() == availableGeometry.width() && screenGeometry.height() == availableGeometry.height()) {
        if (n == 0) { //任务栏在下侧
            trayIconMenu->move(availableGeometry.x() + cursor_x - trayIconMenu->width()/2, screenMainRect.y() + availableGeometry.height() - trayIconMenu->height() - m - d);
        } else if(n == 1) { //任务栏在上侧
            trayIconMenu->move(availableGeometry.x() + cursor_x - trayIconMenu->width()/2, screenMainRect.y() + screenGeometry.height() - availableGeometry.height() + m + d);
        } else if (n == 2) { //任务栏在左侧
            trayIconMenu->move(m + d, cursor_y - trayIconMenu->height()/2);
            //if (screenGeometry.x() == 0){//主屏在左侧
            //    trayIconMenu->move(screenGeometry.width() - availableGeometry.width() + m + d, screenMainRect.y() + screenMainRect.height() - trayIconMenu->height() - s);
            //}else{//主屏在右侧
            //    trayIconMenu->move(screenGeometry.width() - availableGeometry.width() + m + d,screenDupRect.y() + screenDupRect.height() - trayIconMenu->height() - s);
            //}
        } else if (n == 3) { //任务栏在右侧
            trayIconMenu->move(screenMainRect.width() - trayIconMenu->width() - m - d, cursor_y - trayIconMenu->height()/2);
            //if (screenGeometry.x() == 0){//主屏在左侧
            //    trayIconMenu->move(screenMainRect.width() + screenDupRect.width() - trayIconMenu->width() - m - d, screenDupRect.y() + screenDupRect.height() - trayIconMenu->height() - s);
            //}else{//主屏在右侧
            //    trayIconMenu->move(availableGeometry.x() + availableGeometry.width() - trayIconMenu->width() - m - d, screenMainRect.y() + screenMainRect.height() - trayIconMenu->height() - s);
            //}
        }
    } else if(screenGeometry.width() == availableGeometry.width() ) {
        if (trayIcon->geometry().y() > availableGeometry.height()/2) { //任务栏在下侧
            trayIconMenu->move(availableGeometry.x() + cursor_x - trayIconMenu->width()/2, screenMainRect.y() + availableGeometry.height() - trayIconMenu->height() - d);
        } else { //任务栏在上侧
            trayIconMenu->move(availableGeometry.x() + cursor_x - trayIconMenu->width()/2, screenMainRect.y() + screenGeometry.height() - availableGeometry.height() + d);
        }
    } else if (screenGeometry.height() == availableGeometry.height()) {
        if (trayIcon->geometry().x() > availableGeometry.width()/2) { //任务栏在右侧
            trayIconMenu->move(availableGeometry.x() + availableGeometry.width() - trayIconMenu->width() - d, cursor_y - trayIconMenu->height()/2);
        } else { //任务栏在左侧
            trayIconMenu->move(screenGeometry.width() - availableGeometry.width() + d, cursor_y - trayIconMenu->height()/2);
        }
    }
}

void KylinNM::on_showWindowAction()
{
    handleIconClicked();
    this->showNormal();
}


///////////////////////////////////////////////////////////////////////////////
//加载动画,获取当前连接的网络和状态并设置图标

void KylinNM::startLoading()
{
//    loading->startLoading();
    setTrayLoading(true);
}

void KylinNM::stopLoading()
{
    loading->stopLoading();
    setTrayLoading(false);
    getActiveInfo();
}

void KylinNM::on_checkOverTime()
{
    QString cmd = "kill -9 $(pidof nmcli)"; //杀掉当前正在进行的有关nmcli命令的进程
    int status = system(cmd.toUtf8().data());
    if (status != 0) {
        qDebug()<<"execute 'kill -9 $(pidof nmcli)' in function 'on_checkOverTime' failed";
        syslog(LOG_ERR, "execute 'kill -9 $(pidof nmcli)' in function 'on_checkOverTime' failed");
    }
    this->stopLoading(); //超时停止等待动画
    is_stop_check_net_state = 0;
}

void KylinNM::getActiveInfo()
{
    QString actLanName = "--";
    QString actWifiName = "--";

    activecon *act = kylin_network_get_activecon_info();
    int index = 0;
    while (act[index].con_name != NULL) {
        if (QString(act[index].type) == "ethernet" || QString(act[index].type) == "802-3-ethernet") {
            actLanName = QString(act[index].con_name);
        }
        if (QString(act[index].type) == "wifi" || QString(act[index].type) == "802-11-wireless") {
            actWifiName = QString(act[index].con_name);
        }
        index ++;
    }


    //ukui3.0中获取currentActWifiSignalLv的值
    if (activeWifiSignalLv > 75) {
        currentActWifiSignalLv = 1;
    } else if(activeWifiSignalLv > 55 && activeWifiSignalLv <= 75) {
        currentActWifiSignalLv = 2;
    } else if(activeWifiSignalLv > 35 && activeWifiSignalLv <= 55) {
        currentActWifiSignalLv = 3;
    } else if( activeWifiSignalLv <= 35) {
        currentActWifiSignalLv = 4;
    }

    // 设置图标
    if (actLanName != "--") {
        setTrayIcon(iconLanOnline);
    } else if (actWifiName != "--") {
        switch (currentActWifiSignalLv) {
        case 1:
            setTrayIcon(iconWifiFull);
            break;
        case 2:
            setTrayIcon(iconWifiHigh);
            break;
        case 3:
            setTrayIcon(iconWifiMedium);
            break;
        case 4:
            setTrayIcon(iconWifiLow);
            break;
        default:
            setTrayIcon(iconWifiFull);
            break;
        }
    } else {
        setTrayIcon(iconLanOffline);
    }
}


///////////////////////////////////////////////////////////////////////////////
//网络设备管理

//网线插拔处理,由kylin-dbus-interface.cpp调用
void KylinNM::onPhysicalCarrierChanged(bool flag)
{
    this->startLoading();
    if (flag) {
        is_stop_check_net_state = 1;
        qDebug()<<"插入了有线网的网线";
        syslog(LOG_DEBUG,"wired physical cable is already plug in");
        wiredCableUpTimer->start(2000);
    } else {
        qDebug()<<"拔出了有线网的网线";
        syslog(LOG_DEBUG,"wired physical cable is already plug out");

        BackThread *bt = new BackThread();
        IFace *iface = bt->execGetIface();
        if (iface->lstate != 0) {
            is_stop_check_net_state = 1;
            wiredCableDownTimer->start(2000);
        }
        delete iface;
        bt->deleteLater();
    }
}

void KylinNM::onCarrierUpHandle()
{
    wiredCableUpTimer->stop();
    //BackThread *up_bt = new BackThread();
    //up_bt->disConnLanOrWifi("ethernet");
    //sleep(1);
    //up_bt->disConnLanOrWifi("ethernet");
    //sleep(1);
    //up_bt->disConnLanOrWifi("ethernet");
    //up_bt->deleteLater();

    this->stopLoading();
    onBtnNetListClicked(1);
    is_stop_check_net_state = 0;
}

void KylinNM::onCarrierDownHandle()
{
    wiredCableDownTimer->stop();
    this->stopLoading();
    onBtnNetListClicked(0);
    is_stop_check_net_state = 0;
}

void KylinNM::onDeleteLan()
{
    deleteLanTimer->stop();
    BackThread *btn_bt = new BackThread();
    btn_bt->disConnLanOrWifi("ethernet");
    sleep(1);
    btn_bt->disConnLanOrWifi("ethernet");
    sleep(1);
    btn_bt->disConnLanOrWifi("ethernet");
    btn_bt->deleteLater();

    this->stopLoading();
    onBtnNetListClicked(0);
    is_stop_check_net_state = 0;
}

//无线网卡插拔处理
void KylinNM::onNetworkDeviceAdded(QDBusObjectPath objPath)
{
    //仅处理无线网卡插入情况
    objKyDBus->isWirelessCardOn = false;
    objKyDBus->getObjectPath();

    if (objKyDBus->wirelessPath.path() == objPath.path()) { //证明添加的是无线网卡
        is_wireless_adapter_ready = 0;
        if (objKyDBus->isWirelessCardOn) {
            syslog(LOG_DEBUG,"wireless device is already plug in");
            qDebug()<<"wireless device is already plug in";
            is_wireless_adapter_ready = 1;
            onBtnWifiClicked(4);
        }
    }
}

void KylinNM::onNetworkDeviceRemoved(QDBusObjectPath objPath)
{
    //仅处理无线网卡拔出情况
    if (objKyDBus->wirelessPath.path() == objPath.path()) {
        objKyDBus->isWirelessCardOn = false;
        objKyDBus->getObjectPath(); //检查是不是还有无线网卡
        if (!objKyDBus->isWirelessCardOn) {
            syslog(LOG_DEBUG,"wireless device is already plug out");
            qDebug()<<"wireless device is already plug out";
            is_wireless_adapter_ready = 0;
            onBtnWifiClicked(5);
        } else {
            syslog(LOG_DEBUG,"wireless device is already plug out, but one more wireless exist");
            qDebug()<<"wireless device is already plug out, but one more wireless exist";
        }
    }
}

void KylinNM::checkIsWirelessDeviceOn()
{
    //启动时判断是否有无线网卡
    //KylinDBus kDBus3;
    if (objKyDBus->isWirelessCardOn) {
        is_wireless_adapter_ready = 1;
    } else {
        is_wireless_adapter_ready = 0;
    }
}

void KylinNM::getLanBandWidth()
{
    BackThread *bt = new BackThread();
    IFace *iface = bt->execGetIface();

    lname = iface->lname;

    mwBandWidth = bt->execChkLanWidth(lname);
}

//检测网络设备状态
bool KylinNM::checkLanOn()
{
    BackThread *bt = new BackThread();
    IFace *iface = bt->execGetIface();

    if (iface->lstate == 2) {
        return false;
    } else {
        return true;
    }

    delete iface;
    bt->deleteLater();
}

bool KylinNM::checkWlOn()
{
    BackThread *bt = new BackThread();
    IFace *iface = bt->execGetIface();

    bool ret = true;
    if (iface->wstate == 2) {
        ret = false;
    }

    delete iface;
    bt->deleteLater();
    return ret;
}


///////////////////////////////////////////////////////////////////////////////
//有线网与无线网按钮响应

void KylinNM::on_btnNet_clicked()
{
    if (checkLanOn()) {
        QThread *t = new QThread();
        BackThread *bt = new BackThread();
        bt->moveToThread(t);
        connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
        connect(t, SIGNAL(started()), bt, SLOT(execDisNet()));
        connect(bt, SIGNAL(disNetDone()), this, SLOT(disNetDone()));
        connect(bt, SIGNAL(btFinish()), t, SLOT(quit()));
        t->start();

    } else {
        is_stop_check_net_state = 1;
        QThread *t = new QThread();
        BackThread *bt = new BackThread();
        bt->moveToThread(t);
        connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
        connect(t, SIGNAL(started()), bt, SLOT(execEnNet()));
        connect(bt, SIGNAL(enNetDone()), this, SLOT(enNetDone()));
        connect(bt, SIGNAL(btFinish()), t, SLOT(quit()));
        t->start();
    }

    this->startLoading();
}

void KylinNM::onBtnWifiClicked(int flag)
{
    qDebug()<<"Value of flag passed into function 'onBtnWifiClicked' is:  "<<flag;
    syslog(LOG_DEBUG, "Value of flag passed into function 'onBtnWifiClicked' is: %d", flag);

    if (is_wireless_adapter_ready == 1) {
        // 当连接上无线网卡时才能打开wifi开关
        // 网络开关关闭时，点击Wifi开关时，程序先打开有线开关
        if (flag == 0 || flag == 1 || flag == 4 || flag == 5) {
            if (checkWlOn()) {
                if (flag != 4) { //以防第二张无线网卡插入时断网
                    is_stop_check_net_state = 1;
                    qDebug() << "aaa111";
                    objKyDBus->setWifiSwitchState(false);
                    lbTopWifiList->hide();
//                    btnAddNet->hide();

                    QThread *t = new QThread();
                    BackThread *bt = new BackThread();
                    bt->moveToThread(t);
                    btnWireless->setSwitchStatus(true);
                    connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
                    connect(t, SIGNAL(started()), bt, SLOT(execDisWifi()));
                    connect(bt, SIGNAL(disWifiDone()), this, SLOT(disWifiDone()));
                    connect(bt, SIGNAL(btFinish()), t, SLOT(quit()));

                    /**锁屏多线程信号槽消息传递异常，暂时以定时器方式检测，需调整pam验证的fork方式
                    */
                    QTimer *timer = new QTimer(this);
                    connect(timer, &QTimer::timeout, this, [=]{
                        if(m_is_reflashWifiUi) {
                            disWifiDone();
                            mutexIsReflashWifi.lock();
                            m_is_reflashWifiUi = false;
                            mutexIsReflashWifi.unlock();
                            timer->stop();
                        }
                    });
                    timer->start(200);
                    QTimer::singleShot(8*1000, this, [=]{
                        if(timer->isActive())
                            timer->stop();
                    });
                    /**锁屏多线程信号槽消息传递异常，暂时以定时器方式检测，需调整pam验证的fork方式
                    */
                    t->start();
                    this->startLoading();
                }
            } else {
                if (is_fly_mode_on == 0) {
                    //on_btnWifiList_clicked();
                    is_stop_check_net_state = 1;
                    qDebug() << "aaa222";
                    objKyDBus->setWifiCardState(true);
                    objKyDBus->setWifiSwitchState(true);
                    //lbTopWifiList->show();
                    //btnAddNet->show();

                    QThread *t = new QThread();
                    BackThread *bt = new BackThread();
                    bt->moveToThread(t);
                    btnWireless->setSwitchStatus(true);
                    connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
                    connect(t, SIGNAL(started()), bt, SLOT(execEnWifi()));
                    connect(bt, SIGNAL(enWifiDone()), this, SLOT(enWifiDone()));
                    connect(bt, SIGNAL(launchLanDone()), this, SLOT(launchLanDone()));
                    connect(bt, SIGNAL(btFinish()), t, SLOT(quit()));

                    /**锁屏多线程信号槽消息传递异常，暂时以定时器方式检测，需调整pam验证的fork方式
                    */
                    QTimer *timer = new QTimer(this);
                    connect(timer, &QTimer::timeout, this, [=]{
                        if(m_is_reflashWifiUi) {
                            enWifiDone();
                            mutexIsReflashWifi.lock();
                            m_is_reflashWifiUi = false;
                            mutexIsReflashWifi.unlock();
                            timer->stop();
                        }
                    });
                    timer->start(200);
                    QTimer::singleShot(8*1000, this, [=]{
                        if(timer->isActive())
                            timer->stop();
                    });
                    t->start();
                    this->startLoading();
                }
            }
        } else if(flag == 2) {
            if (is_fly_mode_on == 0) {
                //on_btnWifiList_clicked();
                is_stop_check_net_state = 1;
                lbTopWifiList->show();
//                btnAddNet->show();

                QThread *t = new QThread();
                BackThread *bt = new BackThread();
                btnWireless->setSwitchStatus(true);
                bt->moveToThread(t);
                connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
                connect(t, SIGNAL(started()), bt, SLOT(execEnWifi()));
                connect(bt, SIGNAL(enWifiDone()), this, SLOT(enWifiDone()));
                connect(bt, SIGNAL(launchLanDone()), this, SLOT(launchLanDone()));
                connect(bt, SIGNAL(btFinish()), t, SLOT(quit()));
                t->start();
                this->startLoading();
            }
        } else if(flag == 3) {
            is_stop_check_net_state = 1;
            lbTopWifiList->hide();
//            btnAddNet->hide();

            QThread *t = new QThread();
            BackThread *bt = new BackThread();
            btnWireless->setSwitchStatus(true);
            bt->moveToThread(t);
            connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
            connect(t, SIGNAL(started()), bt, SLOT(execDisWifi()));
            connect(bt, SIGNAL(disWifiDone()), this, SLOT(disWifiDone()));
            connect(bt, SIGNAL(btFinish()), t, SLOT(quit()));
            t->start();
            this->startLoading();
        } else {
            qDebug()<<"receive an invalid value in function onBtnWifiClicked";
            syslog(LOG_DEBUG, "receive an invalid value in function onBtnWifiClicked");
        }

    } else {
        lbTopWifiList->hide();
//        btnAddNet->hide();

        if (flag == 0) {
             objKyDBus->setWifiSwitchState(false);
             objKyDBus->setWifiCardState(false);
        }

        QString txt(tr("No wireless card detected")); //未检测到无线网卡
        objKyDBus->showDesktopNotify(txt);
        qDebug()<<"No wireless card detected";
        syslog(LOG_DEBUG, "No wireless card detected");
        //QString cmd = "export LANG='en_US.UTF-8';export LANGUAGE='en_US';notify-send '" + txt + "' -t 3800";
        //int status = system(cmd.toUtf8().data());
        //if (status != 0){ syslog(LOG_ERR, "execute 'notify-send' in function 'onBtnWifiClicked' failed");}

        disWifiStateKeep();
    }

}

void KylinNM::onBtnNetListClicked(int flag)
{
    this->is_btnNetList_clicked = 1;
    this->is_btnWifiList_clicked = 0;

    ui->lbNetListBG->setStyleSheet(btnOnQss);
    ui->lbWifiListBG->setStyleSheet(btnOffQss);

    BackThread *bt = new BackThread();
    IFace *iface = bt->execGetIface();

    lbLoadDown->show();
    lbLoadUp->show();
    lbLoadDownImg->show();
    lbLoadUpImg->show();
    if (iface->lstate != 0) {
        lbLoadDown->hide();
        lbLoadUp->hide();
        lbLoadDownImg->hide();
        lbLoadUpImg->hide();
    }

    lbNoItemTip->hide();

    ui->lbNetwork->setText(tr("Ethernet"));
    btnWireless->hide();
    //ui->lbBtnWifiBG->hide();
    //ui->lbBtnWifiBall->hide();

    // 强行设置为打开
    if (flag == 1) {
        this->startLoading();
        this->ksnm->execGetLanList();
        this->scrollAreal->show();
        this->topLanListWidget->show();
        this->scrollAreaw->hide();
        this->topWifiListWidget->hide();
        on_btnNetList_pressed();
        return;
    }

    if (iface->lstate != 2) {
        this->startLoading();
        this->ksnm->execGetLanList();
    } else {
        this->startLoading();
        this->ksnm->execGetLanList();
    }

//    btnCreateNet->show();
//    btnAddNet->hide();
    this->scrollAreal->show();
    this->topLanListWidget->show();
    this->scrollAreaw->hide();
    this->topWifiListWidget->hide();
    on_btnNetList_pressed();
    //YYF
    ui->lbNetListImg->setStyleSheet("QLabel{border-image:url(:/res/l/pb-network-online.png);background-position:center;background-repeat:no-repeat;}");
    ui->lbWifiListImg->setStyleSheet("QLabel{border-image:url(:/res/x/pb-wifi-n.png);background-position:center;background-repeat:no-repeat;}");
    lbNetListText->setStyleSheet("QLabel{color:rgba(47, 179, 232, 1);background-color:transparent;}");
    lbWifiListText->setStyleSheet("QLabel{color:rgba(38, 38, 38, 0.75);background-color:transparent;}");


    delete iface;
    bt->deleteLater();
}

// 当点击wifi标题的时候执行
void KylinNM::on_btnWifiList_clicked()
{
    this->is_btnWifiList_clicked = 1;
    this->is_btnNetList_clicked = 0;

    BackThread *bt = new BackThread();
    IFace *iface = bt->execGetIface();

    lbLoadDown->show();
    lbLoadUp->show();
    lbLoadDownImg->show();
    lbLoadUpImg->show();
    if (iface->wstate != 0) {
        lbLoadDown->hide();
        lbLoadUp->hide();
        lbLoadDownImg->hide();
        lbLoadUpImg->hide();
    }

    ui->lbNetListBG->setStyleSheet(btnOffQss);
    ui->lbWifiListBG->setStyleSheet(btnOnQss);

    lbNoItemTip->hide();
    ui->lbNetwork->setText(tr("Wifi"));
    btnWireless->show();
    //ui->lbBtnWifiBG->show();
    //ui->lbBtnWifiBall->show();
    if (iface->wstate == 0 || iface->wstate == 1) {
        //ui->lbBtnWifiBG->setStyleSheet(btnBgOnQss);
        //ui->lbBtnWifiBall->move(X_RIGHT_WIFI_BALL, Y_WIFI_BALL);
        btnWireless->setSwitchStatus(true);
    } else {
        //ui->lbBtnWifiBG->setStyleSheet(btnBgOffQss);
        //ui->lbBtnWifiBall->move(X_LEFT_WIFI_BALL, Y_WIFI_BALL);
        btnWireless->setSwitchStatus(false);
    }

    if (iface->wstate != 2) {
        //ui->lbBtnWifiBG->setStyleSheet(btnBgOnQss);
        //ui->lbBtnWifiBall->move(X_RIGHT_WIFI_BALL, Y_WIFI_BALL);
        btnWireless->setSwitchStatus(true);
        lbTopWifiList->show();
//        btnAddNet->show();

        this->startLoading();
        this->ksnm->execGetWifiList();
    } else {
        //ui->lbBtnWifiBG->setStyleSheet(btnBgOffQss);
        //ui->lbBtnWifiBall->move(X_LEFT_WIFI_BALL, Y_WIFI_BALL);
        btnWireless->setSwitchStatus(false);
        delete topWifiListWidget; //清空top列表
        createTopWifiUI(); //创建顶部无线网item
        lbTopWifiList->hide();
        ui->lbNetListImg->setStyleSheet("QLabel{border-image:url(:/res/l/pb-network-offline.png);background-position:center;background-repeat:no-repeat;}");
        ui->lbWifiListImg->setStyleSheet("QLabel{border-image:url(:/res/x/pb-wifi-y.png);background-position:center;background-repeat:no-repeat;}");
        lbNetListText->setStyleSheet("QLabel{color:rgba(38, 38, 38, 0.75);background-color:transparent;}");
        lbWifiListText->setStyleSheet("QLabel{color:rgba(47, 179, 232, 1);background-color:transparent;}");

        // 清空wifi列表
        wifiListWidget = new QWidget(scrollAreaw);
        wifiListWidget->resize(W_LIST_WIDGET+173*isTabletStyle, H_WIFI_ITEM_BIG_EXTEND);
        scrollAreaw->setWidget(wifiListWidget);
        scrollAreaw->move(W_LEFT_AREA, Y_SCROLL_AREA);


        // 当前连接的wifi
        OneConnForm *ccf = new OneConnForm(topWifiListWidget, this, confForm, ksnm);
        ccf->setName(tr("Not connected"));//"当前未连接任何 Wifi"
        ccf->setSignal("0", "--");
        ccf->setRate("0");
        ccf->setConnedString(1, tr("Disconnected"), "");//"未连接"
        ccf->isConnected = false;
        ccf->setTopItem(false);
        ccf->setAct(true);
        ccf->move(L_VERTICAL_LINE_TO_ITEM, 0);
        ccf->show();

        this->lanListWidget->hide();
        this->wifiListWidget->show();

        getActiveInfo();
        is_stop_check_net_state = 0;
    }

//    btnCreateNet->hide();
//    if(is_wireless_adapter_ready == 1)
//    {
//        btnAddNet->show();
//    }

    this->scrollAreal->hide();
    this->topLanListWidget->hide();
    this->scrollAreaw->show();
    this->topWifiListWidget->show();
    on_btnWifiList_pressed();

    delete iface;
    bt->deleteLater();
}

void KylinNM::on_btnNetList_pressed()
{
    //ui->btnNetList->setStyleSheet("#btnNetList{font-size:12px;color:white;border:1px solid rgba(255,255,255,0.1);border:1px solid rgba(255,255,255,0.5);background:transparent;background-color:rgba(255,255,255,0.1);}");
    //ui->btnWifiList->setStyleSheet("#btnWifiList{font-size:12px;color:white;border:1px solid rgba(255,255,255,0.1);background:transparent;background-color:rgba(0,0,0,0.2);}"
    //                              "#btnWifiList:Pressed{border:1px solid rgba(255,255,255,0.5);background:transparent;background-color:rgba(255,255,255,0.1);}");
}

void KylinNM::on_btnWifiList_pressed()
{
     //YYF 复制自tabletStyle方法，后期考虑合并
    ui->btnNetList->setStyleSheet(btnOffQss);
    ui->btnWifiList->setStyleSheet(btnOnQss);
    ui->lbNetListBG->setStyleSheet(btnOffQss);
    ui->lbWifiListBG->setStyleSheet(btnOnQss);
    ui->lbNetListImg->setStyleSheet("QLabel{border-image:url(:/res/l/pb-network-offline.png);background-position:center;background-repeat:no-repeat;}");
    ui->lbWifiListImg->setStyleSheet("QLabel{border-image:url(:/res/x/pb-wifi-y.png);background-position:center;background-repeat:no-repeat;}");
    lbNetListText->setStyleSheet("QLabel{color:rgba(38, 38, 38, 0.75);background-color:transparent;}");
    lbWifiListText->setStyleSheet("QLabel{color:rgba(47, 179, 232, 1);background-color:transparent;}");

    //ui->btnWifiList->setStyleSheet("#btnWifiList{font-size:12px;color:white;border:1px solid rgba(255,255,255,0.1);border:1px solid rgba(255,255,255,0.5);background:transparent;background-color:rgba(255,255,255,0.1);}");
    //ui->btnNetList->setStyleSheet("#btnNetList{font-size:12px;color:white;border:1px solid rgba(255,255,255,0.1);background:transparent;background-color:rgba(0,0,0,0.2);}"
    //                              "#btnNetList:Pressed{border:1px solid rgba(255,255,255,0.5);background:transparent;background-color:rgba(255,255,255,0.1);}");
}


///////////////////////////////////////////////////////////////////////////////
//网络列表加载与更新

// 获取lan列表回调
void KylinNM::getLanListDone(QStringList slist)
{
    if (this->ksnm->isUseOldLanSlist) {
        slist = oldLanSlist;
        this->ksnm->isUseOldLanSlist = false;
    }

    delete topLanListWidget; // 清空top列表

    createTopLanUI(); //创建顶部有线网item

    // 清空lan列表
    lanListWidget = new QWidget(scrollAreal);
    lanListWidget->resize(W_LIST_WIDGET, H_NORMAL_ITEM + H_LAN_ITEM_EXTEND);
    scrollAreal->setWidget(lanListWidget);
    scrollAreal->move(W_LEFT_AREA, Y_SCROLL_AREA);
    lanNameList.clear();

    // 获取当前连接的lan name
    QString actLanName = "--";
    activecon *act = kylin_network_get_activecon_info();

    int index = 0;
    while (act[index].con_name != NULL) {
        if (QString(act[index].type) == "ethernet" || QString(act[index].type) == "802-3-ethernet") {
            actLanName = QString(act[index].con_name);
            break;
        }
        index ++;
    }

    // 若当前lan name为"--"，设置OneConnForm
    OneLancForm *ccf = new OneLancForm(topLanListWidget, this, confForm, ksnm);
    topLanListWidget->resize(W_TOP_LIST_WIDGET+173*isTabletStyle, H_NORMAL_ITEM + H_GAP_UP + X_ITEM);
    ccf->setFixedWidth(414+194*isTabletStyle);
    if (actLanName == "--") {
        ccf->setName(tr("Not connected"), "");//"当前未连接任何 以太网"
        ccf->setIcon(false);
        ccf->setConnedString(1, tr("Disconnected"));//"未连接"
        ccf->isConnected = false;
        ifLanConnected = false;
        lbLoadDown->hide();
        lbLoadUp->hide();
        lbLoadDownImg->hide();
        lbLoadUpImg->hide();
        ccf->setTopItem(false);
        ccf->setAct(false);
    }
    else
    ccf->setAct(true);
    ccf->move(L_VERTICAL_LINE_TO_ITEM, 0);
    ccf->show();
    // 填充可用网络列表
    QString headLine = slist.at(0);
    int indexDevice, indexName;
    headLine = headLine.trimmed();

    bool isChineseExist = headLine.contains(QRegExp("[\\x4e00-\\x9fa5]+"));
    if (isChineseExist) {
        indexDevice = headLine.indexOf("设备") + 2;
        indexName = headLine.indexOf("名称") + 4;
    } else {
        indexDevice = headLine.indexOf("DEVICE");
        indexName = headLine.indexOf("NAME");
    }

    QString order = "a"; //为避免同名情况，这里给每一个有线网设定一个唯一标志
    for(int i = 1, j = 0; i < slist.size(); i ++) {
        QString line = slist.at(i);
        QString ltype = line.mid(0, indexDevice).trimmed();
        QString nname = line.mid(indexName).trimmed();

        if (ltype != "802-11-wireless" && ltype != "wifi" && ltype != "" && ltype != "--") {
            lanNameList.append(nname);
            // 当前连接的lan
            if (nname == actLanName) {
                objKyDBus->getConnectNetIp();
                actLanName = "--";
                if (mwBandWidth == "Unknown!") { getLanBandWidth(); }

                connect(ccf, SIGNAL(selectedOneLanForm(QString, QString)), this, SLOT(oneTopLanFormSelected(QString, QString)));
                connect(ccf, SIGNAL(disconnActiveLan()), this, SLOT(activeLanDisconn()));
                ccf->setName(nname, nname + order);
                ccf->setIcon(true);
                ccf->setLanInfo(objKyDBus->dbusActiveLanIpv4, objKyDBus->dbusActiveLanIpv6, mwBandWidth, objKyDBus->dbusLanMac);
                ccf->setConnedString(1, tr("NetOn,"));//"已连接"
                ccf->isConnected = true;
                ifLanConnected = true;
                lbLoadDown->show();
                lbLoadUp->show();
                lbLoadDownImg->show();
                lbLoadUpImg->show();
                ccf->setTopItem(false);
                currSelNetName = "";
                objKyDBus->dbusActiveLanIpv4 = "";
                objKyDBus->dbusActiveLanIpv6 = "";
                syslog(LOG_DEBUG, "already insert an active lannet in the top of lan list");
            } else {
                objKyDBus->getLanIp(nname);
                OneLancForm *ocf = new OneLancForm(lanListWidget, this, confForm, ksnm);
//                connect(ocf, SIGNAL(selectedOneLanForm(QString, QString)), this, SLOT(oneLanFormSelected(QString, QString)));
                ocf->setName(nname, nname + order);
                ocf->setIcon(false); //YYF
                ocf->setLine(true);
                ocf->setLanInfo(objKyDBus->dbusLanIpv4, objKyDBus->dbusLanIpv6, tr("Disconnected"), objKyDBus->dbusLanMac);
                ocf->setConnedString(0, tr("Disconnected"));//"未连接"
                ocf->move(L_VERTICAL_LINE_TO_ITEM, j * H_NORMAL_ITEM);
                lanListWidget->resize(W_LIST_WIDGET+194*isTabletStyle, lanListWidget->height() + H_NORMAL_ITEM);
                ocf->setFixedWidth(W_LIST_WIDGET+194*isTabletStyle); //YYF
                ocf->setSelected(false, false);
                ocf->show();
                j ++;
            }
            order += "a";
        }
    }

    QList<OneLancForm *> itemList = lanListWidget->findChildren<OneLancForm *>();
    int n = itemList.size();
    if (n >= 1) {
        OneLancForm *lastItem = itemList.at(n-1);
        lastItem->setLine(false);
        lbNoItemTip->hide();
    } else {
        if (!ifLanConnected) {
            lbNoItemTip->hide();
//            lbTopLanList->hide();
//            btnCreateNet->hide();
        } else {
            lbNoItemTip->show();
            lbNoItemTip->setText(tr("No Other Wired Network Scheme"));
        }
    }

    this->lanListWidget->show();
    this->topLanListWidget->show();
    this->wifiListWidget->hide();
    this->topWifiListWidget->hide();

    this->stopLoading();
    oldLanSlist = slist;
    is_stop_check_net_state = 0;
}

// 获取wifi列表回调
void KylinNM::getWifiListDone(QStringList slist)
{
    qDebug()<<"debug: oldWifiSlist.size()="<<oldWifiSlist.size()<<"   slist.size()="<<slist.size();

    if (is_update_wifi_list == 0 || oldWifiSlist.size() == 0) {
        loadWifiListDone(slist);
    } else {
        updateWifiListDone(slist);
        is_update_wifi_list = 0;
    }
    oldWifiSlist = slist;
}

// 加载wifi列表
void KylinNM::loadWifiListDone(QStringList slist)
{
    delete topWifiListWidget; //清空top列表
    createTopWifiUI(); //创建topWifiListWidget

    // 清空wifi列表
    wifiListWidget = new QWidget(scrollAreaw);
    wifiListWidget->resize(W_LIST_WIDGET+173*isTabletStyle, H_WIFI_ITEM_BIG_EXTEND);
    scrollAreaw->setWidget(wifiListWidget);
    scrollAreaw->move(W_LEFT_AREA, Y_SCROLL_AREA);

    QList<QString> currConnWifiBSsidUuid;
    currConnWifiBSsidUuid = objKyDBus->getAtiveWifiBSsidUuid(slist);
    // 获取当前连接的wifi name
    QString actWifiName = "--";
    QString actWifiId = "--";
    actWifissid = "--";
    actWifiuuid = "--";
    if (currConnWifiBSsidUuid.size() > 1) {
        actWifiuuid = currConnWifiBSsidUuid.at(0);
        for (int i=1; i<currConnWifiBSsidUuid.size(); i++) {
            actWifiBssidList.append(currConnWifiBSsidUuid.at(i));
        }
    } else {
        actWifiBssidList.append("--");
    }

    activecon *act = kylin_network_get_activecon_info();
    int index = 0;
    while (act[index].con_name != NULL) {
        if (QString(act[index].type) == "wifi" || QString(act[index].type) == "802-11-wireless") {
            actWifiName = QString(act[index].con_name);
            break;
        }
        index ++;
    }
    // 根据当前连接的wifi 设置OneConnForm
    OneConnForm *ccf = new OneConnForm(topWifiListWidget, this, confForm, ksnm);
    if (actWifiName == "--" && actWifiBssidList.at(0) == "--") {
        ccf->setName(tr("Not connected"));//"当前未连接任何 Wifi"
        ccf->setSignal("0", "--");
        activeWifiSignalLv = 0;
        ccf->setConnedString(1, tr("Disconnected"), "");//"未连接"
        ccf->isConnected = false;
        ifWLanConnected = false;
        lbLoadDown->hide();
        lbLoadUp->hide();
        lbLoadDownImg->hide();
        lbLoadUpImg->hide();
        ccf->setTopItem(false);
    }
    ccf->setAct(true);
    ccf->move(L_VERTICAL_LINE_TO_ITEM, 0);
    ccf->show();
    // 填充可用网络列表
    QString headLine = slist.at(0);
    int indexSignal,indexSecu, indexFreq, indexBSsid, indexName, indexPath;
    headLine = headLine.trimmed();

    bool isChineseExist = headLine.contains(QRegExp("[\\x4e00-\\x9fa5]+"));
    if (isChineseExist) {
        indexSignal = headLine.indexOf("SIGNAL");
        indexSecu = headLine.indexOf("安全性");
        indexFreq = headLine.indexOf("频率") + 4;
        indexBSsid = headLine.indexOf("BSSID") + 6;
        indexName = indexBSsid + 19;
        indexPath = headLine.indexOf("DBUS-PATH");
    } else {
        indexSignal = headLine.indexOf("SIGNAL");
        indexSecu = headLine.indexOf("SECURITY");
        indexFreq = headLine.indexOf("FREQ");
        indexBSsid = headLine.indexOf("BSSID");
        indexName = indexBSsid + 19;
        indexPath = headLine.indexOf("DBUS-PATH");
    }
    QStringList wnames;
    int count = 0;
    QString actWifiBssid = " ";
    for (int i = 1; i < slist.size(); i ++) {
        QString line = slist.at(i);
        QString wbssid = line.mid(indexBSsid, 17).trimmed();
        QString wname = line.mid(indexName, indexPath - indexName).trimmed();

        if (actWifiBssidList.contains(wbssid)) {
            actWifiName = wname;
        }
        if ("*" == line.mid(0,indexSignal).trimmed()){
            actWifiBssid = wbssid;
        }
    }

    if (actWifiBssidList.size()==1 && actWifiBssidList.at(0)=="--") {
        actWifiId = actWifiName;
        actWifiName = "--";
    }
    for (int i = 1, j = 0; i < slist.size(); i ++) {
        QString line = slist.at(i);
        //QString wsignal = line.mid(0, indexRate).trimmed();
        //QString wrate = line.mid(indexRate, indexSecu - indexRate).trimmed();
        QString wsignal = line.mid(indexSignal, 3).trimmed();
        QString wsecu = line.mid(indexSecu, indexFreq - indexSecu).trimmed();
        QString wbssid = line.mid(indexBSsid, 17).trimmed();
        QString wname = line.mid(indexName, indexPath - indexName).trimmed();

        bool isContinue = false;
        foreach (QString addName, wnames) {
            // 重复的网络名称，跳过不处理
            if(addName == wname){ isContinue = true; }
        }
        if(isContinue){ continue; }

        if (actWifiName != "--" && actWifiName == wname) {
            if (!actWifiBssidList.contains(wbssid)) {
                continue; //若当前热点ssid名称和已经连接的wifi的ssid名称相同，但bssid不同，则跳过
            }
        }

        if (wname != "" && wname != "--") {
            // 当前连接的wifi
            if (wname == actWifiName) {
                connect(ccf, SIGNAL(selectedOneWifiForm(QString,int)), this, SLOT(oneTopWifiFormSelected(QString,int)));
                connect(ccf, SIGNAL(disconnActiveWifi()), this, SLOT(activeWifiDisconn()));
                QString path = line.mid(indexPath).trimmed();
                QString m_name = this->objKyDBus->getWifiSsid(QString("/org/freedesktop/NetworkManager/AccessPoint/%1").arg(path.mid(path.lastIndexOf("/") + 1)));
//                ccf->setName(m_name);
                if (m_name.isEmpty() || m_name == "") {
                    ccf->setName(wname);
                } else {
                    ccf->setName(m_name);
                }
                //ccf->setRate(wrate);
                int signal = wsignal.toInt() + 11;
                ccf->setSignal(QString::number(signal), wsecu);
                activeWifiSignalLv = wsignal.toInt();
                objKyDBus->getWifiMac(wname);
                ccf->setWifiInfo(wsecu, wsignal, objKyDBus->dbusWifiMac);
                ccf->setConnedString(1, tr("NetOn,"), wsecu);//"已连接"
                ccf->isConnected = true;
                ifWLanConnected = true;
                lbLoadDown->show();
                lbLoadUp->show();
                lbLoadDownImg->show();
                lbLoadUpImg->show();
                ccf->setTopItem(false);
                currSelNetName = "";

                syslog(LOG_DEBUG, "already insert an active wifi in the top of wifi list");
            } else {
                wifiListWidget->resize(W_LIST_WIDGET+173*isTabletStyle, wifiListWidget->height() + H_NORMAL_ITEM);

                OneConnForm *ocf = new OneConnForm(wifiListWidget, this, confForm, ksnm);
                connect(ocf, SIGNAL(selectedOneWifiForm(QString,int)), this, SLOT(oneWifiFormSelected(QString,int)));
                connect(ocf,&OneConnForm::onLineEditClicked, this, &KylinNM::onLineEditClicked);
                QString path = line.mid(indexPath).trimmed();
                QString m_name = this->objKyDBus->getWifiSsid(QString("/org/freedesktop/NetworkManager/AccessPoint/%1").arg(path.mid(path.lastIndexOf("/") + 1)));
//                ocf->setName(m_name);
                if (m_name.isEmpty() || m_name == "") {
                    ocf->setName(wname);
                } else {
                    ocf->setName(m_name);
                }
                //ocf->setRate(wrate);
                ocf->setLine(true);
                ocf->setSignal(wsignal, wsecu);
                objKyDBus->getWifiMac(wname);
                ocf->setWifiInfo(wsecu, wsignal, objKyDBus->dbusWifiMac);
                ocf->setConnedString(0, tr("Disconnected"), wsecu);
                ocf->move(L_VERTICAL_LINE_TO_ITEM, j * H_NORMAL_ITEM);
                ocf->setSelected(false, false);
                ocf->show();

                j ++;
                count ++;
            }

            wnames.append(wname);
        }
    }
    QList<OneConnForm *> itemList = wifiListWidget->findChildren<OneConnForm *>();
    int n = itemList.size();
    if (n >= 1) {
        OneConnForm *lastItem = itemList.at(n-1);
        lastItem->setLine(false);
        lbNoItemTip->hide();
    } else {
        if (ifWLanConnected) {
            lbNoItemTip->show();
            lbNoItemTip->setText(tr("No Other Wireless Network Scheme"));
        } else {
            lbNoItemTip->hide();
            lbTopWifiList->hide();
//            btnAddNet->hide();
        }
    }

    this->lanListWidget->hide();
    this->topLanListWidget->hide();
    this->wifiListWidget->show();
    this->topWifiListWidget->show();

    this->stopLoading();
    is_stop_check_net_state = 0;

    actWifiBssidList.clear();
    wnames.clear();
}

// 更新wifi列表
void KylinNM::updateWifiListDone(QStringList slist)
{
    if (this->ksnm->isExecutingGetLanList){ return;}

    //获取表头信息
    QString lastHeadLine = oldWifiSlist.at(0);
    //int lastIndexName = lastHeadLine.indexOf("SSID");
    int lastIndexName, lastIndexPath;
    lastHeadLine = lastHeadLine.trimmed();
    bool isChineseInIt = lastHeadLine.contains(QRegExp("[\\x4e00-\\x9fa5]+"));
    if (isChineseInIt) {
        lastIndexName = lastHeadLine.indexOf("BSSID") + 6 + 19;
    } else {
        lastIndexName = lastHeadLine.indexOf("BSSID") + 19;
    }
    lastIndexPath = lastHeadLine.indexOf("DBUS-PATH");

    QString headLine = slist.at(0);
    int indexSecu, indexFreq, indexBSsid, indexName, indexPath;
    headLine = headLine.trimmed();
    bool isChineseExist = headLine.contains(QRegExp("[\\x4e00-\\x9fa5]+"));
    if (isChineseExist) {
        indexSecu = headLine.indexOf("安全性");
        indexFreq = headLine.indexOf("频率") + 4;
        indexBSsid = headLine.indexOf("BSSID") + 6;
        //indexName = headLine.indexOf("SSID") + 6;
        indexName = indexBSsid + 19;
        indexPath = headLine.indexOf("DBUS-PATH");
    } else {
        indexSecu = headLine.indexOf("SECURITY");
        indexFreq = headLine.indexOf("FREQ");
        indexBSsid = headLine.indexOf("BSSID");
        indexName = indexBSsid + 19;
        indexPath = headLine.indexOf("DBUS-PATH");
    }

    //列表中去除已经减少的wifi
    for (int i=1; i<oldWifiSlist.size(); i++){
        QString line = oldWifiSlist.at(i);
        QString lastWname = line.mid(lastIndexName, lastIndexPath - lastIndexName).trimmed();
        for (int j=1; j<slist.size(); j++){
            QString line = slist.at(j);
            QString wname = line.mid(indexName, indexPath - indexName).trimmed();

            if (lastWname == wname){break;} //在slist最后之前找到了lastWname，则停止
            if (j == slist.size()-1) {
                syslog(LOG_DEBUG, "Will remove a Wi-Fi, it's name is: %s", lastWname.toUtf8().data());
                qDebug()<<"Will remove a Wi-Fi, it's name is: "<<lastWname;
                QList<OneConnForm *> wifiList = wifiListWidget->findChildren<OneConnForm *>();
                for (int pos = 0; pos < wifiList.size(); pos ++) {
                    OneConnForm *ocf = wifiList.at(pos);
                    if (ocf->getName() == lastWname) {
                        if (ocf->isActive == true){break;
                        } else {
                            delete ocf;
                            //删除元素下面的的所有元素上移
                            for (int after_pos = pos+1; after_pos < wifiList.size(); after_pos ++) {
                                OneConnForm *after_ocf = wifiList.at(after_pos);
                                if (lastWname == currSelNetName) {after_ocf->move(L_VERTICAL_LINE_TO_ITEM, after_ocf->y() - H_NORMAL_ITEM - H_WIFI_ITEM_BIG_EXTEND);}
                                else {after_ocf->move(L_VERTICAL_LINE_TO_ITEM, after_ocf->y() - H_NORMAL_ITEM);}
                            }
                            wifiListWidget->resize(W_LIST_WIDGET+173*isTabletStyle, wifiListWidget->height() - H_NORMAL_ITEM);
                            break;
                        }
                    }
                }

            } //end if (j == slist.size()-1)
        } //end (int j=1; j<slist.size(); j++)
    }
    //列表中插入新增的wifi
    QStringList wnames;
    int count = 0;
    for(int i = 1; i < slist.size(); i++){
        QString line = slist.at(i);
        QString wsignal = line.mid(0, indexSecu).trimmed();
        QString wsecu = line.mid(indexSecu, indexFreq - indexSecu).trimmed();
        QString wname = line.mid(indexName, indexPath - indexName).trimmed();

        if(wname == "" || wname == "--"){continue;}

        bool isContinue = false;
        foreach (QString addName, wnames) {
            // 重复的网络名称，跳过不处理
            if(addName == wname){isContinue = true;}
        }
        if(isContinue){continue;}
        wnames.append(wname);

        for (int j=1; j < oldWifiSlist.size(); j++) {
            QString line = oldWifiSlist.at(j);
            QString lastWname = line.mid(lastIndexName, lastIndexPath - lastIndexName).trimmed();

            if (lastWname == wname.trimmed()){break;} //上一次的wifi列表已经有名为wname的wifi，则停止
            if (j == oldWifiSlist.size()-1) { //到lastSlist最后一个都没找到，执行下面流程
                syslog(LOG_DEBUG, "Will add a Wi-Fi, it's name is: %s", wname.toUtf8().data());
                qDebug()<<"Will add a Wi-Fi, it's name is: "<<wname;
                QList<OneConnForm *> wifiList = wifiListWidget->findChildren<OneConnForm *>();
                int n = wifiList.size();
                int posY = 0;
                if (n >= 1) {
                    OneConnForm *lastOcf = wifiList.at(n-1);
                    lastOcf->setLine(true);
                    if (lastOcf->wifiName == currSelNetName) {
                        posY = lastOcf->y()+H_NORMAL_ITEM + H_WIFI_ITEM_BIG_EXTEND;
                    } else {
                        posY = lastOcf->y()+H_NORMAL_ITEM;
                    }
                }

                wifiListWidget->resize(W_LIST_WIDGET+173*isTabletStyle, wifiListWidget->height() + H_NORMAL_ITEM);
                OneConnForm *addItem = new OneConnForm(wifiListWidget, this, confForm, ksnm);
                connect(addItem, SIGNAL(selectedOneWifiForm(QString,int)), this, SLOT(oneWifiFormSelected(QString,int)));
                connect(addItem,&OneConnForm::onLineEditClicked, this, &KylinNM::onLineEditClicked);
                QString path = slist.at(i).mid(indexPath).trimmed();
                QString m_name = this->objKyDBus->getWifiSsid(QString("/org/freedesktop/NetworkManager/AccessPoint/%1").arg(path.mid(path.lastIndexOf("/") + 1)));
                if (m_name.isEmpty() || m_name == "") {
                    addItem->setName(wname);
                } else {
                    addItem->setName(m_name);
                }
                addItem->setLine(false);
                addItem->setSignal(wsignal, wsecu);
                objKyDBus->getWifiMac(wname);
                addItem->setWifiInfo(wsecu, wsignal, objKyDBus->dbusWifiMac);
                addItem->setConnedString(0, tr("Disconnected"), wsecu);//"未连接"
                addItem->move(L_VERTICAL_LINE_TO_ITEM, posY);
                addItem->setSelected(false, false);
                addItem->show();

                count += 1;
            }
        }
    }

    this->lanListWidget->hide();
    this->topLanListWidget->hide();
    this->wifiListWidget->show();
    this->topWifiListWidget->show();
    this->stopLoading();
}


///////////////////////////////////////////////////////////////////////////////
//主窗口其他按钮点击响应

void KylinNM::on_btnAdvConf_clicked()
{
    QProcess *qprocess = new QProcess(this);
    qprocess->start("nm-connection-editor &");
    // int status = system("nm-connection-editor &");
    // if (status != 0){ syslog(LOG_ERR, "execute 'nm-connection-editor &' in function 'on_btnAdvConf_clicked' failed");}
}

void KylinNM::on_btnAdvConf_pressed()
{
    //ui->lbBtnConfBG->setStyleSheet(btnOnQss);
}

void KylinNM::on_btnAdvConf_released()
{
    //ui->lbBtnConfBG->setStyleSheet(btnOffQss);
}

void KylinNM::on_btnFlyMode_clicked()
{
    if (is_fly_mode_on == 0) {
        ui->lbFlyImg->setStyleSheet("QLabel{background-image:url(:/res/x/fly-mode-on.svg);}");
        ui->lbFlyBG->setStyleSheet(btnOnQss);
        is_fly_mode_on = 1;

        onBtnWifiClicked(0);
        on_btnWifiList_clicked();
    } else {
        ui->lbFlyImg->setStyleSheet("QLabel{background-image:url(:/res/x/fly-mode-off.svg);}");
        ui->lbFlyBG->setStyleSheet(btnOffQss);
        is_fly_mode_on = 0;
    }
}

void KylinNM::on_btnHotspot_clicked()
{
    if (is_wireless_adapter_ready == 1) {
        if (is_hot_sopt_on == 0) {
            ui->lbHotImg->setStyleSheet("QLabel{background-image:url(:/res/x/hot-spot-on.svg);}");
            ui->lbHotBG->setStyleSheet(btnOnQss);
            is_hot_sopt_on = 1;

            QApplication::setQuitOnLastWindowClosed(false);
            DlgHotspotCreate *hotCreate = new DlgHotspotCreate(objKyDBus->dbusWiFiCardName);
            connect(hotCreate,SIGNAL(updateHotspotList()),this,SLOT(on_btnWifiList_clicked() ));
            connect(hotCreate,SIGNAL(btnHotspotState()),this,SLOT(on_btnHotspotState() ));
            hotCreate->show();
        } else {
            on_btnHotspotState();

            BackThread objBT;
            objBT.disConnLanOrWifi("wifi");

            sleep(2);
            on_btnWifiList_clicked();
        }
    }
}

void KylinNM::onBtnAddNetClicked()
{
//    QApplication::setQuitOnLastWindowClosed(false);
    DlgConnHidWifi *connHidWifi = new DlgConnHidWifi(0, this, this->parentWidget());
    connect(connHidWifi, SIGNAL(reSetWifiList() ), this, SLOT(on_btnWifiList_clicked()) );
    connHidWifi->show();
}

void KylinNM::onBtnCreateNetClicked()
{
    QPoint pos = QCursor::pos();
    QRect primaryGeometry;
    for (QScreen *screen : qApp->screens()) {
        if (screen->geometry().contains(pos)) {
            primaryGeometry = screen->geometry();
        }
    }

    if (primaryGeometry.isEmpty()) {
        primaryGeometry = qApp->primaryScreen()->geometry();
    }

    ConfForm *m_cf = new ConfForm(this->parentWidget());
    m_cf->setMainWindow(this);
    m_cf->cbTypeChanged(3);
    m_cf->move(primaryGeometry.width() / 2 - m_cf->width() / 2, primaryGeometry.height() / 2 - m_cf->height() / 2);
    m_cf->show();
}


///////////////////////////////////////////////////////////////////////////////
//处理窗口变化、网络状态变化

//列表中item的扩展与收缩
void KylinNM::oneLanFormSelected(QString lanName, QString uniqueName)
{
    QList<OneLancForm *> topLanList = topLanListWidget->findChildren<OneLancForm *>();
    QList<OneLancForm *> lanList = lanListWidget->findChildren<OneLancForm *>();

    //**********************先处理下方列表********************//
    // 下方所有元素回到原位
    for (int i = 0, j = 0;i < lanList.size(); i ++) {
        OneLancForm *ocf = lanList.at(i);
        if (ocf->isActive == true) {
            ocf->move(L_VERTICAL_LINE_TO_ITEM, 0);
        }
        if (ocf->isActive == false) {
            ocf->move(L_VERTICAL_LINE_TO_ITEM, j * H_NORMAL_ITEM);
            j ++;
        }
    }

    //是否与上一次选中同一个网络框
    if (currSelNetName == uniqueName) {
        // 缩小所有选项卡
        for (int i = 0;i < lanList.size(); i ++) {
            OneLancForm *ocf = lanList.at(i);
            if (ocf->uniqueName == uniqueName) {
                ocf->setSelected(false, true);
            } else {
                ocf->setSelected(false, false);
            }
        }

        currSelNetName = "";
    } else {
        int selectY = 0;
        for (int i = 0;i < lanList.size(); i ++) {
            OneLancForm *ocf = lanList.at(i);
            if (ocf->uniqueName == uniqueName) {
                selectY = ocf->y(); //获取选中item的y坐标
                break;
            }
        }

        // 选中元素下面的所有元素下移 H_LAN_ITEM_EXTEND
        for (int i = 0;i < lanList.size(); i ++) {
            OneLancForm *ocf = lanList.at(i);
            if (ocf->y() > selectY) {
                ocf->move(L_VERTICAL_LINE_TO_ITEM, ocf->y() + H_LAN_ITEM_EXTEND);
            }
        }

        for (int i = 0;i < lanList.size(); i ++) {
            OneLancForm *ocf = lanList.at(i);
            if (ocf->uniqueName == uniqueName) {
                ocf->setSelected(true, false);
                selectY = ocf->y();
            } else {
                ocf->setSelected(false, false);
            }
        }

        currSelNetName = uniqueName;
    }

    QList<OneLancForm *> itemList = lanListWidget->findChildren<OneLancForm *>();
    int n = itemList.size();
    if (n >= 1) {
        OneLancForm *lastItem = itemList.at(n-1);
        lastItem->setLine(false);
    }

    //**********************处理上方列表-界面所有控件回原位********************//
    topLanListWidget->resize(W_TOP_LIST_WIDGET, H_NORMAL_ITEM + H_GAP_UP + X_ITEM); // 顶部的item缩小
    lbTopLanList->move(X_MIDDLE_WORD, H_NORMAL_ITEM + H_GAP_UP);
//    btnCreateNet->move(X_BTN_FUN, Y_BTN_FUN);
    scrollAreal->move(W_LEFT_AREA, Y_SCROLL_AREA);
    lbNoItemTip->move(this->width()/2 - W_NO_ITEM_TIP/2 + W_LEFT_AREA/2, this->height()/2);

    OneLancForm *ocf = topLanList.at(0);
    ocf->setTopItem(false);
}
void KylinNM::oneTopLanFormSelected(QString lanName, QString uniqueName)
{
        currSelNetName = uniqueName;
}

void KylinNM::oneWifiFormSelected(QString wifiName, int extendLength)
{
    QList<OneConnForm *>topWifiList = topWifiListWidget->findChildren<OneConnForm *>();
    QList<OneConnForm *> wifiList = wifiListWidget->findChildren<OneConnForm *>();

    //******************先处理下方列表****************//
    // 下方所有元素回到原位
    for (int i = 0, j = 0;i < wifiList.size(); i ++) {
        OneConnForm *ocf = wifiList.at(i);
        if (ocf->isActive == true) {
            ocf->move(L_VERTICAL_LINE_TO_ITEM, 0);
        }
        if (ocf->isActive == false) {
            ocf->move(L_VERTICAL_LINE_TO_ITEM, j * H_NORMAL_ITEM);
            j ++;
        }
    }

    //是否与上一次选中同一个网络框
    if (currSelNetName == wifiName) {
        // 缩小所有选项卡
        for (int i = 0;i < wifiList.size(); i ++) {
            OneConnForm *ocf = wifiList.at(i);
            if (ocf->wifiName == wifiName) {
                if (ocf->wifiName == hideWiFiConn) {
                    ocf->setHideItem(true, true);
                } else {
                    ocf->setSelected(false, true);
                }
            } else {
                if (ocf->wifiName == hideWiFiConn) {
                    ocf->setHideItem(true, true);
                } else {
                    ocf->setSelected(false, false);
                }
            }

        }
        currSelNetName = "";
    } else {
        int selectY = 0;
        for (int i = 0;i < wifiList.size(); i ++) {
            OneConnForm *ocf = wifiList.at(i);
            if (ocf->wifiName == wifiName) {
                selectY = ocf->y(); //获取选中item的y坐标
                this->scrollAreaw->verticalScrollBar()->setValue(selectY);
                break;
            }
        }

        // 选中元素下面的所有元素下移 H_WIFI_ITEM_BIG_EXTEND
        for (int i = 0;i < wifiList.size(); i ++) {
            OneConnForm *ocf = wifiList.at(i);
            if (ocf->y() > selectY) {
                ocf->move(L_VERTICAL_LINE_TO_ITEM, ocf->y() + extendLength);
            }
        }

        for (int i = 0;i < wifiList.size(); i ++) {
            OneConnForm *ocf = wifiList.at(i);
            if (ocf->wifiName == wifiName) {
                if (ocf->wifiName == hideWiFiConn) {
                    ocf->setHideItem(true, true);
                } else {
                    ocf->setSelected(true, false);
                }
            } else {
                if (ocf->wifiName == hideWiFiConn) {
                    ocf->setHideItem(true, true);
                } else {
                    ocf->setSelected(false, false);
                }
            }
        }

        currSelNetName = wifiName;
    }

    //最后一个item没有下划线
    QList<OneConnForm *> itemList = wifiListWidget->findChildren<OneConnForm *>();
    int n = itemList.size();
    if (n >= 1) {
        OneConnForm *lastItem = itemList.at(n-1);
        lastItem->setLine(false);
    }

    //********************处理上方列表-界面所有控件回原位******************//
    // 顶部的item缩小
    topWifiListWidget->resize(W_TOP_LIST_WIDGET+194*isTabletStyle, H_NORMAL_ITEM + H_GAP_UP + X_ITEM);
    lbTopWifiList->move(X_MIDDLE_WORD, H_NORMAL_ITEM + H_GAP_UP);
//    btnAddNet->move(X_BTN_FUN, Y_BTN_FUN);
    scrollAreaw->move(W_LEFT_AREA, Y_SCROLL_AREA);
    lbNoItemTip->move(this->width()/2 - W_NO_ITEM_TIP/2 + W_LEFT_AREA/2, this->height()/2);

    OneConnForm *ocf = topWifiList.at(0);
    ocf->setTopItem(false);
}
void KylinNM::oneTopWifiFormSelected(QString wifiName, int extendLength)
{
    QList<OneConnForm *>topWifiList = topWifiListWidget->findChildren<OneConnForm *>();
    QList<OneConnForm *> wifiList = wifiListWidget->findChildren<OneConnForm *>();

    if (currSelNetName == wifiName) {
        // 与上一次选中同一个网络框，缩小当前选项卡
        topWifiListWidget->resize(W_TOP_LIST_WIDGET+194*isTabletStyle, H_NORMAL_ITEM + H_GAP_UP + X_ITEM);
        lbTopWifiList->move(X_MIDDLE_WORD, H_NORMAL_ITEM + H_GAP_UP);
//        btnAddNet->move(X_BTN_FUN, Y_BTN_FUN);
        scrollAreaw->move(W_LEFT_AREA, Y_SCROLL_AREA);
        lbNoItemTip->move(this->width()/2 - W_NO_ITEM_TIP/2 + W_LEFT_AREA/2, this->height()/2);

        OneConnForm *ocf = topWifiList.at(0);
        ocf->setTopItem(false);

        currSelNetName = "";
    } else {
        // 没有与上一次选中同一个网络框，放大当前选项卡

        for(int i = 0;i < wifiList.size(); i ++) {
            // 所有元素回到原位
            OneConnForm *ocf = wifiList.at(i);
            ocf->setSelected(false, false);
            ocf->move(L_VERTICAL_LINE_TO_ITEM, i * H_NORMAL_ITEM);
        }

        topWifiListWidget->resize(W_TOP_LIST_WIDGET+194*isTabletStyle, H_NORMAL_ITEM + H_WIFI_ITEM_BIG_EXTEND + H_GAP_UP + X_ITEM);
        lbTopWifiList->move(X_MIDDLE_WORD, H_NORMAL_ITEM + H_WIFI_ITEM_BIG_EXTEND + H_GAP_UP);
//        btnAddNet->move(X_BTN_FUN, Y_BTN_FUN + H_WIFI_ITEM_BIG_EXTEND);
        scrollAreaw->move(W_LEFT_AREA, Y_SCROLL_AREA + H_WIFI_ITEM_BIG_EXTEND);
        lbNoItemTip->move(this->width()/2 - W_NO_ITEM_TIP/2 + W_LEFT_AREA/2, this->height()/2 + 65);

        OneConnForm *ocf = topWifiList.at(0);
        ocf->setTopItem(true);

        currSelNetName = wifiName;
    }
}

//断开网络处理
void KylinNM::activeLanDisconn()
{
    syslog(LOG_DEBUG, "Wired net is disconnected");

    QString txt(tr("Wired net is disconnected"));
    objKyDBus->showDesktopNotify(txt);
    //QString cmd = "export LANG='en_US.UTF-8';export LANGUAGE='en_US';notify-send '" + txt + "...' -t 3800";
    //int status1 = system(cmd.toUtf8().data());
    //if (status1 != 0){ syslog(LOG_ERR, "execute 'notify-send' in function 'execConnWifiPWD' failed");}

    currSelNetName = "";
    //this->startLoading();
    emit this->waitLanStop();
    this->ksnm->execGetLanList();
    QTimer::singleShot(200, this, &KylinNM::onConnectChanged);
}

void KylinNM::activeWifiDisconn()
{
    QThread *tt = new QThread();
    BackThread *btt = new BackThread();
    btt->moveToThread(tt);
    connect(tt, SIGNAL(finished()), tt, SLOT(deleteLater()));
    connect(this, SIGNAL(disConnSparedNet(QString)), btt, SLOT(disConnSparedNetSlot(QString)),Qt::DirectConnection);
    connect(btt, SIGNAL(disFinish()), this, SLOT(activeGetWifiList()), Qt::DirectConnection);
    connect(btt, SIGNAL(ttFinish()), tt, SLOT(quit()),Qt::DirectConnection);

    tt->start();
    activeStartLoading();
    QTimer::singleShot(200, this, &KylinNM::onConnectChanged);
}

void KylinNM::reflashWifiUi()
{
    mutexIsReflashWifi.lock();
    m_is_reflashWifiUi = true;
    mutexIsReflashWifi.unlock();
}

void KylinNM::activeStartLoading()
{
    syslog(LOG_DEBUG, "Wi-Fi is disconnected");
    emit this->disConnSparedNet("wifi");
}
void KylinNM::activeGetWifiList()
{
    emit this->waitWifiStop();
    this->ksnm->execGetWifiList();
}

//网络开关处理，打开与关闭网络
void KylinNM::enNetDone()
{
    BackThread *bt = new BackThread();
    mwBandWidth = bt->execChkLanWidth(lname);

    ui->lbBtnNetBG->setStyleSheet(btnOnQss);

    // 打开网络开关时如果Wifi开关是打开的，设置其样式
    if (checkWlOn()) {
        btnWireless->setSwitchStatus(true);
        //ui->lbBtnWifiBG->setStyleSheet(btnBgOnQss);
        //ui->lbBtnWifiBall->move(X_RIGHT_WIFI_BALL, Y_WIFI_BALL);
    }

    onBtnNetListClicked(1);
    is_stop_check_net_state = 0;

    qDebug()<<"debug: already turn on the switch of lan network";
    syslog(LOG_DEBUG, "Already turn on the switch of lan network");
}
void KylinNM::disNetDone()
{
    this->is_btnNetList_clicked = 1;
    this->is_btnWifiList_clicked = 0;

    ui->lbNetListBG->setStyleSheet(btnOnQss);
    ui->lbWifiListBG->setStyleSheet(btnOffQss);

    ui->lbNetwork->setText("有线网络");
    btnWireless->hide();
    //ui->lbBtnWifiBG->hide();
    //ui->lbBtnWifiBall->hide();

    delete topLanListWidget; // 清空top列表
    createTopLanUI(); //创建顶部有线网item

    // 清空lan列表
    lanListWidget = new QWidget(scrollAreal);
    lanListWidget->resize(W_LIST_WIDGET, H_NORMAL_ITEM + H_LAN_ITEM_EXTEND);
    scrollAreal->setWidget(lanListWidget);
    scrollAreal->move(W_LEFT_AREA, Y_SCROLL_AREA);

    // 当前连接的lan
    OneLancForm *ccf = new OneLancForm(topLanListWidget, this, confForm, ksnm);
    ccf->setName(tr("Not connected"), "");//"当前未连接任何 以太网"
    ccf->setIcon(false);
    ccf->setConnedString(1, tr("Disconnected"));//"未连接"
    ccf->isConnected = false;
    ccf->setTopItem(false);
    ccf->setAct(true);
    ccf->move(L_VERTICAL_LINE_TO_ITEM, 0);
    ccf->show();

    ui->lbBtnNetBG->setStyleSheet(btnOffQss);

    btnWireless->setSwitchStatus(false);

    this->lanListWidget->show();
    this->wifiListWidget->hide();
    this->scrollAreal->show();
    this->topLanListWidget->show();
    this->scrollAreaw->hide();
    this->topWifiListWidget->hide();

    on_btnNetList_pressed();

    qDebug()<<"debug: already turn off the switch of lan network";
    syslog(LOG_DEBUG, "Already turn off the switch of lan network");

    this->stopLoading();
    QTimer::singleShot(200, this, &KylinNM::onConnectChanged);
}
void KylinNM::launchLanDone()
{
    ui->lbBtnNetBG->setStyleSheet(btnOnQss);
}

void KylinNM::enWifiDone()
{
    //ui->lbBtnWifiBG->setStyleSheet(btnBgOnQss);
    //ui->lbBtnWifiBall->move(X_RIGHT_WIFI_BALL, Y_WIFI_BALL);

    is_update_wifi_list = 0;
    if (is_btnWifiList_clicked) {
        this->ksnm->execGetWifiList();
    } else {
        on_btnWifiList_clicked();
    }

    qDebug()<<"debug: already turn on the switch of wifi network";
    syslog(LOG_DEBUG, "Already turn on the switch of wifi network");
}
void KylinNM::disWifiDone()
{
    disWifiDoneChangeUI();

    on_btnWifiList_pressed();

    qDebug()<<"debug: already turn off the switch of wifi network";
    syslog(LOG_DEBUG, "Already turn off the switch of wifi network");

    this->stopLoading();
    is_stop_check_net_state = 0;
    QTimer::singleShot(200, this, &KylinNM::onConnectChanged);
}
void KylinNM::disWifiStateKeep()
{
    if (this->is_btnNetList_clicked == 1) {
        btnWireless->setSwitchStatus(false);
        //ui->lbBtnWifiBG->setStyleSheet(btnBgOffQss);
        //ui->lbBtnWifiBall->move(X_LEFT_WIFI_BALL, Y_WIFI_BALL);
    }
    if (this->is_btnWifiList_clicked== 1) {
        disWifiDoneChangeUI();

        // this->stopLoading();
        getActiveInfo();
    }
}
void KylinNM::disWifiDoneChangeUI()
{
    wifiListWidget = new QWidget(scrollAreaw);
    wifiListWidget->resize(W_LIST_WIDGET+173*isTabletStyle, H_WIFI_ITEM_BIG_EXTEND);
    scrollAreaw->setWidget(wifiListWidget);
    scrollAreaw->move(W_LEFT_AREA, Y_SCROLL_AREA);

    lbTopWifiList->move(X_MIDDLE_WORD, H_NORMAL_ITEM + H_GAP_UP);
//    btnAddNet->move(X_BTN_FUN, Y_BTN_FUN);
    topWifiListWidget->resize(W_TOP_LIST_WIDGET+194*isTabletStyle, H_NORMAL_ITEM + H_GAP_UP + X_ITEM);

    QList<OneConnForm *> wifiList = topWifiListWidget->findChildren<OneConnForm *>();
    for (int i = 0; i < wifiList.size(); i ++) {
        OneConnForm *ocf = wifiList.at(i);
        if (ocf->isActive == true) {
            ocf->setSelected(false, false);
            ocf->setName(tr("Not connected"));//"当前未连接任何 Wifi"
            ocf->setSignal("0", "--");
            ocf->setConnedString(1, tr("Disconnected"), "");//"未连接"
            lbLoadDown->hide();
            lbLoadUp->hide();
            lbLoadDownImg->hide();
            lbLoadUpImg->hide();
            ocf->isConnected = false;
            ocf->setTopItem(false);
            disconnect(ocf, SIGNAL(selectedOneWifiForm(QString,int)), this, SLOT(oneTopWifiFormSelected(QString,int)));
        } else {
            ocf->deleteLater();
        }
    }

    btnWireless->setSwitchStatus(false);
    //ui->lbBtnWifiBG->setStyleSheet(btnBgOffQss);
    //ui->lbBtnWifiBall->move(X_LEFT_WIFI_BALL, Y_WIFI_BALL);

    this->lanListWidget->hide();
    this->topLanListWidget->hide();
    this->wifiListWidget->show();
    this->topWifiListWidget->show();
    this->scrollAreal->hide();
    this->scrollAreaw->show();
}

void KylinNM::on_btnHotspotState()
{
    ui->lbHotImg->setStyleSheet("QLabel{background-image:url(:/res/x/hot-spot-off.svg);}");
    ui->lbHotBG->setStyleSheet(btnOffQss);
    is_hot_sopt_on = 0;
}

//处理外界对网络的连接与断开
void KylinNM::onExternalConnectionChange(QString type)
{
    qDebug() <<QTime::currentTime() << "info:[KylinNM] [onExternalConnectionChange] type=" << type;
    QTimer::singleShot(200, this, &KylinNM::onConnectChanged);

    if (!is_stop_check_net_state) {
        is_stop_check_net_state = 1;
        if (type == "802-3-ethernet" || type == "ethernet") {
            QTimer::singleShot(2*1000, this, SLOT(onExternalLanChange() ));
        }

        if (type == "802-11-wireless" || type == "wifi") {
            QTimer::singleShot(4*1000, this, SLOT(onExternalWifiChange() ));
        }
    }
}

void KylinNM::onExternalLanChange()
{
    onBtnNetListClicked(0);
}

void KylinNM::onExternalWifiChange()
{
    if (is_connect_wifi_failed) {
        qDebug()<<"debug: connect wifi failed just now, no need to refresh wifi interface";
        is_connect_wifi_failed = 0;
    } else {
        on_btnWifiList_clicked();
    }
}

//处理外界对wifi的打开与关闭
void KylinNM::onExternalWifiSwitchChange(bool wifiEnabled)
{
    if (!is_stop_check_net_state) {
        is_stop_check_net_state = 1;
        if (wifiEnabled) {
            qDebug()<<"debug: external wifi switch turn on";
            syslog(LOG_DEBUG, "debug: external wifi switch turn on");
            QTimer::singleShot(4*1000, this, SLOT(onExternalWifiChange() ));
        } else {
            qDebug()<<"debug: external wifi switch turn off";
            syslog(LOG_DEBUG, "debug: external wifi switch turn off");
            QTimer::singleShot(3*1000, this, SLOT(onExternalWifiChange() ));
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//循环处理部分，目前仅on_checkWifiListChanged 与on_setNetSpeed两个函数在运行

void KylinNM::on_checkWifiListChanged()
{
    if (is_stop_check_net_state==0 && this->is_btnWifiList_clicked==1 && this->isVisible()) {
        BackThread *loop_bt = new BackThread();
        IFace *loop_iface = loop_bt->execGetIface();

        if (loop_iface->wstate != 2) {
            is_update_wifi_list = 1;
            this->ksnm->execGetWifiList(); //更新wifi列表
        }

        delete loop_iface;
        loop_bt->deleteLater();
    }
}

void KylinNM::on_setNetSpeed()
{
    if (this->isVisible() && is_stop_check_net_state==0) {
        if (is_btnWifiList_clicked == 1) {
            if ( objNetSpeed->getCurrentDownloadRates(objKyDBus->dbusWiFiCardName.toUtf8().data(), &start_rcv_rates, &start_tx_rates) == -1) {
                start_rcv_rates = end_rcv_rates;
            }
        } else if(is_btnNetList_clicked == 1) {
            if ( objNetSpeed->getCurrentDownloadRates(objKyDBus->dbusLanCardName.toUtf8().data(), &start_rcv_rates, &start_tx_rates) == -1) {
                start_tx_rates = end_tx_rates;
            }
        }

        long int delta_rcv = (start_rcv_rates - end_rcv_rates)/800;
        long int delta_tx = (start_tx_rates - end_tx_rates)/8;
        if (delta_rcv>=10000 || delta_rcv<0){delta_rcv = 0;}
        if (delta_tx>=10000 || delta_tx<0){delta_tx = 0;}

        int rcv_num = delta_rcv/3;
        int tx_num = delta_tx/3;

        QString str_rcv;
        QString str_tx;

        if (rcv_num < 1000) {
            str_rcv = QString::number(rcv_num) + "KB/s.";
        } else {
            int remainder;
            if (rcv_num%1000 < 100) {
                remainder = 0;
            } else {
                remainder = (rcv_num%1000)/100;
            }
            str_rcv = QString::number(rcv_num/1000) + "."  + QString::number(remainder) + "MB/s.";
        }

        if (tx_num < 1000) {
            str_tx = QString::number(tx_num) + "KB/s";
        } else {
            int remainder;
            if (tx_num%1000 < 100) {
                remainder = 0;
            } else {
                remainder = (tx_num%1000)/100;
            }
            str_tx = QString::number(tx_num/1000) + "."  + QString::number(remainder) + "MB/s";
        }

        lbLoadDown->setText(str_rcv);
        lbLoadUp->setText(str_tx);

        switch (str_rcv.size()) {
        case 6:
            lbLoadUp->move(X_ITEM + 187, Y_TOP_ITEM + 32);
            lbLoadUpImg->move(X_ITEM + 170, Y_TOP_ITEM + 35);
            break;
        case 7:
            lbLoadUp->move(X_ITEM + 194, Y_TOP_ITEM + 32);
            lbLoadUpImg->move(X_ITEM + 176, Y_TOP_ITEM + 35);
            break;
        case 8:
            lbLoadUp->move(X_ITEM + 199, Y_TOP_ITEM + 32);
            lbLoadUpImg->move(X_ITEM + 186, Y_TOP_ITEM + 35);
            break;
        default:
            break;
        }

        end_rcv_rates = start_rcv_rates;
        end_tx_rates = start_tx_rates;
    }
}

void KylinNM::connLanDone(int connFlag)
{
    emit this->waitLanStop(); //停止加载动画

    // Lan连接结果，0点击连接成功 1失败 3开机启动网络工具时已经连接
    if (connFlag == 0) {
        syslog(LOG_DEBUG, "Wired net already connected by clicking button");
        this->is_wired_line_ready = 1;
        this->is_by_click_connect = 1;
        this->ksnm->execGetLanList();

        QString txt(tr("Conn Ethernet Success"));
        objKyDBus->showDesktopNotify(txt);
        //QString cmd = "export LANG='en_US.UTF-8';export LANGUAGE='en_US';notify-send '" + txt + "' -t 3800";
        //int status = system(cmd.toUtf8().data());
        //if (status != 0){ syslog(LOG_ERR, "execute 'notify-send' in function 'connLanDone' failed");}
    }

    if (connFlag == 1) {
        qDebug()<<"without net line connect to computer";
        this->is_wired_line_ready = 0; //without net line connect to computer
        is_stop_check_net_state = 0;

        QString txt(tr("Conn Ethernet Fail"));
        objKyDBus->showDesktopNotify(txt);
        //QString cmd = "export LANG='en_US.UTF-8';export LANGUAGE='en_US';notify-send '" + txt + "' -t 3800";
        //int status = system(cmd.toUtf8().data());
        //if (status != 0){ syslog(LOG_ERR, "execute 'notify-send' in function 'connLanDone' failed");}
    }

    if (connFlag == 3) {
        syslog(LOG_DEBUG, "Launch kylin-nm, Lan already connected");
        this->is_wired_line_ready = 1;
    }

    this->stopLoading();
    QTimer::singleShot(200, this, &KylinNM::onConnectChanged);
}

void KylinNM::connWifiDone(int connFlag)
{
    emit this->waitWifiStop(); //停止加载动画
    // Wifi连接结果，0点击连接成功 1失败 2没有配置文件 3开机启动网络工具时已经连接
    if (connFlag == 0) {
        syslog(LOG_DEBUG, "Wi-Fi already connected by clicking button");
        this->is_by_click_connect = 1;
        this->ksnm->execGetWifiList();

        QString txt(tr("Conn Wifi Success"));
        objKyDBus->showDesktopNotify(txt);
        //QString cmd = "export LANG='en_US.UTF-8';export LANGUAGE='en_US';notify-send '" + txt + "' -t 3800";
        //int status = system(cmd.toUtf8().data());
        //if (status != 0){ syslog(LOG_ERR, "execute 'notify-send' in function 'connWifiDone' failed");}
    } else if (connFlag == 1) {
        is_stop_check_net_state = 0;
        is_connect_wifi_failed = 1;

        QString txt(tr("Confirm your Wi-Fi password or usable of wireless card"));
        objKyDBus->showDesktopNotify(txt);
        //QString cmd = "export LANG='en_US.UTF-8';export LANGUAGE='en_US';notify-send '" + txt + "...' -t 3800";
        //int status1 = system(cmd.toUtf8().data());
        //if (status1 != 0){ syslog(LOG_ERR, "execute 'notify-send' in function 'execConnWifiPWD' failed");}
    } else if (connFlag == 3) {
        syslog(LOG_DEBUG, "Launch kylin-nm, Wi-Fi already connected");
    }
    QTimer::singleShot(200, this, &KylinNM::onConnectChanged);
}

int KylinNM::getConnectStatus()
{
    int ret = -1;
    QString actLanName = "--";
    QString actWifiName = "--";

    activecon *act = kylin_network_get_activecon_info();
    int index = 0;
    while (act[index].con_name != NULL) {
        if (QString(act[index].type) == "ethernet" || QString(act[index].type) == "802-3-ethernet") {
            actLanName = QString(act[index].con_name);
        }
        if (QString(act[index].type) == "wifi" || QString(act[index].type) == "802-11-wireless") {
            actWifiName = QString(act[index].con_name);
        }
        index ++;
    }

    //ukui3.0中获取currentActWifiSignalLv的值
    if (activeWifiSignalLv > 75) {
        currentActWifiSignalLv = 1;
    } else if(activeWifiSignalLv > 55 && activeWifiSignalLv <= 75) {
        currentActWifiSignalLv = 2;
    } else if(activeWifiSignalLv > 35 && activeWifiSignalLv <= 55) {
        currentActWifiSignalLv = 3;
    } else if( activeWifiSignalLv <= 35) {
        currentActWifiSignalLv = 4;
    }

    // 设置图标
    if (actLanName != "--") {
        ret = 0;
    } else if (actWifiName != "--") {
        ret = 1;
    } else {
        ret = -1;
    }

    return ret;
}
