/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackInterface.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RPI
    {
        class MeshDrawPacket;
        class Material;
        class ShaderResourceGroup;
    }

    namespace Render
    {
        class EditorEditorModeFeedbackSystemComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorModeFeedbackInterface
            , public AZ::TickBus::Handler
            , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorEditorModeFeedbackSystemComponent, "{A88EE29D-4C72-4995-B3BD-41EEDE480487}");

            static void Reflect(AZ::ReflectContext* context);

            ~EditorEditorModeFeedbackSystemComponent();

            // EditorComponentBase overrides ...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // EditorModeFeedbackInterface overrides ...
            bool IsEnabled() const override;
            void RegisterDrawableComponent(
                EntityComponentIdPair entityComponentId,
                uint32_t objectId,
                const Data::Asset<RPI::ModelAsset>& modelAsset) override;

        private:
            // ViewportEditorModeNotificationsBus overrides ...
            void OnEditorModeActivated(
                const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
            void OnEditorModeDeactivated(
                const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

            // TickBus overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            //! Flag to specify whether or not the editor feedback effects are active.
            bool m_enabled = false;

            //!
            using ModelAssetMeshDrawPackets = AZStd::tuple<const Data::Asset<RPI::ModelAsset>*, uint32_t, AZStd::vector<RPI::MeshDrawPacket>>;

            //!
            AZStd::unordered_map<EntityId, AZStd::unordered_map<ComponentId, ModelAssetMeshDrawPackets>> m_entityComponentDrawPackets;

            //!
            Data::Instance<RPI::Material> m_maskMaterial = nullptr;
        };
    }
}
