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

#include <EMotionFX/Source/PhysicsSetup.h>
#include <EMotionFX/Source/Actor.h>

#include "PhysicsSetupUtils.h"

namespace EMotionFX
{
    size_t PhysicsSetupUtils::CountColliders(const Actor* actor, PhysicsSetup::ColliderConfigType colliderConfigType, bool ignoreShapeType, Physics::ShapeType shapeTypeToCount)
    {
        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(colliderConfigType);
        if (!colliderConfig)
        {
            return 0;
        }

        size_t result = 0;
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : colliderConfig->m_nodes)
        {
            if (ignoreShapeType)
            {
                // Count in all colliders.
                result += nodeConfig.m_shapes.size();
            }
            else
            {
                // Count in only the given collider type.
                for (const Physics::ShapeConfigurationPair& shapeConfigPair : nodeConfig.m_shapes)
                {
                    if (shapeConfigPair.second->GetShapeType() == shapeTypeToCount)
                    {
                        result++;
                    }
                }
            }
        }

        return result;
    }

} //namespace EMotionFX
