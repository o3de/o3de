/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

#include <ScriptCanvas/Core/GraphScopedTypes.h>

namespace ScriptCanvas
{
    class FunctionRequests : public AZ::EBusTraits
    {
    public:
        using BusIdType = AZ::EntityId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void OnSignalOut(ID, SlotId) = 0;
    };

    using FunctionRequestBus = AZ::EBus<FunctionRequests>;

    class FunctionNodeNotifications : public AZ::EBusTraits
    {
    public:
        using BusIdType = GraphScopedNodeId;        
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual void OnNameChanged() = 0;
    };

    using FunctionNodeNotificationBus = AZ::EBus<FunctionNodeNotifications>;

    class FunctionNodeRequests : public AZ::EBusTraits
    {
    public:
        using BusIdType = GraphScopedNodeId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual AZStd::string GetName() const = 0;
    };

    using FunctionNodeRequestBus = AZ::EBus<FunctionNodeRequests>;
}
