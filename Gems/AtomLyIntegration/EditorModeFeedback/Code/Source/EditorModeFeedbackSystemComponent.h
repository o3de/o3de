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
#include <AzCore/std/functional.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <EditorModeFeedback/EditorModeFeedbackInterface.h>
#include <Atom/RPI.Reflect/Model/ModelLodIndex.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RPI
    {
        class MeshDrawPacket;
        class Material;
    }

    namespace Render
    {
        //! Component for the editor mode feedback system.
        class EditorModeFeedbackSystemComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorModeFeedbackInterface
            , public AZ::TickBus::Handler
            , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorModeFeedbackSystemComponent, "{A88EE29D-4C72-4995-B3BD-41EEDE480487}");

            static void Reflect(AZ::ReflectContext* context);

            ~EditorModeFeedbackSystemComponent();

            // EditorComponentBase overrides ...
            void Activate() override;
            void Deactivate() override;

            // EditorModeFeedbackInterface overrides ...
            bool IsEnabled() const override;
            void RegisterOrUpdateDrawableComponent(
                EntityComponentIdPair entityComponentId, const MeshFeatureProcessorInterface::MeshHandle& meshHandle) override;

        private:
            // ViewportEditorModeNotificationsBus overrides ...
            void OnEditorModeActivated(
                const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
            void OnEditorModeDeactivated(
                const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

            // TickBus overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            //! Loads the pass templates mapping file.
            void LoadPassTemplateMappings();

            //! Flag to specify whether or not the editor feedback effects are active.
            bool m_enabled = false;

            //! Data to construct draw packets for meshes.
            struct MeshHandleDrawPackets
            {
                ~MeshHandleDrawPackets();

                const MeshFeatureProcessorInterface::MeshHandle* m_meshHandle;
                RPI::ModelLodIndex m_modelLodIndex = RPI::ModelLodIndex::Null; 
                AZStd::vector<RPI::MeshDrawPacket> m_meshDrawPackets;
            };
            
            //! Map for entities and their drawable components.
            AZStd::unordered_map<EntityId, AZStd::unordered_map<ComponentId, MeshHandleDrawPackets>> m_entityComponentMeshHandleDrawPackets;

            //! Material for sending draw packets to the entity mask pass.
            Data::Instance<RPI::Material> m_maskMaterial = nullptr;

            //! Settings registery override for enabling/disabling editor mode feedback.
            bool m_registeryEnabled = false;

            //! Used for loading the pass templates.
            RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
        };
    } // namespace Render
} // namespace AZ
