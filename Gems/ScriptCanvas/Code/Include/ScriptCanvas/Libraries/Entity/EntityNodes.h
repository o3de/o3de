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
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetEntityRight, DefaultScale<1>, "Entity/Transform", "{C12282BE-29D2-497D-8C22-75B940E254E2}", "returns the right direction vector from the specified entity's world transform, scaled by a given value (Lumberyard uses Z up, right handed)", "EntityId", "Scale");

        AZ_INLINE Vector3Type GetEntityForward(AZ::EntityId entityId, NumberType scale)
        {
            AZ::Transform worldTransform = {};
            AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

            Vector3Type vector = worldTransform.GetBasisY();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetEntityForward, DefaultScale<1>, "Entity/Transform", "{719D9F76-84D4-4B0F-BCEB-26D5D097C7D6}", "returns the forward direction vector from the specified entity' world transform, scaled by a given value (Lumberyard uses Z up, right handed)", "EntityId", "Scale");

        AZ_INLINE Vector3Type GetEntityUp(AZ::EntityId entityId, NumberType scale)
        {
            AZ::Transform worldTransform = {};
            AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

            Vector3Type vector = worldTransform.GetBasisZ();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(GetEntityUp, DefaultScale<1>, "Entity/Transform", "{96B86F3F-F022-4611-9AEA-175EA952C562}", "returns the up direction vector from the specified entity's world transform, scaled by a given value (Lumberyard uses Z up, right handed)", "EntityId", "Scale");

        using Registrar = RegistrarGeneric
            <
            GetEntityRightNode,
            GetEntityForwardNode,
            GetEntityUpNode
            >;
    }
}