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