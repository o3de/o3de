/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QPoint>
#include <QPointF>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/UI/Notifications/ToastBus.h>
#endif

namespace AzQtComponents
{
    class ToastNotification;
}

namespace AzToolsFramework
{
    /**
    * \brief A QWidget that displays and manages a queue of toast notifications. 
    *
    * This view must be updated by its parent when the parent widget is show, hidden, moved
    * or resized because toast notifications are displayed on top of the parent and are not part
    * of the layout, so they must be manually moved.
    */
    class ToastNotificationsView final
        : public QWidget 
        , protected ToastRequestBus::Handler
    {
        Q_OBJECT
    public:
        ToastNotificationsView(QWidget* parent, ToastRequestBusId busId);
        ~ToastNotificationsView() override;

        void HideToastNotification(const ToastId& toastId) override;

        ToastId ShowToastNotification(const AzQtComponents::ToastConfiguration& toastConfiguration) override;
        ToastId ShowToastAtCursor(const AzQtComponents::ToastConfiguration& toastConfiguration) override;
        ToastId ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint, const AzQtComponents::ToastConfiguration&) override;

        void OnHide();
        void OnShow();
        void UpdateToastPosition();

        void SetOffset(const QPoint& offset);
        void SetAnchorPoint(const QPointF& anchorPoint);
        void SetMaxQueuedNotifications(AZ::u32 maxQueuedNotifications);
        void SetRejectDuplicates(bool rejectDuplicates);

    private:
        ToastId CreateToastNotification(const AzQtComponents::ToastConfiguration& toastConfiguration);
        void DisplayQueuedNotification();
        bool DuplicateNotificationInQueue(const AzQtComponents::ToastConfiguration& toastConfiguration);
        QPoint GetGlobalPoint();

        ToastId m_activeNotification;
        AZStd::unordered_map<ToastId, AzQtComponents::ToastNotification*> m_notifications;
        AZStd::vector<ToastId> m_queuedNotifications;

        QPoint m_offset = QPoint(10, 10);
        QPointF m_anchorPoint = QPointF(1, 0);
        AZ::u32 m_maxQueuedNotifications = 5;
        bool m_rejectDuplicates = true;
    };
} // AzToolsFramework
