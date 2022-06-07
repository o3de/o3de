/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationPhysXProviderComponent.h"

#include <AzCore/Debug/Budget.h>
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
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationPhysXProviderComponent, AZ::Component>()
                ->Field("Debug Draw Input Data" , &RecastNavigationPhysXProviderComponent::m_debugDrawInputData)
                ->Version(1)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RecastNavigationPhysXProviderComponent>()->RequestBus("RecastNavigationProviderRequestBus");
        }
    }

    void RecastNavigationPhysXProviderComponent::Activate()
    {
        RecastNavigationPhysXProviderCommon::OnActivate();
        RecastNavigationProviderRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RecastNavigationPhysXProviderComponent::Deactivate()
    {
        RecastNavigationPhysXProviderCommon::OnDeactivate();
        RecastNavigationProviderRequestBus::Handler::BusDisconnect();
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> RecastNavigationPhysXProviderComponent::CollectGeometry(
        float tileSize, float borderSize)
    {
        // Blocking call.
        return CollectGeometryImpl(tileSize, borderSize, GetWorldBounds(), m_debugDrawInputData);
    }

    void RecastNavigationPhysXProviderComponent::CollectGeometryAsync(
        float tileSize,
        float borderSize,
        AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> tileCallback)
    {
        CollectGeometryAsyncImpl(tileSize, borderSize, GetWorldBounds(), m_debugDrawInputData, AZStd::move(tileCallback));
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
        const int tilesAlongX = aznumeric_cast<int>(AZStd::ceil(extents.GetX() / tileSize));
        const int tilesAlongY = aznumeric_cast<int>(AZStd::ceil(extents.GetY() / tileSize));

        return tilesAlongX * tilesAlongY;
    }
} // namespace RecastNavigation
