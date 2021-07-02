/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NotificationWindow.h"
#include "EMStudioManager.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>


namespace EMStudio
{
    // constructor
    NotificationWindow::NotificationWindow(QWidget* parent, EType type, const QString& Message)
        : QWidget(parent)
    {
        // set the opacity
        mOpacity = 210;

        // set the window title
        setWindowTitle("Notification");

        // window, no border, no focus, stays on top
        setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);

        // enable the translucent background
        setAttribute(Qt::WA_TranslucentBackground);

        // enable the show without activating to avoid focus when show
        setAttribute(Qt::WA_ShowWithoutActivating);

        // set the fixed width
        setFixedWidth(300);

        // create the icon
        mIcon = new QToolButton();
        mIcon->setObjectName("NotificationIcon");
        mIcon->setStyleSheet("#NotificationIcon{ background-color: transparent; border: none; }");
        mIcon->setIconSize(QSize(22, 22));
        mIcon->setFocusPolicy(Qt::NoFocus);
        if (type == TYPE_ERROR)
        {
            mIcon->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/ExclamationMark.svg"));
        }
        else if (type == TYPE_WARNING)
        {
            mIcon->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Warning.svg"));
        }
        else if (type == TYPE_SUCCESS)
        {
            mIcon->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Confirm.svg"));
        }
        connect(mIcon, &QToolButton::pressed, this, &NotificationWindow::IconPressed);

        // create the message label
        mMessageLabel = new QLabel(Message);
        mMessageLabel->setWordWrap(true);

        // create the layout
        QHBoxLayout* layout = new QHBoxLayout();
        layout->addWidget(mIcon);
        layout->addWidget(mMessageLabel);

        // set the layout
        setLayout(layout);

        // start the timer
        mTimer = new QTimer(this);
        mTimer->setSingleShot(true);
        connect(mTimer, &QTimer::timeout, this, &NotificationWindow::TimerTimeOut);
        mTimer->start(GetNotificationWindowManager()->GetVisibleTime() * 1000);
    }

    NotificationWindow::~NotificationWindow()
    {
        // remove from the notification window manager
        GetManager()->GetNotificationWindowManager()->RemoveNotificationWindow(this);
    }


    // paint event
    void NotificationWindow::paintEvent(QPaintEvent* event)
    {
        MCORE_UNUSED(event);
        QPainter p(this);
        p.setPen(Qt::transparent);
        p.setBrush(QColor(0, 0, 0, mOpacity));
        p.setRenderHint(QPainter::Antialiasing);
        p.drawRoundedRect(rect(), 10, 10);
    }


    // resize event
    void NotificationWindow::resizeEvent(QResizeEvent* event)
    {
        MCORE_UNUSED(event);
        setMask(rect());
    }


    // mouse press event
    void NotificationWindow::mousePressEvent(QMouseEvent* event)
    {
        // we only want the left button, stop here if the timer is not active too
        if ((mTimer->isActive() == false) || (event->button() != Qt::LeftButton))
        {
            return;
        }

        // stop the timer because the event will be called before
        mTimer->stop();

        // call the timer time out function
        TimerTimeOut();
    }


    // icon pressed
    void NotificationWindow::IconPressed()
    {
        // stop here if the timer is not active
        if (mTimer->isActive() == false)
        {
            return;
        }

        // stop the timer because the event will be called before
        mTimer->stop();

        // call the timer time out function
        TimerTimeOut();
    }


    // timer time out
    void NotificationWindow::TimerTimeOut()
    {
        // create the opacity effect and set it on the icon
        QGraphicsOpacityEffect* iconOpacityEffect = new QGraphicsOpacityEffect(this);
        mIcon->setGraphicsEffect(iconOpacityEffect);

        // create the property animation to control the property value
        QPropertyAnimation* iconPropertyAnimation = new QPropertyAnimation(iconOpacityEffect, "opacity");
        iconPropertyAnimation->setDuration(500);
        iconPropertyAnimation->setStartValue((double)mOpacity / 255.0);
        iconPropertyAnimation->setEndValue(0.0);
        iconPropertyAnimation->setEasingCurve(QEasingCurve::Linear);
        iconPropertyAnimation->start(QPropertyAnimation::DeleteWhenStopped);

        // create the opacity effect and set it on the label
        QGraphicsOpacityEffect* labelOpacityEffect = new QGraphicsOpacityEffect(this);
        mMessageLabel->setGraphicsEffect(labelOpacityEffect);

        // create the property animation to control the property value
        QPropertyAnimation* labelPropertyAnimation = new QPropertyAnimation(labelOpacityEffect, "opacity");
        labelPropertyAnimation->setDuration(500);
        labelPropertyAnimation->setStartValue((double)mOpacity / 255.0);
        labelPropertyAnimation->setEndValue(0.0);
        labelPropertyAnimation->setEasingCurve(QEasingCurve::Linear);
        labelPropertyAnimation->start(QPropertyAnimation::DeleteWhenStopped);

        // connect the animation, the two animations are the same so only connect for the text is enough
        connect(labelOpacityEffect, &QGraphicsOpacityEffect::opacityChanged, this, &NotificationWindow::OpacityChanged);
        connect(labelPropertyAnimation, &QPropertyAnimation::finished, this, &NotificationWindow::FadeOutFinished);
    }


    // opacity changed
    void NotificationWindow::OpacityChanged(qreal opacity)
    {
        // set the new opacity for the paint of the window
        mOpacity = aznumeric_cast<int>(opacity * 255);

        // update the window
        update();
    }


    // fade out finished
    void NotificationWindow::FadeOutFinished()
    {
        // hide the notification window
        hide();

        // remove from the notification window manager
        GetManager()->GetNotificationWindowManager()->RemoveNotificationWindow(this);

        // delete later
        deleteLater();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_NotificationWindow.cpp>
