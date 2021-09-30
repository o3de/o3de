/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/BoundsTestComponent.h>

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

    void BoundsTestComponent::Reflect([[maybe_unused]] AZ::ReflectContext* context)
    {
        // noop
    }

    void BoundsTestComponent::Activate()
    {
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void BoundsTestComponent::Deactivate()
    {
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
    }

    AZ::Aabb BoundsTestComponent::GetWorldBounds()
    {
        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        return GetLocalBounds().GetTransformedAabb(worldFromLocal);
    }

    AZ::Aabb BoundsTestComponent::GetLocalBounds()
    {
        return AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.5f), AZ::Vector3(0.5f));
    }

} // namespace UnitTest
