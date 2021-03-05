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
#include <CoreLights/PointLightComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorPointLightComponent final
            : public EditorRenderComponentAdapter<PointLightComponentController, PointLightComponent, PointLightComponentConfig>
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , private TransformNotificationBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<PointLightComponentController, PointLightComponent, PointLightComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorPointLightComponent, EditorPointLightComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorPointLightComponent() = default;
            EditorPointLightComponent(const PointLightComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            // EntityDebugDisplayEventBus overrides ...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            // EditorComponentSelectionRequestsBus overrides ...
            bool SupportsEditorRayIntersect() override { return true; }
            bool EditorSelectionIntersectRayViewport(
                const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;

            // BoundsRequestBus overrides ...
            AZ::Aabb GetWorldBounds() override;
            AZ::Aabb GetLocalBounds() override;

        private:
            AZStd::tuple<float, Vector3> GetRadiusAndPosition() const;

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            //! Returns a radius for the light relative to the viewport, ensuring the the light will always take up at least a certain amount of
            //! screen space for selection and debug drawing
            AZStd::tuple<float, Vector3> GetViewportRadiusAndPosition(const AzFramework::ViewportInfo& viewportInfo) const;

            //! EditorRenderComponentAdapter overrides ...
            u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
