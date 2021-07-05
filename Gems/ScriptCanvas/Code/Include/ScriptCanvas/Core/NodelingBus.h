/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#include <ScriptCanvas/Core/GraphScopedTypes.h>

namespace ScriptCanvas
{
    class NodelingRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphScopedNodeId;

        virtual AZ::EntityId GetNodeId() const = 0;
        virtual GraphScopedNodeId GetGraphScopedNodeId() const = 0;
        virtual const AZStd::string& GetDisplayName() const = 0;

        virtual void Setup() = 0;
        virtual void SetDisplayName(const AZStd::string& displayName) = 0;
    };
    
    using NodelingRequestBus = AZ::EBus<NodelingRequests>;
    
    class NodelingNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphScopedNodeId;
        
        virtual void OnNameChanged(const AZStd::string& newName) = 0;
    };
    
    using NodelingNotificationBus = AZ::EBus<NodelingNotifications>;
}
