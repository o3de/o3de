/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationPhysXProviderComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    RecastNavigationPhysXProviderComponent::RecastNavigationPhysXProviderComponent(bool debugDrawInputData)
        : RecastNavigationPhysXProviderCommon(false)
        , m_debugDrawInputData(debugDrawInputData)
    {
    }

    void RecastNavigationPhysXProviderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationPhysXProviderComponent, AZ::Component>()
                ->Version(1)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RecastNavigationPhysXProviderComponent>()->RequestBus("RecastNavigationPhysXProviderRequestBus");
        }
    }

    void RecastNavigationPhysXProviderComponent::Activate()
    {
        RecastNavigationPhysXProviderRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RecastNavigationPhysXProviderComponent::Deactivate()
    {
        RecastNavigationPhysXProviderRequestBus::Handler::BusDisconnect();
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> RecastNavigationPhysXProviderComponent::CollectGeometry(
        float tileSize, float borderSize)
    {
        // Blocking call.
        return CollectGeometryImpl(tileSize, borderSize, GetWorldBounds(), m_debugDrawInputData);
    }

    AZ::Aabb RecastNavigationPhysXProviderComponent::GetWorldBounds() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, GetEntityId(),
            &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return worldBounds;
    }

    int RecastNavigationPhysXProviderComponent::GetNumberOfTiles(float tileSize) const
    {
        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        const int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        const int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        return tilesAlongX * tilesAlongY;
    }
} // namespace RecastNavigation
