/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace ScriptCanvas
{
    namespace EntityIDNodes
    {
        using namespace Data;
        static const char* k_categoryName = "Entity/Entity";
        
        AZ_INLINE BooleanType IsValid(const EntityIDType& source)
        {
            return source.IsValid();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsValid, k_categoryName, "{0ED8A583-A397-4657-98B1-433673323F21}", "returns true if Source is valid, else false", "Source");
        
        AZ_INLINE StringType ToString(const EntityIDType& source)
        {
            return source.ToString();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToString, k_categoryName, "{B094DCAE-15D5-42A3-8D8C-5BD68FE6E356}", "returns a string representation of Source", "Source");

        AZ_INLINE BooleanType IsActive(const EntityIDType& entityId)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            return (entity && entity->GetState() == AZ::Entity::State::Active);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsActive, k_categoryName, "{DF5240FD-6510-4C24-8382-9515C4B0C7B4}", "returns true if entity with the provided Id is valid and active.", "Entity Id");

        using Registrar = RegistrarGeneric<
            IsValidNode,
            ToStringNode,
            IsActiveNode
        >;

    }
} 

