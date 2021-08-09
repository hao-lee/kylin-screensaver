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

#ifndef KYLINDBUSINTERFACE_H
#define KYLINDBUSINTERFACE_H

#include <sys/syslog.h>

#include <QObject>
#include <QDebug>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusObjectPath>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QVariant>
#include <QVariantMap>
#include <QGSettings/QGSettings>
#include <QTimer>
#include <QThread>

class KylinNM;
class Utils;

class KylinDBus : public QObject
{
    Q_OBJECT
public:
    explicit KylinDBus(KylinNM *mw = 0, QObject *parent = nullptr);
    ~KylinDBus();

    void getObjectPath();
    int getAccessPointsNumber();
    void showDesktopNotify(QString message);
    void initConnectionInfo();
    void connectWiredNet(QString netName);
    void getConnectNetIp();

    int getTaskBarPos(QString str);
    int getTaskBarHeight(QString str);
    void initTaskbarGsetting();
    int getTaskbarHeight();
    int getTaskbarPos();

    void getWifiSwitchState();
    bool getSwitchStatus(QString key);
    void setWifiSwitchState(bool signal);
    void setWifiCardState(bool signal);

    void initTransparentState();
    double getTransparentData();

    void initBtnWifiGsetting();
    void setBtnWifiGsetting(bool bl);
    QString getWifiSsid(QString accessPointPath);
    QList<QString> getAtiveWifiBSsidUuid(QStringList wifilist);

    QDBusObjectPath wiredPath; //有线设备的路径
    QDBusObjectPath wirelessPath; //无线设备的路径
    QList<QDBusObjectPath> multiWiredPaths; //已连接网络的对象路径列表

    QList<QString> multiWiredCableState;
    bool isWiredCableOn = false; //是否插入了网线
    bool isWirelessCardOn = false; //是否插入了无线网卡

    QString dbusLanCardName = "";
    QString dbusLanIpv4 = "";
    QString dbusLanIpv6 = "";
    QString dbusActiveLanIpv4 = "";
    QString dbusActiveLanIpv6 = "";
    QString dbusLanGateway = "";
    QString dbusLanMac = "";
    QString dbusWiFiCardName = "";
    QString dbusWifiMac = "";

public slots:
    void onNewConnection(QDBusObjectPath objPath);
    void onConnectionRemoved(QDBusObjectPath objPath);
    void onPropertiesChanged(QVariantMap qvm);
    void onLanPropertyChanged(QVariantMap qvm);
    void onWifiPropertyChanged(QVariantMap qvm);
    void onAccessPointAdded(QDBusObjectPath objPath);
    void getPhysicalCarrierState(int n);
    void getLanHwAddressState();
    void getWiredCardName();
    void getWirelessCardName();
    void getLanIp(QString netName);
    void getWifiMac(QString netName);
    void slot_timeout();
    void setWifiSignal(int wifiSignal, QString wifiname);
    void setWifiConnStatus(bool is_wifi_connected);
    void setLanConnStatus(bool is_lan_connected);

private:
    KylinNM *mw;
    Utils *mUtils;
    QThread *mUtilsThread;

    int a = 0;
    bool isRunningFunction = false;
    QTimer *time = nullptr;
    QList<QDBusObjectPath> oldPaths; //已连接网络的对象路径列表
    QStringList oldPathInfo; //某个已连接网络对象路径对应的网络类型(ethernet or wifi)
    bool oldWifiSwitchState; //上一次获取到的wifi开关状态

    QGSettings *m_tastbar_gsettings = nullptr;
    QGSettings *m_nm_gsettings = nullptr;
    QGSettings *m_gsettings = nullptr;
    QGSettings *m_transparency_gsettings = nullptr;
    QGSettings *m_btnWifi_gsettings = nullptr;

    bool is_wificonnected = false;

signals:
    void updateWiredList(int n);
    void requestSendDesktopNotify(QString message);
};

#endif // KYLINDBUSINTERFACE_H
