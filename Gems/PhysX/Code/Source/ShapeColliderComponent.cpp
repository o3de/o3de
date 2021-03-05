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
        // all currently supported shape types scale uniformly based on the largest element of the non-uniform scale
        const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());

        for (auto& shapeConfigPair : m_shapeConfigList)
        {
            if (shapeConfigPair.second)
            {
                shapeConfigPair.second->m_scale = uniformScale;
            }
        }
    }
} // namespace PhysX
