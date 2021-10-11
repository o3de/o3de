/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Notifications/ToastNotificationsSystemComponent.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/std/functional.h>
#include <QTimer>

namespace AzToolsFramework
{
    void ToastNotificationsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ToastNotificationsSystemComponent, AZ::Component>();
        }
    }

    void ToastNotificationsSystemComponent::Activate()
    {
        ToastRequestBus::Handler::BusConnect();
    }

    void ToastNotificationsSystemComponent::Deactivate()
    {
        ToastRequestBus::Handler::BusDisconnect();
    }

    class NotificationParentEventFilter
        : public QWidget
    {
    public:
        NotificationParentEventFilter(QWidget* parent, QEvent::Type eventType, const AZStd::function<void(void)>& callback)
            : QWidget(parent)
            , m_callback(callback)
            , m_eventType(eventType)
        {
        }
        bool eventFilter(QObject* object, QEvent* event) override
        {
            if (event->type() == m_eventType)
            {
                m_callback();
            }
            //if (event->type() == QEvent::Move ||  event->type() == QEvent::Resize)
            //{
            //    // reposition the notification
            //    QList<AzQtComponents::ToastNotification*> notifications = parent()->findChildren<AzQtComponents::ToastNotification*>();
            //    foreach (AzQtComponents::ToastNotification* notification,  notifications)
            //    {
            //        // Want this to be roughly in the top right corner of the graphics view.
            //        QPoint globalPoint = parentWidget()->mapToGlobal(QPoint(width() - 10, 10));

            //        // Anchor point will be top right
            //        QPointF anchorPoint = QPointF(1, 0);

            //        notification->UpdatePosition(globalPoint, anchorPoint);
            //    }
            //}
            //else if (event->type() == QEvent::Show)
            //{
            //    m_onShow();
            //}
            //else if (event->type() == QEvent::Hide)
            //{
            //    m_onHide();
            //}
            return QWidget::eventFilter(object, event);
        }

    private:
        QEvent::Type m_eventType;
        AZStd::function<void(void)> m_callback;
    };

    ToastId ToastNotificationsSystemComponent::ShowToastNotification(QWidget* parent, const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        AZ_Assert(parent, "Invalid parent widget provided to ShowToastNotification");
        ToastId toastId = CreateToastNotification(parent, toastConfiguration); 
        if (!m_queuedNotifications.contains(parent))
        {
            m_queuedNotifications.insert(parent, ToastNotificationQueue());

            parent->installEventFilter(new NotificationParentEventFilter(
                parent, QEvent::Show,
                [&]()
                {
                    //QTimer::singleShot(
                    //    0,
                    //    [&]()
                    //    {
                            DisplayQueuedNotification(parent);
                        //});
                }));

            parent->installEventFilter(new NotificationParentEventFilter(
                parent, QEvent::Hide,
                [&]()
                {
                    if (m_queuedNotifications[parent].m_activeNotification.IsValid())
                    {
                        auto notificationIter = m_notifications.find(m_queuedNotifications[parent].m_activeNotification);
                        if (notificationIter != m_notifications.end())
                        {
                            notificationIter->second->hide();
                        }
                    }
                }));

            AZStd::function<void(void)> moveResizeCallback = [&]()
            {
                if (m_queuedNotifications[parent].m_activeNotification.IsValid())
                {
                    QTimer::singleShot(
                        0,
                        [&]()
                        {
                            auto notificationIter = m_notifications.find(m_queuedNotifications[parent].m_activeNotification);
                            if (notificationIter != m_notifications.end())
                            {
                                // Want this to be roughly in the top right corner of the graphics view.
                                QPoint globalPoint = parent->mapToGlobal(QPoint(parent->width() - 10, 10));

                                // Anchor point will be top right
                                QPointF anchorPoint = QPointF(1, 0);

                                notificationIter->second->UpdatePosition(globalPoint, anchorPoint);
                            }
                        });
                }
            };
            parent->installEventFilter(new NotificationParentEventFilter(parent, QEvent::Move, moveResizeCallback));
            parent->installEventFilter(new NotificationParentEventFilter(parent, QEvent::Resize, moveResizeCallback));

            QObject::connect(
                parent, &QObject::destroyed,
                [&]()
                {
                    m_queuedNotifications.remove(parent);
                });
        }

        m_queuedNotifications[parent].m_queue.emplace_back(toastId);

        if (!m_queuedNotifications[parent].m_activeNotification.IsValid())
        {
            DisplayQueuedNotification(parent);
        }

        return toastId;
    }

    ToastId ToastNotificationsSystemComponent::ShowToastAtCursor(QWidget* parent, const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        ToastId toastId = CreateToastNotification(parent, toastConfiguration); 
        m_notifications[toastId]->ShowToastAtCursor();
        return toastId;
    }

    ToastId ToastNotificationsSystemComponent::ShowToastAtPoint(QWidget* parent, const QPoint& screenPosition, const QPointF& anchorPoint, const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        ToastId toastId = CreateToastNotification(parent, toastConfiguration); 
        m_notifications[toastId]->ShowToastAtPoint(screenPosition, anchorPoint);
        return toastId;
    }

    void ToastNotificationsSystemComponent::HideToastNotification(const ToastId& toastId)
    {
        auto notificationIter = m_notifications.find(toastId);
        if (notificationIter != m_notifications.end())
        {
            QWidget* parent = notificationIter->second->parentWidget();
            if (parent && m_queuedNotifications.contains(parent))
            {
                AZStd::vector<ToastId>* queue = &m_queuedNotifications[parent].m_queue;
                auto queuedIter = AZStd::find(queue->begin(), queue->end(), toastId);
                if (queuedIter != queue->end())
                {
                    queue->erase(queuedIter);
                }
            }

            notificationIter->second->reject();
        }
    }

    ToastId ToastNotificationsSystemComponent::CreateToastNotification(QWidget* parent, const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        AzQtComponents::ToastNotification* notification = aznew AzQtComponents::ToastNotification(parent, toastConfiguration);
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

    void ToastNotificationsSystemComponent::DisplayQueuedNotification(QWidget* parent)
    {        
        //AZ_Assert(parent, "Invalid parent widget provided");
        if (!parent)
        {
            return;
        }

        if (!m_queuedNotifications.contains(parent) || m_queuedNotifications[parent].m_queue.empty() || !parent->isVisible())
        {
            return;
        }

        AZStd::vector<ToastId>* queue = &m_queuedNotifications[parent].m_queue;
        ToastId toastId = queue->front();
        queue->erase(queue->begin());

        auto notificationIter = m_notifications.find(toastId);
        if (notificationIter != m_notifications.end())
        {
            m_queuedNotifications[parent].m_activeNotification = toastId;

            // Default position is the top right corner of the parent widget.
            AZ_Assert(parent, "ToastNotification parent is invalid");
            QPoint globalPoint = parent->mapToGlobal(QPoint(parent->width() - 10, 10));

            // Anchor point will be top right
            QPointF anchorPoint = QPointF(1, 0);

            notificationIter->second->ShowToastAtPoint(globalPoint, anchorPoint);

            QObject::connect(
                notificationIter->second, &AzQtComponents::ToastNotification::ToastNotificationHidden,
                [&]()
                {
                    m_queuedNotifications[parent].m_activeNotification.SetInvalid();
                    DisplayQueuedNotification(parent);
                }
                );
        }

        // If we didn't actually show something, recurse to avoid things getting stuck in the queue.
        if (!m_queuedNotifications[parent].m_activeNotification.IsValid())
        {
            DisplayQueuedNotification(parent);
        }
    }
}
