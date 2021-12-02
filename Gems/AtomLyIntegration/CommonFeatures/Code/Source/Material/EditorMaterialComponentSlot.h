/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <QPixmap>

namespace AZ
{
    namespace Render
    {
        //! Details for a single editable material assignment
        struct EditorMaterialComponentSlot final
        {
            AZ_RTTI(EditorMaterialComponentSlot, "{344066EB-7C3D-4E92-B53D-3C9EBD546488}");
            AZ_CLASS_ALLOCATOR(EditorMaterialComponentSlot, SystemAllocator, 0);

            static bool ConvertVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
            static void Reflect(ReflectContext* context);

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
            void SetAsset(const Data::AssetId& assetId);

            //! Assign a new material override asset
            void SetAsset(const Data::Asset<RPI::MaterialAsset>& asset);

            //! Remove material and prperty overrides
            void Clear();

            //! Remove prperty overrides
            void ClearOverrides();

            void OpenMaterialExporter();
            void OpenMaterialEditor() const;
            void OpenMaterialInspector();
            void OpenUvNameMapInspector();

            AZ::EntityId m_entityId;
            MaterialAssignmentId m_id;
            Data::Asset<RPI::MaterialAsset> m_materialAsset;

        private:
            void OpenPopupMenu(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType);
            void OnMaterialChanged() const;
            void OnDataChanged() const;
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
