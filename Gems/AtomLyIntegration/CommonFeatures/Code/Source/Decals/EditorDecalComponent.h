/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/Decals/DecalConstants.h>
#include <Decals/DecalComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorDecalComponent final
            : public EditorRenderComponentAdapter<DecalComponentController, DecalComponent, DecalComponentConfig>
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
        {
        public:

            using BaseClass = EditorRenderComponentAdapter<DecalComponentController, DecalComponent, DecalComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDecalComponent, EditorDecalComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorDecalComponent() = default;
            EditorDecalComponent(const DecalComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            // EntityDebugDisplayEventBus overrides ...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            // EditorComponentSelectionRequestsBus overrides ...
            bool SupportsEditorRayIntersect() override;
            // Returns true if the given ray hits the decal
            bool EditorSelectionIntersectRayViewport(const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distance) override;
            // Returns a coarse AABB that surrounds the decal
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;

            // BoundsRequestBus overrides ...
            AZ::Aabb GetWorldBounds() const override;
            AZ::Aabb GetLocalBounds() const override;

        private:

            // Returns the component transform which includes uniform-scale, rotation and translation
            AZ::Transform GetWorldTransform() const;

            // Returns the full transform, including both the uniform scale and non-uniform scale along with rotation and translation
            AZ::Matrix3x4 GetWorldTransformWithNonUniformScale() const;

            //! EditorRenderComponentAdapter overrides ...
            u32 OnConfigurationChanged() override;
        };
    } // namespace Render
} // namespace AZ
