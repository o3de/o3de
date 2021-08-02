/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QCursor>

#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Widgets/ToastNotification/ToastNotification.h>
#include <StaticLib/GraphCanvas/Widgets/ToastNotification/ui_ToastNotification.h>

#include <GraphCanvas/Components/ToastBus.h>

namespace GraphCanvas
{
    //////////////////////
    // ToastNotification
    //////////////////////

    ToastNotification::ToastNotification(QWidget* parent, const ToastConfiguration& toastConfiguration)
        : QDialog(parent, Qt::FramelessWindowHint)
        , m_toastId(AZ::Entity::MakeId())
        , m_closeOnClick(true)
        , m_ui(new Ui::ToastNotification())
        , m_fadeAnimation(nullptr)
    {
        setProperty("HasNoWindowDecorations", true);

        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_DeleteOnClose);

        m_ui->setupUi(this);

        QIcon toastIcon;

        switch (toastConfiguration.GetToastType())
        {
        case ToastType::Error:
            toastIcon = QIcon(":/GraphCanvasEditorResources/toast_error_icon.png");
            break;
        case ToastType::Warning:
            toastIcon = QIcon(":/GraphCanvasEditorResources/toast_warning_icon.png");
            break;
        case ToastType::Information:
            toastIcon = QIcon(":/GraphCanvasEditorResources/toast_information_icon.png");
            break;
        case ToastType::Custom:
            toastIcon = QIcon(toastConfiguration.GetCustomToastImage().c_str());
        default:
            break;
        }

        m_ui->iconLabel->setPixmap(toastIcon.pixmap(64, 64));

        m_ui->titleLabel->setText(toastConfiguration.GetTitleLabel().c_str());
        m_ui->mainLabel->setText(toastConfiguration.GetDescriptionLabel().c_str());

        m_lifeSpan.setInterval(aznumeric_cast<int>(toastConfiguration.GetDuration().count()));
        m_closeOnClick = toastConfiguration.GetCloseOnClick();

        m_ui->closeButton->setVisible(m_closeOnClick);
        QObject::connect(m_ui->closeButton, &QToolButton::clicked, this, &ToastNotification::accept);

        m_fadeDuration = toastConfiguration.GetFadeDuration();
        
        QObject::connect(&m_lifeSpan, &QTimer::timeout, this, &ToastNotification::FadeOut);
    }

    ToastNotification::~ToastNotification()
    {        
    }

    ToastId ToastNotification::GetToastId() const
    {
        return m_toastId;
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

        emit ToastNotificationShown();
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

        ToastNotificationBus::Event(GetToastId(), &ToastNotifications::OnToastDismissed);
        emit ToastNotificationHidden();
    }

    void ToastNotification::mousePressEvent(QMouseEvent*)
    {
        if (m_closeOnClick)
        {
            ToastNotificationBus::Event(GetToastId(), &ToastNotifications::OnToastInteraction);
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
#include <StaticLib/GraphCanvas/Widgets/ToastNotification/moc_ToastNotification.cpp>
}
