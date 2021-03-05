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
            AZ::Aabb GetWorldBounds() override;
            AZ::Aabb GetLocalBounds() override;

        private:

            AZ::Transform GetTransform() const;

            //! EditorRenderComponentAdapter overrides ...
            u32 OnConfigurationChanged() override;
        };
    } // namespace Render
} // namespace AZ
