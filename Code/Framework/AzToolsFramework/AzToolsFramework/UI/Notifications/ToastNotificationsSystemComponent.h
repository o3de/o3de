/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/UI/Notifications/ToastBus.h>
#include <QHash>

namespace AzQtComponents
{
    class ToastNotification;
}

namespace AzToolsFramework
{
    struct ToastNotificationQueue
    {
        ToastId m_activeNotification;
        AZStd::vector<ToastId> m_queue;
    };

    class ToastNotificationsSystemComponent final
        : public AZ::Component
        , protected ToastRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ToastNotificationsSystemComponent, "{E13A5182-AD35-4D27-AFDA-3593EF536810}");

        ToastNotificationsSystemComponent() = default;
        ~ToastNotificationsSystemComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;

        ToastId ShowToastNotification(QWidget* parent, const AzQtComponents::ToastConfiguration& toastConfiguration) override;
        ToastId ShowToastAtCursor(QWidget* parent, const AzQtComponents::ToastConfiguration& toastConfiguration) override;
        ToastId ShowToastAtPoint(QWidget* parent, const QPoint& screenPosition, const QPointF& anchorPoint, const AzQtComponents::ToastConfiguration&) override;

        void HideToastNotification(const ToastId& toastId) override;

    private:
        ToastId CreateToastNotification(QWidget* parent, const AzQtComponents::ToastConfiguration& toastConfiguration);
        void OnNotificationHidden();
        void DisplayQueuedNotification(QWidget* parent);

        ToastId m_activeNotification;
        AZStd::unordered_map<ToastId, AzQtComponents::ToastNotification*> m_notifications;
        //AZStd::unordered_map<QWidget*, ToastNotificationQueue> m_queuedNotifications;
        QHash<QWidget*, ToastNotificationQueue> m_queuedNotifications;
    };
} // AzToolsFramework
