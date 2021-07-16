/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                for (const AzPhysics::ShapeColliderPair& shapeConfigPair : nodeConfig.m_shapes)
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
