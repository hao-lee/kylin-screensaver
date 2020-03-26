/* iconedit.cpp
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
#include "iconedit.h"
#include <QHBoxLayout>
#include <QWidget>
#include <QPainter>
#include <QFile>
#include <QKeyEvent>
#include <QDebug>
#include <QTimer>
#include <QtX11Extras/QX11Info>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>


/**
 * @brief 判断大写键状态
 * @return true: 大写锁定
 */
bool checkCapsLockState()
{
    Display *display = XOpenDisplay(NULL);
    bool capsState = false;
    if(display) {
        unsigned int n;
        XkbGetIndicatorState(display, XkbUseCoreKbd, &n);
        capsState = (n & 0x01) == 1;
    }
    return capsState;
}


IconEdit::IconEdit(QWidget *parent)
    : QWidget(parent),
      m_timer(nullptr)
{
    m_edit = new QLineEdit(this);
    m_edit->setObjectName(QStringLiteral("passwdEdit"));
    m_edit->setAttribute(Qt::WA_InputMethodEnabled, false); //禁用输入法
    m_edit->setContextMenuPolicy(Qt::NoContextMenu);    //禁用右键菜单
    m_edit->installEventFilter(this);

    m_capsIcon = new QLabel(this);
    m_capsIcon->setObjectName(QStringLiteral("capsIconLabel"));
    m_capsIcon->setVisible(checkCapsLockState());

    m_iconButton = new QPushButton(this);
    m_iconButton->setObjectName(QStringLiteral("loginButton"));
    m_iconButton->setFocusPolicy(Qt::NoFocus);
    m_iconButton->setCursor(QCursor(Qt::PointingHandCursor));

    m_modeButton = new QPushButton(this);
    m_modeButton->setObjectName(QStringLiteral("echoModeButton"));
    m_modeButton->setCheckable(true);
    m_modeButton->setFocusPolicy(Qt::NoFocus);
    m_modeButton->setCursor(Qt::PointingHandCursor);
    connect(m_modeButton, &QPushButton::clicked, this, [&](bool checked){
        setType(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(m_capsIcon);
    layout->addWidget(m_modeButton);
    layout->addWidget(m_iconButton);

    connect(m_edit, &QLineEdit::returnPressed, this, &IconEdit::clicked_cb);
    connect(m_iconButton, &QPushButton::clicked, this, &IconEdit::clicked_cb);

    setFocusProxy(m_edit);
}

void IconEdit::setType(QLineEdit::EchoMode type)
{
    m_edit->setEchoMode(type);
    if(type == 0)
       m_modeButton->setChecked(true);
    else
       m_modeButton->setChecked(false);
}


void IconEdit::resizeEvent(QResizeEvent *)
{
    // 设置输入框中文件输入区，不让输入的文字在被隐藏在按钮下
    m_edit->setTextMargins(1, 1, m_iconButton->width() + m_modeButton->width(), 1);
    m_edit->setFixedSize(size());
}

bool IconEdit::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == m_edit){
        if(event->type() == 6){
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if(keyEvent->key()==80 &&keyEvent->modifiers() ==(Qt::MetaModifier)){
                event->ignore();
                return true;
            }
        }
        if(event->type() == 23)
        {
            XSetInputFocus(QX11Info::display(),this->winId(),RevertToParent,CurrentTime);
        }
    }
    return false;
}

void IconEdit::clicked_cb()
{
    startWaiting();
    emit clicked(m_edit->text());
}

void IconEdit::onCapsStateChanged()
{
    bool capsState = m_capsIcon->isHidden();
    m_capsIcon->setVisible(capsState);
    int w = m_iconButton->width() + m_modeButton->width();
    m_edit->setTextMargins(1, 1, capsState ? w + m_capsIcon->width() : w, 1);
}

void IconEdit::setIcon(const QString &text)
{
    m_iconButton->setIcon(QIcon());
    m_iconButton->setText(text);
    m_iconText = text;
    m_icon = QIcon();
}

void IconEdit::setIcon(const QIcon &icon)
{
    m_iconButton->setIcon(icon);
    m_iconButton->setText("");
    m_icon = icon;
    m_iconText = "";
}

void IconEdit::clear()
{
    m_edit->setText("");
    setPrompt("");
}

void IconEdit::setPrompt(const QString &prompt)
{
    m_edit->setPlaceholderText(prompt);
}

const QString IconEdit::text()
{
    return m_edit->text();
}

void IconEdit::startWaiting()
{
    m_edit->setReadOnly(true);
    m_iconButton->setEnabled(false);


    if(!m_timer)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(120);
        connect(m_timer, &QTimer::timeout, this, &IconEdit::updatePixmap);
    }
    m_waitingPixmap.load(":/image/assets/waiting.png");
    m_iconButton->setIconSize(m_iconButton->size());
    m_iconButton->setIcon(QIcon(m_waitingPixmap));
    m_timer->start();
}


void IconEdit::stopWaiting()
{
    if(m_timer && m_timer->isActive())
    {
        m_timer->stop();
    }

    m_edit->setReadOnly(false);
    m_iconButton->setEnabled(true);
    if(!m_icon.isNull())
        m_iconButton->setIcon(m_icon);
    else
        m_iconButton->setText(m_iconText);
}

void IconEdit::updatePixmap()
{
    QMatrix matrix;
    matrix.rotate(90.0);
    m_waitingPixmap = m_waitingPixmap.transformed(matrix, Qt::FastTransformation);
    m_iconButton->setIcon(QIcon(m_waitingPixmap));
}

void IconEdit::setEnabled(bool enabled)
{
    m_edit->setAttribute(Qt::WA_InputMethodEnabled, false);
    m_edit->setEnabled(enabled);
    m_iconButton->setEnabled(enabled);
}
