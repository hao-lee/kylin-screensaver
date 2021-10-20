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


#include "ui_dlgconnhidwifileap.h"
#include "kylinheadfile.h"

#include <sys/syslog.h>

DlgConnHidWifiLeap::DlgConnHidWifiLeap(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgConnHidWifiLeap)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    //需要添加 void paintEvent(QPaintEvent *event) 函数

    QPainterPath path;
    auto rect = this->rect();
    rect.adjust(1, 1, -1, -1);
    path.addRoundedRect(rect, 6, 6);
    setProperty("blurRegion", QRegion(path.toFillPolygon().toPolygon()));

    this->setStyleSheet("QWidget{border-radius:6px;background-color:rgba(19,19,20,0.7);border:1px solid rgba(255, 255, 255, 0.05);}");

    MyQss objQss;

    ui->lbBoder->setStyleSheet("QLabel{border-radius:6px;background-color:rgba(19,19,20,0.95);border:1px solid rgba(255, 255, 255, 0.05);}");
    ui->lbBoder->hide();
    ui->lbLeftupTitle->setStyleSheet("QLabel{border:0px;font-size:20px;color:rgba(255,255,255,0.97);background-color:transparent;}");
    ui->lbConn->setStyleSheet(objQss.labelQss);
    ui->lbNetName->setStyleSheet(objQss.labelQss);
    ui->lbSecurity->setStyleSheet(objQss.labelQss);
    ui->lbUserName->setStyleSheet(objQss.labelQss);
    ui->lbPassword->setStyleSheet(objQss.labelQss);

    ui->cbxConn->setStyleSheet(objQss.cbxQss);
    ui->cbxConn->setView(new  QListView());
    ui->leNetName->setStyleSheet(objQss.leQss);
    ui->leUserName->setStyleSheet(objQss.leQss);
    ui->lePassword->setStyleSheet(objQss.leQss);
    ui->cbxSecurity->setStyleSheet(objQss.cbxQss);
    ui->cbxSecurity->setView(new  QListView());
    ui->checkBoxPwd->setStyleSheet(objQss.checkBoxQss);

    ui->btnCancel->setStyleSheet(objQss.btnCancelQss);
    ui->btnConnect->setStyleSheet(objQss.btnConnQss);
    ui->lineUp->setStyleSheet(objQss.lineQss);
    ui->lineDown->setStyleSheet(objQss.lineQss);

    ui->lbLeftupTitle->setText(tr("Add hidden Wi-Fi")); //加入隐藏Wi-Fi
    ui->lbConn->setText(tr("Connection")); //连接设置:
    ui->lbNetName->setText(tr("Network name")); //网络名称:
    ui->lbSecurity->setText(tr("Wi-Fi security")); //Wi-Fi安全性:
    ui->lbUserName->setText(tr("Username")); //用户名:
    ui->lbPassword->setText(tr("Password")); //密码:
    ui->btnCancel->setText(tr("Cancel")); //取消
    ui->btnConnect->setText(tr("Connect")); //连接

    ui->cbxConn->addItem(tr("C_reate…")); //新建...
    int status = system("nmcli connection show>/tmp/kylin-nm-connshow");
    if (status != 0){ syslog(LOG_ERR, "execute 'nmcli connection show' in function 'DlgConnHidWifiLeap' failed");}
    QFile file("/tmp/kylin-nm-connshow");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"Can't open the file!";
    }
    QString txt = file.readAll();
    QStringList txtLine = txt.split("\n");
    file.close();
    foreach (QString line, txtLine) {
        if(line.indexOf("wifi") != -1){
            QStringList subLine = line.split(" ");
            ui->cbxConn->addItem(subLine[0]);
        }
    }
    ui->cbxConn->setCurrentIndex(0);

    ui->cbxSecurity->addItem(tr("None")); //无
    ui->cbxSecurity->addItem(tr("WPA & WPA2 Personal")); //WPA 及 WPA2 个人
    ui->cbxSecurity->addItem(tr("WEP 40/128-bit Key (Hex or ASCII)")); //WEP 40/128 位密钥(十六进制或ASCII)
    ui->cbxSecurity->addItem(tr("WEP 128-bit Passphrase")); //WEP 128 位密码句
    ui->cbxSecurity->addItem("LEAP");
    ui->cbxSecurity->addItem(tr("Dynamic WEP (802.1X)")); //动态 WEP (802.1x)
    ui->cbxSecurity->addItem(tr("WPA & WPA2 Enterprise")); //WPA 及 WPA2 企业
    ui->cbxSecurity->setCurrentIndex(4);
    connect(ui->cbxSecurity,SIGNAL(currentIndexChanged(QString)),this,SLOT(changeDialog()));

    ui->btnConnect->setEnabled(false);

    this->setFixedSize(432,434);

    /**解决锁屏设置 X11BypassWindowManagerHint 属性导致QCombox弹框异常的问题---START----
    ** 手动绑定下拉框视图和下拉框
    **/
    ui->cbxConn->view()->setWindowFlags(Qt::Popup | Qt::X11BypassWindowManagerHint);
    ui->cbxConn->view()->setParent(this);
    ui->cbxConn->view()->hide();
    ui->cbxConn->installEventFilter(this);

    ui->cbxSecurity->view()->setWindowFlags(Qt::Popup | Qt::X11BypassWindowManagerHint);
    ui->cbxSecurity->view()->setParent(this);
    ui->cbxSecurity->view()->hide();
    ui->cbxSecurity->installEventFilter(this);

    connect(ui->cbxConn->view(), &QAbstractItemView::pressed, this, [=](QModelIndex index){
       Q_EMIT ui->cbxConn->setCurrentIndex(index.row());
       ui->cbxConn->view()->hide();
    });

    connect(ui->cbxSecurity->view(), &QAbstractItemView::pressed, this, [=](QModelIndex index){
       ui->cbxSecurity->view()->hide();
       Q_EMIT ui->cbxSecurity->setCurrentIndex(index.row());
    });

    ui->cbxConn->view()->setGeometry(QRect(ui->cbxConn->geometry().left(), ui->cbxConn->geometry().bottom(), ui->cbxConn->view()->width(), ui->cbxConn->view()->height()));
    ui->cbxSecurity->view()->setGeometry(QRect(ui->cbxSecurity->geometry().left(), ui->cbxSecurity->geometry().bottom(), ui->cbxSecurity->view()->width(), ui->cbxSecurity->view()->height()));
    /**解决锁屏设置 X11BypassWindowManagerHint 属性导致QCombox弹框异常的问题---END----
    ** 手动绑定下拉框视图和下拉框
    **/
}

