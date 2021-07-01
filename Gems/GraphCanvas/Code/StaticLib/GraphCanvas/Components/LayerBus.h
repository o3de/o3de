/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

#include <GraphCanvas/Editor/EditorTypes.h>

#include <GraphCanvas/Utils/StateControllers/StateController.h> 

namespace GraphCanvas
{
    class LayerControllerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual StateController< AZStd::string >* GetLayerModifierController() = 0;

        virtual int GetSelectionOffset() const = 0;
        virtual int GetGroupLayerOffset() const = 0;
    };

    using LayerControllerRequestBus = AZ::EBus<LayerControllerRequests>;

    class LayerControllerNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnOffsetsChanged(int selectionOffset, int groupOffset) = 0;
    };

    using LayerControllerNotificationBus = AZ::EBus<LayerControllerNotifications>;
}
