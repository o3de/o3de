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

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{    
    class SlotMappingRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual AZ::EntityId MapToGraphCanvasId(const ScriptCanvas::SlotId& slotId) = 0;
    };

    using SlotMappingRequestBus = AZ::EBus<SlotMappingRequests>;

    class SceneMemberMappingConfigurationRequests : public AZ::EBusTraits
    {
    public:
        // This bus will be keyed off of the GraphCanvas::MemberId.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void ConfigureMapping(const AZ::EntityId& scriptCanvasMemberId) = 0;
    };

    using SceneMemberMappingConfigurationRequestBus = AZ::EBus<SceneMemberMappingConfigurationRequests>;

    class SceneMemberMappingRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZ::EntityId GetGraphCanvasEntityId() const = 0;
    };

    using SceneMemberMappingRequestBus = AZ::EBus<SceneMemberMappingRequests>;
}