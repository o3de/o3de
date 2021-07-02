/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/MathUtils.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>
#include <AzCore/Component/TransformBus.h>

namespace ScriptCanvas
{
    namespace EntityNodes
    {
        using namespace Data;
        static const char* k_categoryName = "Entity/Transform";

        template<int t_Index>
        AZ_INLINE void DefaultScale(Node& node) { SetDefaultValuesByIndex<t_Index>::_(node, Data::One()); }

        AZ_INLINE Vector3Type GetEntityRight(AZ::EntityId entityId, NumberType scale)
        {
            AZ::Transform worldTransform = {};
            AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

            Vector3Type vector = worldTransform.GetBasisX();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetEntityRight, DefaultScale<1>, k_categoryName, "{C12282BE-29D2-497D-8C22-75B940E254E2}", "returns the right direction vector from the specified entity's world transform, scaled by a given value (Lumberyard uses Z up, right handed)", "EntityId", "Scale");

        AZ_INLINE Vector3Type GetEntityForward(AZ::EntityId entityId, NumberType scale)
        {
            AZ::Transform worldTransform = {};
            AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

            Vector3Type vector = worldTransform.GetBasisY();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetEntityForward, DefaultScale<1>, k_categoryName, "{719D9F76-84D4-4B0F-BCEB-26D5D097C7D6}", "returns the forward direction vector from the specified entity' world transform, scaled by a given value (Lumberyard uses Z up, right handed)", "EntityId", "Scale");

        AZ_INLINE Vector3Type GetEntityUp(AZ::EntityId entityId, NumberType scale)
        {
            AZ::Transform worldTransform = {};
            AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

            Vector3Type vector = worldTransform.GetBasisZ();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetEntityUp, DefaultScale<1>, k_categoryName, "{96B86F3F-F022-4611-9AEA-175EA952C562}", "returns the up direction vector from the specified entity's world transform, scaled by a given value (Lumberyard uses Z up, right handed)", "EntityId", "Scale");

        using Registrar = RegistrarGeneric<
            GetEntityRightNode,
            GetEntityForwardNode,
            GetEntityUpNode
        >;
    }
}
