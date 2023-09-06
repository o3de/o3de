/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/BoundsTestComponent.h>

#include <AzCore/Math/Obb.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace UnitTest
{
    AZ::Aabb BoundsTestComponent::GetEditorSelectionBoundsViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
    {
        return GetWorldBounds();
    }

    bool BoundsTestComponent::EditorSelectionIntersectRayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        return AzToolsFramework::AabbIntersectRay(src, dir, GetWorldBounds(), distance);
    }

    bool BoundsTestComponent::SupportsEditorRayIntersect()
    {
        return true;
    }

    void BoundsTestComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoundsTestComponent, EditorComponentBase>()->Version(1);
        }
    }

    void BoundsTestComponent::Activate()
    {
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());

        // default local bounds to unit cube
        m_localBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.5f), AZ::Vector3(0.5f));
    }

    void BoundsTestComponent::Deactivate()
    {
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
    }

    AZ::Aabb BoundsTestComponent::GetWorldBounds() const
    {
        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        return GetLocalBounds().GetTransformedAabb(worldFromLocal);
    }

    AZ::Aabb BoundsTestComponent::GetLocalBounds() const
    {
        return m_localBounds;
    }

    void RenderGeometryIntersectionTestComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RenderGeometryIntersectionTestComponent, BoundsTestComponent>()->Version(1);
        }
    }

    void RenderGeometryIntersectionTestComponent::Activate()
    {
        BoundsTestComponent::Activate();

        const AZ::EntityId entityId = GetEntityId();
        AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(contextId, entityId, &AzFramework::EntityIdContextQueries::GetOwningContextId);
        AzFramework::RenderGeometry::IntersectionRequestBus::Handler::BusConnect({entityId, contextId});
    }

    void RenderGeometryIntersectionTestComponent::Deactivate()
    {
        AzFramework::RenderGeometry::IntersectionRequestBus::Handler::BusDisconnect();
        BoundsTestComponent::Deactivate();
    }

    AzFramework::RenderGeometry::RayResult RenderGeometryIntersectionTestComponent::RenderGeometryIntersect(
        const AzFramework::RenderGeometry::RayRequest& ray) const
    {
        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AzFramework::RenderGeometry::RayResult rayResult;

        float t = 0.0f;
        const AZ::Obb obb = GetLocalBounds().GetTransformedObb(worldFromLocal);
        const AZ::Vector3 rayDirection = ray.m_endWorldPosition - ray.m_startWorldPosition;
        if (AZ::Intersect::IntersectRayObb(ray.m_startWorldPosition, rayDirection, obb, t))
        {
            rayResult.m_worldPosition = ray.m_startWorldPosition + rayDirection * t;
            rayResult.m_entityAndComponent = AZ::EntityComponentIdPair(GetEntityId(), GetId());
            rayResult.m_distance = t;
            rayResult.m_uv = AZ::Vector2::CreateZero();
            rayResult.m_worldNormal = AZ::Vector3::CreateZero();
        }

        return rayResult;
    }
} // namespace UnitTest
