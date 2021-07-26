/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
