/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Notifications/ToastNotificationsView.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/functional.h>

namespace AzToolsFramework
{
    ToastNotificationsView::ToastNotificationsView(QWidget* parent, ToastRequestBusId busId)
        : QWidget(parent)
    {
        ToastRequestBus::Handler::BusConnect(busId);
    }

    ToastNotificationsView::~ToastNotificationsView()
    {
        ToastRequestBus::Handler::BusDisconnect();
    }

    void ToastNotificationsView::OnHide()
    {
        QWidget::hide();

        if (m_activeNotification.IsValid())
        {
            auto notificationIter = m_notifications.find(m_activeNotification);
            if (notificationIter != m_notifications.end())
            {
                notificationIter->second->hide();
            }
        }
    }

    void ToastNotificationsView::UpdateToastPosition()
    {
        if (m_activeNotification.IsValid())
        {
            auto notificationIter = m_notifications.find(m_activeNotification);
            if (notificationIter != m_notifications.end())
            {
                notificationIter->second->UpdatePosition(GetGlobalPoint(), m_anchorPoint);
            }
        }
    }

    void ToastNotificationsView::OnShow()
    {
        QWidget::show();

        if (m_activeNotification.IsValid() || !m_queuedNotifications.empty())
        {
            DisplayQueuedNotification();
        }
    }

    ToastId ToastNotificationsView::ShowToastNotification(const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        // reject duplicate messages
        if (m_rejectDuplicates && DuplicateNotificationInQueue(toastConfiguration))
        {
            return ToastId();
        }

        ToastId toastId = CreateToastNotification(toastConfiguration); 
        m_queuedNotifications.emplace_back(toastId);

        if (!m_activeNotification.IsValid())
        {
            DisplayQueuedNotification();
        }
        else if (m_queuedNotifications.size() >= m_maxQueuedNotifications)
        {
            // hiding the active toast will cause the next toast to be displayed
            HideToastNotification(m_activeNotification);
        }

        return toastId;
    }

    bool ToastNotificationsView::DuplicateNotificationInQueue(const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        for (auto iter : m_notifications)
        {
            if (iter.second && iter.second->IsDuplicate(toastConfiguration))
            {
                return true; 
            }
        }

        return false;
    }

    ToastId ToastNotificationsView::ShowToastAtCursor(const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        ToastId toastId = CreateToastNotification(toastConfiguration); 
        m_notifications[toastId]->ShowToastAtCursor();
        return toastId;
    }

    ToastId ToastNotificationsView::ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint, const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        ToastId toastId = CreateToastNotification(toastConfiguration); 
        m_notifications[toastId]->ShowToastAtPoint(screenPosition, anchorPoint);
        return toastId;
    }

    void ToastNotificationsView::HideToastNotification(const ToastId& toastId)
    {
        auto notificationIter = m_notifications.find(toastId);
        if (notificationIter != m_notifications.end())
        {
            auto queuedIter = AZStd::find(m_queuedNotifications.begin(), m_queuedNotifications.end(), toastId);
            if (queuedIter != m_queuedNotifications.end())
            {
                m_queuedNotifications.erase(queuedIter);
            }

            notificationIter->second->reject();
        }
    }

    ToastId ToastNotificationsView::CreateToastNotification(const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        AzQtComponents::ToastNotification* notification = new AzQtComponents::ToastNotification(this, toastConfiguration);
        ToastId toastId = AZ::Entity::MakeId();
        m_notifications[toastId] = notification;

        QObject::connect(
            m_notifications[toastId], &AzQtComponents::ToastNotification::ToastNotificationHidden,
            [toastId]()
            {
                ToastNotificationBus::Event(toastId, &ToastNotificationBus::Events::OnToastDismissed);
            });

        QObject::connect(
            m_notifications[toastId], &AzQtComponents::ToastNotification::ToastNotificationInteraction,
            [toastId]()
            {
                ToastNotificationBus::Event(toastId, &ToastNotificationBus::Events::OnToastInteraction);
            });

        return toastId;
    }

    QPoint ToastNotificationsView::GetGlobalPoint()
    {
        QPoint relativePoint = m_offset;

        AZ_Assert(parentWidget(), "ToastNotificationsView has invalid parent QWidget");
        if (m_anchorPoint.x() == 1.0)
        {
            relativePoint.setX(parentWidget()->width() - m_offset.x());
        }
        if (m_anchorPoint.y() == 1.0)
        {
            relativePoint.setY(parentWidget()->height() - m_offset.y());
        }

        return parentWidget()->mapToGlobal(relativePoint);
    }

    void ToastNotificationsView::DisplayQueuedNotification()
    {        
        AZ_Assert(parentWidget(), "ToastNotificationsView has invalid parent QWidget");
        if (m_queuedNotifications.empty() || !parentWidget()->isVisible() || !isVisible())
        {
            return;
        }

        ToastId toastId = m_queuedNotifications.front();
        m_queuedNotifications.erase(m_queuedNotifications.begin());

        auto notificationIter = m_notifications.find(toastId);
        if (notificationIter != m_notifications.end())
        {
            m_activeNotification = toastId;

            notificationIter->second->ShowToastAtPoint(GetGlobalPoint(), m_anchorPoint);

            QObject::connect(
                notificationIter->second, &AzQtComponents::ToastNotification::ToastNotificationHidden,
                [&]()
                {
                    m_activeNotification.SetInvalid();
                    DisplayQueuedNotification();
                }
                );
        }

        // If we didn't actually show something, recurse to avoid things getting stuck in the queue.
        if (!m_activeNotification.IsValid())
        {
            DisplayQueuedNotification();
        }
    }

    void ToastNotificationsView::SetOffset(const QPoint& offset)
    {
        m_offset = offset;
    }

    void ToastNotificationsView::SetAnchorPoint(const QPointF& anchorPoint)
    {
        m_anchorPoint = anchorPoint;
    }

    void ToastNotificationsView::SetMaxQueuedNotifications(AZ::u32 maxQueuedNotifications)
    {
        m_maxQueuedNotifications = maxQueuedNotifications;
    }

    void ToastNotificationsView::SetRejectDuplicates(bool rejectDuplicates)
    {
        m_rejectDuplicates = rejectDuplicates;
    }
}
