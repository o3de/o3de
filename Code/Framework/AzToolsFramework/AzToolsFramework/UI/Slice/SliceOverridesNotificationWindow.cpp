/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/UI/Slice/ui_NotificationWindow.h>
AZ_POP_DISABLE_WARNING
#include "UI/Slice/Constants.h"

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/UI/Slice/SliceOverridesNotificationWindow.hxx>
AZ_POP_DISABLE_WARNING
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                                    // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
                                                                    // 4800: 'QFlags<QPainter::RenderHint>::Int': forcing value to bool 'true' or 'false' (performance warning)
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
AZ_POP_DISABLE_WARNING
#include <QTimer>


namespace AzToolsFramework
{
    // constructor
    SliceOverridesNotificationWindow::SliceOverridesNotificationWindow(QWidget* parent, EType type, const QString& Message)
        : QWidget(parent)
        , m_ui(new Ui::NotificationWindow())
    {
        m_ui->setupUi(this);

        // window, no border, no focus, stays on top
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);

        // enable the translucent background
        setAttribute(Qt::WA_TranslucentBackground);

        // enable the show without activating to avoid focus when show
        setAttribute(Qt::WA_ShowWithoutActivating);

        m_icon = m_ui->toolButton;
        m_icon->setStyleSheet("{ background-color: transparent; border: none; }");
        if (type == EType::TypeError)
        {
            m_icon->setIcon(QIcon(":/PropertyEditor/Resources/save_fail.png"));
        }
        else if (type == EType::TypeSuccess)
        {
            m_icon->setIcon(QIcon(":/PropertyEditor/Resources/save_succeed.png"));
        }
        connect(m_icon, SIGNAL(pressed()), this, SLOT(IconPressed()));

        m_messageLabel = m_ui->label;
        m_messageLabel->setText(Message);
        m_messageLabel->setAttribute(Qt::WA_TranslucentBackground);

        // set the opacity
        m_opacity = SliceUIConstants::opacity;

        // start the timer
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(TimerTimeOut()));
        m_timer->start(SliceUIConstants::timerStartValue);
    }

    SliceOverridesNotificationWindow::~SliceOverridesNotificationWindow()
    {
        // remove from the notification window manager
        RemoveNotificationWindow(this);
    }


    // paint event
    void SliceOverridesNotificationWindow::paintEvent(QPaintEvent* /* unused */)
    {
        QPainter p(this);
        p.setPen(Qt::transparent);
        QColor painterColor;
        painterColor.setRgbF(0, 0, 0, m_opacity);
        p.setBrush(painterColor);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawRoundedRect(rect(), SliceUIConstants::roundedRectXRadius, SliceUIConstants::roundedRectYRadius);
    }


    // resize event
    void SliceOverridesNotificationWindow::resizeEvent(QResizeEvent* /* unused */)
    {
        setMask(rect());
    }


    // mouse press event
    void SliceOverridesNotificationWindow::mousePressEvent(QMouseEvent* event)
    {
        // we only want the left button, stop here if the timer is not active too
        if ((m_timer->isActive() == false) || (event->button() != Qt::LeftButton))
        {
            return;
        }

        // stop the timer because the event will be called before
        m_timer->stop();

        // call the timer time out function
        TimerTimeOut();
    }


    // icon pressed
    void SliceOverridesNotificationWindow::IconPressed()
    {
        // stop here if the timer is not active
        if (m_timer->isActive() == false)
        {
            return;
        }

        // stop the timer because the event will be called before
        m_timer->stop();

        // call the timer time out function
        TimerTimeOut();
    }


    // timer time out
    void SliceOverridesNotificationWindow::TimerTimeOut()
    {
        //// create the opacity effect and set it on the icon
        QGraphicsOpacityEffect* iconOpacityEffect = new QGraphicsOpacityEffect(this);
        m_icon->setGraphicsEffect(iconOpacityEffect);

        //// create the property animation to control the property value
        QPropertyAnimation* iconPropertyAnimation = new QPropertyAnimation(iconOpacityEffect, "opacity");
        iconPropertyAnimation->setDuration(SliceUIConstants::animationDuration);
        iconPropertyAnimation->setStartValue(m_opacity);
        iconPropertyAnimation->setEndValue(SliceUIConstants::animationEndValue);
        iconPropertyAnimation->setEasingCurve(QEasingCurve::Linear);
        iconPropertyAnimation->start(QPropertyAnimation::DeleteWhenStopped);

        // create the opacity effect and set it on the label
        QGraphicsOpacityEffect* labelOpacityEffect = new QGraphicsOpacityEffect(this);
        m_messageLabel->setGraphicsEffect(labelOpacityEffect);

        // create the property animation to control the property value
        QPropertyAnimation* labelPropertyAnimation = new QPropertyAnimation(labelOpacityEffect, "opacity");
        labelPropertyAnimation->setDuration(SliceUIConstants::animationDuration);
        labelPropertyAnimation->setStartValue(m_opacity);
        labelPropertyAnimation->setEndValue(SliceUIConstants::animationEndValue);
        labelPropertyAnimation->setEasingCurve(QEasingCurve::Linear);
        labelPropertyAnimation->start(QPropertyAnimation::DeleteWhenStopped);

        // connect the animation, the two animations are the same so only connect for the text is enough
        connect(labelOpacityEffect, SIGNAL(opacityChanged(qreal)), this, SLOT(OpacityChanged(qreal)));
        connect(labelPropertyAnimation, SIGNAL(finished()), this, SLOT(FadeOutFinished()));
    }


    // opacity changed
    void SliceOverridesNotificationWindow::OpacityChanged(qreal newOpacity)
    {
        // set the new opacity for the paint of the window
        m_opacity = static_cast<float>(newOpacity);

        // update the window
        update();
    }


    // fade out finished
    void SliceOverridesNotificationWindow::FadeOutFinished()
    {
        // hide the notification window
        hide();

        // remove from the notification window manager
        RemoveNotificationWindow(this);

        // delete later
        deleteLater();
    }

} // namespace AzToolsFramework

#include "UI/Slice/moc_SliceOverridesNotificationWindow.cpp"
