/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Render/GeometryIntersectionBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace UnitTest
{
    //! Basic component that implements BoundsRequestBus and EditorComponentSelectionRequestsBus to be compatible
    //! with the Editor visibility system.
    //! Note: Used for simulating selection (picking) in the viewport.
    class BoundsTestComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AzFramework::BoundsRequestBus::Handler
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(
            BoundsTestComponent, "{E6312E9D-8489-4677-9980-C93C328BC92C}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // EditorComponentSelectionRequestsBus overrides ...
        AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
        bool SupportsEditorRayIntersect() override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() const override;
        AZ::Aabb GetLocalBounds() const override;

        AZ::Aabb m_localBounds; //!< Local bounds that can be modified for certain tests (defaults to unit cube).
    };

    class RenderGeometryIntersectionTestComponent
        : public BoundsTestComponent
        , public AzFramework::RenderGeometry::IntersectionRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(RenderGeometryIntersectionTestComponent, "{6F46B5BF-60DF-4BDD-9BA7-9658E85B99C2}", BoundsTestComponent);

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // IntersectionRequestBus overrides ...
        AzFramework::RenderGeometry::RayResult RenderGeometryIntersect(const AzFramework::RenderGeometry::RayRequest& ray) const override;
    };
} // namespace UnitTest
