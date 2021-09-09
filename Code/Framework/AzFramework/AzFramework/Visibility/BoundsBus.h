/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>
// required by components implementing BoundsRequestBus to notify the
// Entity that their bounds has changed (or to access the cached entity bound union)
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    //! Implemented by components that provide bounds for use with various systems.
    class BoundsRequests
        : public AZ::ComponentBus
    {
    public:
        static void Reflect(AZ::ReflectContext* context);

        //! Returns an axis aligned bounding box in world space.
        //! @note It is preferred to use CalculateEntityWorldBoundsUnion in the general case as
        //! more than one component may be providing a bound. It isn't guaranteed which bound
        //! will be returned by a single call to GetWorldBounds.
        virtual AZ::Aabb GetWorldBounds() = 0;

        //! Returns an axis aligned bounding box in local space.
        //! @note It is preferred to use CalculateEntityLocalBoundsUnion in the general case as
        //! more than one component may be providing a bound. It isn't guaranteed which bound
        //! will be returned by a single call to GetLocalBounds.
        virtual AZ::Aabb GetLocalBounds() = 0;

    protected:
        ~BoundsRequests() = default;
    };

    using BoundsRequestBus = AZ::EBus<BoundsRequests>;

    //! Returns a union of all local Aabbs provided by components implementing the BoundsRequestBus.
    //! @note It is preferred to call this function as opposed to GetLocalBounds directly as more than one
    //! component may be implementing this bus on an Entity and so only the first result (Aabb) will be returned.
    inline AZ::Aabb CalculateEntityLocalBoundsUnion(const AZ::Entity* entity)
    {
        AZ::EBusReduceResult<AZ::Aabb, AabbUnionAggregator> aabbResult(AZ::Aabb::CreateNull());
        BoundsRequestBus::EventResult(aabbResult, entity->GetId(), &BoundsRequestBus::Events::GetLocalBounds);

        if (aabbResult.value.IsValid())
        {
            return aabbResult.value;
        }

        return AZ::Aabb::CreateFromMinMax(-AZ::Vector3(0.5f), AZ::Vector3(0.5f));
    }

    //! Returns a union of all world Aabbs provided by components implementing the BoundsRequestBus.
    //! @note It is preferred to call this function as opposed to GetWorldBounds directly as more than one
    //! component may be implementing this bus on an Entity and so only the first result (Aabb) will be returned.
    inline AZ::Aabb CalculateEntityWorldBoundsUnion(const AZ::Entity* entity)
    {
        AZ::EBusReduceResult<AZ::Aabb, AabbUnionAggregator> aabbResult(AZ::Aabb::CreateNull());
        BoundsRequestBus::EventResult(aabbResult, entity->GetId(), &BoundsRequestBus::Events::GetWorldBounds);

        if (aabbResult.value.IsValid())
        {
            return aabbResult.value;
        }

        AZ::TransformInterface* transformInterface = entity->GetTransform();
        const AZ::Vector3 worldTranslation = transformInterface->GetWorldTranslation();
        return AZ::Aabb::CreateCenterHalfExtents(worldTranslation, AZ::Vector3(0.5f));
    }
} // namespace AzFramework
