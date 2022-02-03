/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/Entity.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <AzQtComponents/Components/ui_ToastNotification.h>

#include <QCursor>
#include <QIcon>
#include <QToolButton>
#include <QPropertyAnimation>
#include <QPainter>

namespace AzQtComponents
{
    ToastNotification::ToastNotification(QWidget* parent, const ToastConfiguration& toastConfiguration)
        : QDialog(parent, Qt::FramelessWindowHint)
        , m_closeOnClick(true)
        , m_ui(new Ui::ToastNotification())
        , m_fadeAnimation(nullptr)
        , m_configuration(toastConfiguration)
    {
        setProperty("HasNoWindowDecorations", true);

        setAttribute(Qt::WA_ShowWithoutActivating);

        m_borderRadius = toastConfiguration.m_borderRadius;
        if (m_borderRadius > 0)
        {
            setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
            setAttribute(Qt::WA_TranslucentBackground);
        }

        m_ui->setupUi(this);

        QIcon toastIcon;

        switch (toastConfiguration.m_toastType)
        {
        case ToastType::Error:
            toastIcon = QIcon(":/stylesheet/img/logging/error.svg");
            break;
        case ToastType::Warning:
            toastIcon = QIcon(":/stylesheet/img/logging/warning-yellow.svg");
            break;
        case ToastType::Information:
            toastIcon = QIcon(":/stylesheet/img/logging/information.svg");
            break;
        case ToastType::Custom:
            toastIcon = QIcon(toastConfiguration.m_customIconImage);
        default:
            break;
        }

        m_ui->iconLabel->setPixmap(toastIcon.pixmap(64, 64));

        m_ui->titleLabel->setText(toastConfiguration.m_title);
        m_ui->mainLabel->setText(toastConfiguration.m_description);

        // hide the optional description if none is provided so the title is centered vertically
        if (toastConfiguration.m_description.isEmpty())
        {
            m_ui->mainLabel->setVisible(false);
            m_ui->verticalLayout->removeWidget(m_ui->mainLabel);
        }

        m_lifeSpan.setInterval(aznumeric_cast<int>(toastConfiguration.m_duration.count()));
        m_closeOnClick = toastConfiguration.m_closeOnClick;

        m_ui->closeButton->setVisible(m_closeOnClick);
        QObject::connect(m_ui->closeButton, &QToolButton::clicked, this, &ToastNotification::accept);

        m_fadeDuration = toastConfiguration.m_fadeDuration;
        
        QObject::connect(&m_lifeSpan, &QTimer::timeout, this, &ToastNotification::FadeOut);
    }

    ToastNotification::~ToastNotification()
    {
    }

    bool ToastNotification::IsDuplicate(const ToastConfiguration& toastConfiguration)
    {
        return toastConfiguration.m_title == m_configuration.m_title 
            && toastConfiguration.m_description == m_configuration.m_description;
    }

    void ToastNotification::paintEvent(QPaintEvent* event)
    {
        if (m_borderRadius > 0)
        {
            QPainter p(this);
            p.setPen(Qt::transparent);
            QColor painterColor;
            painterColor.setRgbF(0, 0, 0, 255);
            p.setBrush(painterColor);
            p.setRenderHint(QPainter::Antialiasing);
            p.drawRoundedRect(rect(), m_borderRadius, m_borderRadius);
        }
        else
        {
            QDialog::paintEvent(event);
        }
    }

    void ToastNotification::ShowToastAtCursor()
    {
        QPoint globalCursorPos = QCursor::pos();

        // Left/middle align it relative to the cursor.
        QPointF anchorPoint(0, 0.5);

        // Magic offset to try to get it to not hide under the cursor.
        // No way to get this programatically from what I can tell.
        globalCursorPos.setX(globalCursorPos.x() + 16);

        ShowToastAtPoint(globalCursorPos, anchorPoint);
    }

    void ToastNotification::ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint)
    {
        show();
        updateGeometry();

        UpdatePosition(screenPosition, anchorPoint);
    }

    void ToastNotification::UpdatePosition(const QPoint& screenPosition, const QPointF& anchorPoint)
    {
        QRect dialogGeometry = geometry();

        QPoint finalPosition;
        finalPosition.setX(aznumeric_cast<int>(screenPosition.x() - dialogGeometry.width() * anchorPoint.x()));
        finalPosition.setY(aznumeric_cast<int>(screenPosition.y() - dialogGeometry.height() * anchorPoint.y()));

        move(finalPosition);
    }

    void ToastNotification::showEvent(QShowEvent* showEvent)
    {
        QDialog::showEvent(showEvent);

        if (m_fadeDuration.count() > 0)
        {
            m_fadeAnimation = new QPropertyAnimation(this, "windowOpacity", this);
            m_fadeAnimation->setKeyValueAt(0, 0);
            m_fadeAnimation->setKeyValueAt(1, 1);

            m_fadeAnimation->setDuration(static_cast<int>(m_fadeDuration.count()));

            m_fadeAnimation->start();

            QObject::connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &ToastNotification::StartTimer);
        }
        else
        {
            StartTimer();
        }
    }

    void ToastNotification::hideEvent(QHideEvent* hideEvent)
    {
        QDialog::hideEvent(hideEvent);

        m_lifeSpan.stop();

        if (m_fadeAnimation)
        {
            m_fadeAnimation->stop();
            delete m_fadeAnimation;
        }

        emit ToastNotificationHidden();
    }

    void ToastNotification::mousePressEvent(QMouseEvent*)
    {
        if (m_closeOnClick)
        {
            emit ToastNotificationInteraction();
            accept();
        }
    }

    bool ToastNotification::eventFilter(QObject*, QEvent* event)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent && mouseEvent->button() == Qt::MouseButton::LeftButton)
            {
                accept();
            }
        }

        return false;
    }

    void ToastNotification::StartTimer()
    {
        delete m_fadeAnimation;
        m_fadeAnimation = nullptr;

        if (m_lifeSpan.interval() != 0)
        {
            m_lifeSpan.start();
        }
    }

    void ToastNotification::FadeOut()
    {
        if (m_fadeDuration.count() > 0)
        {
            m_fadeAnimation = new QPropertyAnimation(this, "windowOpacity", this);
            m_fadeAnimation->setKeyValueAt(0, windowOpacity());
            m_fadeAnimation->setKeyValueAt(1, 0);

            m_fadeAnimation->setDuration(static_cast<int>(m_fadeDuration.count()));

            m_fadeAnimation->start();

            QObject::connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &ToastNotification::accept);
        }
        else
        {
            accept();
        }
    }    
#include "Components/moc_ToastNotification.cpp"
}
