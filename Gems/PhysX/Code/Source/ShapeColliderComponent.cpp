/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>
#include <ShapeColliderComponent.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    namespace Utils
    {
        Physics::CapsuleShapeConfiguration ConvertFromLmbrCentralCapsuleConfig(
            const LmbrCentral::CapsuleShapeConfig& inputCapsuleConfig)
        {
            return Physics::CapsuleShapeConfiguration(inputCapsuleConfig.m_height, inputCapsuleConfig.m_radius);
        }
    } // namespace Utils

    void ShapeColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShapeColliderComponent, BaseColliderComponent>()
                ->Version(1)
                ;
        }
    }

    // BaseColliderComponent
    void ShapeColliderComponent::UpdateScaleForShapeConfigs()
    {
        const AZ::Vector3 overallScale = Utils::GetOverallScale(GetEntityId());

        for (auto& shapeConfigPair : m_shapeConfigList)
        {
            if (shapeConfigPair.second)
            {
                shapeConfigPair.second->m_scale = overallScale;
            }
        }
    }
} // namespace PhysX
