/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Visibility/BoundsBus.h>

#include <AzCore/RTTI/BehaviorContext.h>

DECLARE_EBUS_INSTANTIATION(AzFramework::BoundsRequests);

namespace AzFramework
{
    void BoundsRequests::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<BoundsRequestBus>("BoundsRequestBus")
                ->Event("GetWorldBounds", &BoundsRequestBus::Events::GetWorldBounds)
                ->Event("GetLocalBounds", &BoundsRequestBus::Events::GetLocalBounds);
        }
    }

    AZ::Aabb CalculateEntityLocalBoundsUnion(const AZ::Entity* entity)
    {
        AZ::EBusReduceResult<AZ::Aabb, AabbUnionAggregator> aabbResult(AZ::Aabb::CreateNull());
        BoundsRequestBus::EventResult(aabbResult, entity->GetId(), &BoundsRequestBus::Events::GetLocalBounds);

        if (aabbResult.value.IsValid())
        {
            return aabbResult.value;
        }

        return AZ::Aabb::CreateFromMinMax(-AZ::Vector3(0.5f), AZ::Vector3(0.5f));
    }

    AZ::Aabb CalculateEntityWorldBoundsUnion(const AZ::Entity* entity)
    {
        AZ::EBusReduceResult<AZ::Aabb, AabbUnionAggregator> aabbResult(AZ::Aabb::CreateNull());
        BoundsRequestBus::EventResult(aabbResult, entity->GetId(), &BoundsRequestBus::Events::GetWorldBounds);

        if (aabbResult.value.IsValid())
        {
            return aabbResult.value;
        }

        AZ::TransformInterface* transformInterface = entity->GetTransform();
        if (!transformInterface)
        {
            return AZ::Aabb::CreateNull();
        }

        const AZ::Vector3 worldTranslation = transformInterface->GetWorldTranslation();
        return AZ::Aabb::CreateCenterHalfExtents(worldTranslation, AZ::Vector3(0.5f));
    }
} // namespace AzFramework
