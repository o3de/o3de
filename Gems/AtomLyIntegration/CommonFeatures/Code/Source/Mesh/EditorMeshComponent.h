/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <Mesh/MeshComponent.h>
#include <Mesh/EditorMeshStats.h>

namespace AZ
{
    namespace Render
    {
        /**
         * In-editor mesh component.
         * Conducts some additional listening and operations to ensure immediate
         * effects when changing fields in the editor.
         */
        class EditorMeshComponent final
            : public EditorRenderComponentAdapter<MeshComponentController, MeshComponent, MeshComponentConfig>
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , private MeshComponentNotificationBus::Handler
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<MeshComponentController, MeshComponent, MeshComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorMeshComponent, EditorMeshComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorMeshComponent() = default;
            explicit EditorMeshComponent(const MeshComponentConfig& config);

            // AZ::Component overrides ...
            void Activate() override;
            void Deactivate() override;

            /// Called when you want to change the game asset through code (like when creating components based on assets).
            void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        private:

            // EditorComponentSelectionRequestsBus overrides ...
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
            bool EditorSelectionIntersectRayViewport(const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
            bool SupportsEditorRayIntersect() override;

            // EntityDebugDisplayEventBus overrides ...
            void DisplayEntityViewport(const AzFramework::ViewportInfo&, AzFramework::DebugDisplayRequests&) override;

            // MeshComponentNotificationBus overrides ...
            void OnModelReady(const Data::Asset<RPI::ModelAsset>& modelAsset, const Data::Instance<RPI::Model>& model) override;

            // AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler overrides
            void OnEntityVisibilityChanged(bool visibility) override;

            // AzToolsFramework::Components::EditorComponentAdapter overrides
            bool ShouldActivateController() const override;

            AZ::u32 OnConfigurationChanged() override;

            AZ::Crc32 AddEditorMaterialComponent();
            bool HasEditorMaterialComponent() const;
            AZ::u32 GetEditorMaterialComponentVisibility() const;

            // Flag used for button placement
            bool m_addMaterialComponentFlag = false;

            // Stats for current mesh asset
            EditorMeshStats m_stats;
        };
    } // namespace Render
} // namespace AZ
