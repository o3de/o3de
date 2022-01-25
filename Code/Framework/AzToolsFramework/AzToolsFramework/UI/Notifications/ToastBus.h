/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzQtComponents/Components/ToastNotificationConfiguration.h>

#include <QPoint>
#endif

namespace AzToolsFramework
{
    typedef AZ::EntityId ToastId;

    /**
    * An EBus for receiving notifications when a user interacts with or dismisses
    * a toast notification.
    */
    class ToastNotifications
        : public AZ::EBusTraits
    {        
    public:   
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ToastId;
        
        virtual void OnToastInteraction() {}
        virtual void OnToastDismissed() {}
    };

    using ToastNotificationBus = AZ::EBus<ToastNotifications>;

    typedef AZ::u32 ToastRequestBusId;

    /**
    * An EBus used to hide or show toast notifications.  Generally, these request are handled by a
    * ToastNotificationsView that has been created with a specific ToastRequestBusId
    * e.g. AZ_CRC("ExampleToastNotificationView")
    */
    class ToastRequests
        : public AZ::EBusTraits
    {        
    public:   
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ToastRequestBusId; // bus is addressed by CRC of the view name 
        
        /**
        * Hide a toast notification widget.
        *
        * @param toastId The toast notification's ToastId 
        */
        virtual void HideToastNotification(const ToastId& toastId) = 0;

        /**
        * Show a toast notification with the specified toast configuration.  When handled by a ToastNotificationsView,
        * notifications are queued and presented to the user in sequence.
        *
        * @param toastConfiguration The toast configuration 
        * @return a ToastId
        */
        virtual ToastId ShowToastNotification(const AzQtComponents::ToastConfiguration& toastConfiguration) = 0;

        /**
        * Show a toast notification with the specified toast configuration at the current moust cursor location.
        *
        * @param toastConfiguration The toast configuration 
        * @return a ToastId
        */
        virtual ToastId ShowToastAtCursor(const AzQtComponents::ToastConfiguration& toastConfiguration) = 0;

        /**
        * Show a toast notification with the specified toast configuration at the specified location.
        *
        * @param screenPosition The screen position 
        * @param anchorPoint The anchorPoint for the toast notification widget 
        * @param toastConfiguration The toast configuration 
        * @return a ToastId
        */
        virtual ToastId ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint, const AzQtComponents::ToastConfiguration&) = 0;
    };

    using ToastRequestBus = AZ::EBus<ToastRequests>;
}