DlgConnHidWifiLeap::~DlgConnHidWifiLeap()
{
    delete ui;
}

/**解决锁屏设置 X11BypassWindowManagerHint 属性导致QCombox弹框异常的问题---START----
** 手动绑定下拉框视图和下拉框
**/
bool DlgConnHidWifiLeap::eventFilter(QObject *obj, QEvent *ev)
{
    if(ev->type() == QEvent::MouseButtonPress)
    {
        if(obj == ui->cbxConn)
        {
            ui->cbxConn->view()->setVisible(!ui->cbxConn->view()->isVisible());
            if(ui->cbxConn->view()->isVisible())
                ui->cbxConn->view()->setFocus();
        } else if (obj == ui->cbxSecurity)
        {
            ui->cbxSecurity->view()->setVisible(!ui->cbxSecurity->view()->isVisible());
            if(ui->cbxSecurity->view()->isVisible())
                ui->cbxSecurity->view()->setFocus();
        }
    }
    return false;
}
/**解决锁屏设置 X11BypassWindowManagerHint 属性导致QCombox弹框异常的问题---END----
** 手动绑定下拉框视图和下拉框
**/

void DlgConnHidWifiLeap::mousePressEvent(QMouseEvent *event){
    if(event->button() == Qt::LeftButton){
        this->isPress = true;
        this->winPos = this->pos();
        this->dragPos = event->globalPos();
        event->accept();
    }
}
void DlgConnHidWifiLeap::mouseReleaseEvent(QMouseEvent *event){
    this->isPress = false;
}
void DlgConnHidWifiLeap::mouseMoveEvent(QMouseEvent *event){
    if(this->isPress){
        this->move(this->winPos - (this->dragPos - event->globalPos()));
        event->accept();
    }
}

