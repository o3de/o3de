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
#include <AzCore/Component/EntityId.h>

namespace GraphCanvas
{
    class EntitySaveDataContainer;

    //! EntitySaveDataRequests
    class EntitySaveDataRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        // Multiple handlers. Events received in defined order.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        
        // Signal that the Save Data should be written out to the specified container.
        virtual void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const = 0;
        
        // Signal that the Save Data should be read in from the specified container.
        // This should be signalled before the entity is added to the scene.
        virtual void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) = 0;
    };

    using EntitySaveDataRequestBus = AZ::EBus<EntitySaveDataRequests>;
}