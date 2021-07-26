/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Entity/BehaviorEntity.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>
#include "EditorComponentAPIBus.h"

namespace AzToolsFramework
{
    //! Exposes the Editor Component CRUD API for the singleton Entity of the current level;
    //! it is exposed to Behavior Context for Editor Scripting.
    //! Use EditorComponentAPIBus For methods that require AZ::EntityComponentIDPairs as input.
    class EditorLevelComponentAPIRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Add Components of the given types to an Entity.
        // Returns an Outcome object - it contains the AZ::EntityComponentIdPairs in case of Success, or an error message in case or Failure.
        virtual EditorComponentAPIRequests::AddComponentsOutcome AddComponentsOfType(const AZ::ComponentTypeList& componentTypeIds) = 0;
        
        //! Returns true if a Component of type provided can be found on the Level Entity, false otherwise.
        virtual bool HasComponentOfType(AZ::Uuid componentTypeId) = 0;

        //! Count Components of type provided on the Level Entity.
        virtual size_t CountComponentsOfType(AZ::Uuid componentTypeId) = 0;

        //! Gets the first Component of type that is attached to the Level Entity.
        // Only returns first component of type if found (early out).
        // Returns an Outcome object - it contains the AZ::EntityComponentIdPair in case of Success, or an error message in case or Failure.
        virtual EditorComponentAPIRequests::GetComponentOutcome GetComponentOfType(AZ::Uuid componentTypeId) = 0;

        //! Get all Components of type that are attached to the Level Entity
        // Returns vector of ComponentIds, or an empty vector if components could not be found.
        virtual EditorComponentAPIRequests::GetComponentsOutcome GetComponentsOfType(AZ::Uuid componentTypeId) = 0;

    };
    using EditorLevelComponentAPIBus = AZ::EBus<EditorLevelComponentAPIRequests>;

}
