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