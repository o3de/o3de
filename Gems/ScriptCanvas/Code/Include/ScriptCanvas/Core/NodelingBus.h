/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
