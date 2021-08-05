/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        // Signal to the object to read save data in the context of a 'preset'
        //
        // Which should consist of a series of visual changes, and nothing data based
        // Optional since not all 'save data' request handles will need to do something with this.
        virtual void ApplyPresetData(const EntitySaveDataContainer& saveDataContainer)
        {
            AZ_UNUSED(saveDataContainer);
        }
    };

    using EntitySaveDataRequestBus = AZ::EBus<EntitySaveDataRequests>;
}
