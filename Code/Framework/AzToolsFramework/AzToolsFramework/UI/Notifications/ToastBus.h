/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzQtComponents/Components/ToastNotificationConfiguration.h>

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QPoint>
#endif

class QWidget;

namespace AzToolsFramework
{
    typedef AZ::EntityId ToastId;

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

    class ToastRequests
        : public AZ::EBusTraits
    {        
    public:   
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ToastRequestBusId; // bus is addressed by CRC of the view name 
        
        virtual ToastId ShowToastNotification(const AzQtComponents::ToastConfiguration& toastConfiguration) = 0;
        virtual ToastId ShowToastAtCursor(const AzQtComponents::ToastConfiguration& toastConfiguration) = 0;
        virtual ToastId ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint, const AzQtComponents::ToastConfiguration&) = 0;

        virtual void HideToastNotification(const ToastId& toastId) = 0;
    };

    using ToastRequestBus = AZ::EBus<ToastRequests>;
}
