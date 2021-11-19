/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentNotificationBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <Material/EditorMaterialComponentSlot.h>
#include <Material/MaterialComponent.h>

namespace AZ
{
    namespace Render
    {
        //! In-editor material component for displaying and editing material assignments.
        class EditorMaterialComponent final
            : public EditorRenderComponentAdapter<MaterialComponentController, MaterialComponent, MaterialComponentConfig>
            , public MaterialReceiverNotificationBus::Handler
            , public MaterialComponentNotificationBus::Handler
            , public EditorMaterialSystemComponentNotificationBus::Handler
        {
        public:
            friend class JsonEditorMaterialComponentSerializer;

            using BaseClass = EditorRenderComponentAdapter<MaterialComponentController, MaterialComponent, MaterialComponentConfig>;
            AZ_EDITOR_COMPONENT(EditorMaterialComponent, EditorMaterialComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);
            static bool ConvertVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            EditorMaterialComponent() = default;
            EditorMaterialComponent(const MaterialComponentConfig& config);

        private:
            // BaseClass overrides ...
            void Activate() override;
            void Deactivate() override;

            void AddContextMenuActions(QMenu* menu) override;

            //! Called when you want to change the game asset through code (like when creating components based on assets).
            void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

            AZ::u32 OnConfigurationChanged() override;

            //! MaterialReceiverNotificationBus::Handler overrides...
            void OnMaterialAssignmentsChanged() override;

            //! MaterialComponentNotificationBus::Handler overrides...
            void OnMaterialInstanceCreated(const MaterialAssignment& materialAssignment) override;

            //! EditorMaterialSystemComponentNotificationBus::Handler overrides...
            void OnRenderMaterialPreviewComplete(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId, const QPixmap& pixmap) override;

            // Regenerates the editor component material slots based on the material and
            // LOD mapping from the model or other consumer of materials.
            // If any corresponding material assignments are found in the component
            // controller configuration then those values will be assigned to the editor component slots.
            void UpdateMaterialSlots();

            // Opens the source material export dialog and updates editor material slots based on
            // selected actions
            AZ::u32 OpenMaterialExporter();

            AZ::u32 OnLodsToggled();

            // Get the visibility of the LOD material slots based on the enable flag
            AZ::Crc32 GetLodVisibility() const;

            // Get the visibility of the default material slot based on the enable flag
            AZ::Crc32 GetDefaultMaterialVisibility() const;

            // Get the visibility of the entire component interface based on the number of selected entities
            AZ::Crc32 GetEditorVisibility() const;

            // Get the visibility of the 'multiple entity selected' warning message box
            AZ::Crc32 GetMessageVisibility() const;

            // Evaluate if materials can be edited
            bool IsEditingAllowed() const;

            AZStd::string GetLabelForLod(int lodIndex) const;

            AZStd::string m_message;
            EditorMaterialComponentSlot m_defaultMaterialSlot;
            EditorMaterialComponentSlotContainer m_materialSlots;
            EditorMaterialComponentSlotsByLodContainer m_materialSlotsByLod;
            bool m_materialSlotsByLodEnabled = false;

            static const char* GenerateMaterialsButtonText;
            static const char* GenerateMaterialsToolTipText;
            static const char* ResetMaterialsButtonText;
            static const char* ResetMaterialsToolTipText;
        };
    } // namespace Render
} // namespace AZ
