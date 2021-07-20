/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{    
    class DynamicSlotRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual void OnUserDataChanged() = 0;

        virtual void StartQueueSlotUpdates() = 0;
        virtual void StopQueueSlotUpdates() = 0;
    };

    using DynamicSlotRequestBus = AZ::EBus<DynamicSlotRequests>;
}