//切换到其他Wi-Fi安全类型
void DlgConnHidWifiLeap::changeDialog()
{
    if(ui->cbxSecurity->currentIndex()==0){
//        QApplication::setQuitOnLastWindowClosed(false);
        this->hide();
        DlgConnHidWifi *connHidWifi = new DlgConnHidWifi(0, 0,this->parentWidget());
        connHidWifi->show();
    } else if(ui->cbxSecurity->currentIndex()==1) {
//        QApplication::setQuitOnLastWindowClosed(false);
        this->hide();
        DlgConnHidWifiWpa *connHidWifiWpa = new DlgConnHidWifiWpa(0, 0, this->parentWidget());
        connHidWifiWpa->show();
    } else if(ui->cbxSecurity->currentIndex()==2) {
//        QApplication::setQuitOnLastWindowClosed(false);
        this->hide();
        DlgConnHidWifiWep *connHidWifiWep = new DlgConnHidWifiWep(0, this->parentWidget());
        connHidWifiWep->show();
    } else if(ui->cbxSecurity->currentIndex()==3) {
//        QApplication::setQuitOnLastWindowClosed(false);
        this->hide();
        DlgConnHidWifiWep *connHidWifiWep = new DlgConnHidWifiWep(1, this->parentWidget());
        connHidWifiWep->show();
    } else if(ui->cbxSecurity->currentIndex()==4) {
        qDebug()<<"it's not need to change dialog";
    } else if(ui->cbxSecurity->currentIndex()==5) {
//        QApplication::setQuitOnLastWindowClosed(false);
        this->hide();
        DlgConnHidWifiSecTls *connHidWifiSecTls = new DlgConnHidWifiSecTls(0, this->parentWidget());
        connHidWifiSecTls->show();
    } else {
//        QApplication::setQuitOnLastWindowClosed(false);
        this->hide();
        DlgConnHidWifiSecTls *connHidWifiSecTls = new DlgConnHidWifiSecTls(1, this->parentWidget());
        connHidWifiSecTls->show();
    }
}

void DlgConnHidWifiLeap::on_btnCancel_clicked()
{
    this->close();
}

void DlgConnHidWifiLeap::on_btnConnect_clicked()
{
    this->close();
}

void DlgConnHidWifiLeap::on_checkBoxPwd_stateChanged(int arg1)
{
    if (arg1 == 0) {
        ui->lePassword ->setEchoMode(QLineEdit::Password);
    } else {
        ui->lePassword->setEchoMode(QLineEdit::Normal);
    }
}

void DlgConnHidWifiLeap::on_leNetName_textEdited(const QString &arg1)
{
    if (ui->leNetName->text() == "" || ui->lePassword->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else if (ui->leNetName->text() == "" || ui->leUserName->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else if (ui->lePassword->text() == "" || ui->leUserName->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else {
        ui->btnConnect->setEnabled(true);
    }
}

void DlgConnHidWifiLeap::on_leUserName_textEdited(const QString &arg1)
{
    if (ui->leNetName->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else if (ui->leUserName->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else if (ui->lePassword->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else {
        ui->btnConnect->setEnabled(true);
    }
}

void DlgConnHidWifiLeap::on_lePassword_textEdited(const QString &arg1)
{
    if (ui->leNetName->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else if (ui->leUserName->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else if (ui->lePassword->text() == ""){
        ui->btnConnect->setEnabled(false);
    } else {
        ui->btnConnect->setEnabled(true);
    }
}

void DlgConnHidWifiLeap::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
       opt.init(this);
       QPainter p(this);
       style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
       QWidget::paintEvent(event);
}
