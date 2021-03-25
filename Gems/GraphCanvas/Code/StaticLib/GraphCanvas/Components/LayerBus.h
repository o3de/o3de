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
