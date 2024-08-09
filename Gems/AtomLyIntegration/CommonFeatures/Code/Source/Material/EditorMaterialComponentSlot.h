/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentNotificationBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <QPixmap>

namespace AZ
{
    namespace Render
    {
        //! Details for a single editable material assignment
        //! EditorMaterialComponentSlot will attempt to forward all changes to selected entities for actions invoked from a context menu or
        //! entity inspector. The set of selected or pinned entities will be retrieved directly from the inspector. Updates will be
        //! applied to the entities and their materials using MaterialComponentRequestBus.
        //! Undo and redo for all of those entities will be managed by calls to OnDataChanged.
        struct EditorMaterialComponentSlot final
            : public AZ::Data::AssetBus::Handler
            , public EditorMaterialSystemComponentNotificationBus::Handler
        {
            AZ_RTTI(EditorMaterialComponentSlot, "{344066EB-7C3D-4E92-B53D-3C9EBD546488}");
            AZ_CLASS_ALLOCATOR(EditorMaterialComponentSlot, SystemAllocator);

            static bool ConvertVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
            static void Reflect(ReflectContext* context);

            EditorMaterialComponentSlot() = default;
            EditorMaterialComponentSlot(const AZ::EntityId& entityId, const MaterialAssignmentId& materialAssignmentId);
            ~EditorMaterialComponentSlot();

            //! Get cached preview image as a buffer to use as an RPE attribute
            //! If a cached image isn't avalible then a request will be made to render one
            AZStd::vector<char> GetPreviewPixmapData() const;

            //! Returns the overridden asset id if it's valid, otherwise gets the default asseet id
            AZ::Data::AssetId GetActiveAssetId() const;

            //! Returns the default asseet id of the material provded by the model
            AZ::Data::AssetId GetDefaultAssetId() const;

            //! Returns the display name of the material slot
            AZStd::string GetLabel() const;

            //! Returns true if the active material asset has a source material
            bool HasSourceData() const;

            //! Assign a new material override asset
            void SetAsset(const Data::Asset<RPI::MaterialAsset>& asset);

            //! Assign a new material override asset
            void SetAssetId(const Data::AssetId& assetId);

            //! Remove material and property overrides
            void Clear();

            //! Remove material overrides
            void ClearMaterial();

            //! Remove property overrides
            void ClearProperties();

            //! Launch the material canvas
            void OpenMaterialCanvas() const;

            //! Launch the material editor application and open the active material for this slot
            void OpenMaterialEditor() const;

            //! Open the material exporter, aka generate source materials dialog, and apply resulting materials to a set of entities
            void OpenMaterialExporter(const AzToolsFramework::EntityIdSet& entityIdsToEdit);

            //! Open the material instance inspector to edit material property overrides for a set of entities 
            void OpenMaterialInspector(const AzToolsFramework::EntityIdSet& entityIdsToEdit);

            //! Open the dialog for mapping UV channels for models and materials
            void OpenUvNameMapInspector(const AzToolsFramework::EntityIdSet& entityIdsToEdit);

            //! Bypass the UI to export the active material to the export path 
            void ExportMaterial(const AZStd::string& exportPath, bool overwrite);

            AZ::EntityId m_entityId;
            MaterialAssignmentId m_id;
            Data::Asset<RPI::MaterialAsset> m_materialAsset;

        private:
            void OpenPopupMenu(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType);
            void OnMaterialChangedFromRPE();
            void OnDataChanged(const AzToolsFramework::EntityIdSet& entityIdsToEdit, bool updateAsset);

            //! EditorMaterialSystemComponentNotificationBus::Handler overrides...
            void OnRenderMaterialPreviewReady(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId, const QPixmap& pixmap) override;

            //! AZ::Data::AssetBus::Handler overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            void UpdatePreview() const;

            mutable bool m_updatePreview = true;
        };

        // Vector of slots for assignable or overridable material data.
        using EditorMaterialComponentSlotContainer = AZStd::vector<EditorMaterialComponentSlot>;

        // Table containing all editable material data that is displayed in the edit context and inspector
        // The vector represents all the LODs that can have material overrides.
        // The container will be populated with every potential material slot on an associated model, using its default values.
        // Whenever changes are made to this container, the modified values are copied into the controller configuration material assignment
        // map as overrides that will be applied to material instances
        using EditorMaterialComponentSlotsByLodContainer = AZStd::vector<EditorMaterialComponentSlotContainer>;
    } // namespace Render
} // namespace AZ
