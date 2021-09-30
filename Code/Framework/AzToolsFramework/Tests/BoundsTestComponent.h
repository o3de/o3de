/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;
    };

} // namespace UnitTest
