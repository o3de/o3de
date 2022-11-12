/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Entity/EntityTypes.h>

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentUtil
        {
            static constexpr AZStd::string_view MaterialExtension("material");
            static constexpr AZStd::string_view MaterialTypeExtension("materialtype");
            static constexpr AZStd::string_view MaterialGraphExtension("materialgraph");
            static constexpr AZStd::string_view MaterialGraphNodeExtension("materialgraphnoode");
            static constexpr AZStd::string_view MaterialGraphTemplateExtension("materialgraphtemplate");
            static constexpr AZStd::string_view ShaderExtension("shader");

            static constexpr AZStd::string_view MaterialExtensionWithDot(".material");
            static constexpr AZStd::string_view MaterialTypeExtensionWithDot(".materialtype");
            static constexpr AZStd::string_view MaterialGraphExtensionWithDot(".materialgraph");
            static constexpr AZStd::string_view MaterialGraphNodeExtensionWithDot(".materialgraphnoode");
            static constexpr AZStd::string_view MaterialGraphTemplateExtensionWithDot(".materialgraphtemplate");
            static constexpr AZStd::string_view ShaderExtensionWithDot(".shader");

            struct MaterialEditData
            {
                AZ::Data::AssetId m_materialAssetId = {};
                AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset = {};
                AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> m_materialTypeAsset = {};
                AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialParentAsset = {};
                AZ::RPI::MaterialSourceData m_materialSourceData;
                AZ::RPI::MaterialTypeSourceData m_materialTypeSourceData;
                AZStd::string m_materialSourcePath;
                AZStd::string m_materialTypeSourcePath;
                AZStd::string m_materialParentSourcePath;
                MaterialPropertyOverrideMap m_materialPropertyOverrideMap = {};
            };

            bool LoadMaterialEditDataFromAssetId(const AZ::Data::AssetId& assetId, MaterialEditData& editData);
            bool SaveSourceMaterialFromEditData(const AZStd::string& path, const MaterialEditData& editData);

            //! Retrieves the material type asset ID for a given material asset ID
            AZ::Data::AssetId GetMaterialTypeAssetIdFromMaterialAssetId(const AZ::Data::AssetId& materialAssetId);

            //! Determines if a set of entities have the same active material type on a given material slot
            //! @param primaryEntityId The entity whose material types will be compared against all others in the set
            //! @param secondaryEntityIds Set of entities that will be compared against material types on the primaryEntityId
            //! @param materialAssignmentId ID of the material type slot that will be tested for quality
            //! @returns True if all of the entities share the same active material type asset on the specified slot
            bool DoEntitiesHaveMatchingMaterialTypes(
                const AZ::EntityId& primaryEntityId,
                const AzToolsFramework::EntityIdSet& secondaryEntityIds,
                const MaterialAssignmentId& materialAssignmentId);

            //! Determines if a set of entities have the same active material on a given material slot
            //! @param primaryEntityId The entity whose materials will be compared against all others in the set
            //! @param secondaryEntityIds Set of entities that will be compared against materials on the primaryEntityId
            //! @param materialAssignmentId ID of the material slot that will be tested for quality
            //! @returns True if all of the entities share the same active material asset on the specified slot
            bool DoEntitiesHaveMatchingMaterials(
                const AZ::EntityId& primaryEntityId,
                const AzToolsFramework::EntityIdSet& secondaryEntityIds,
                const MaterialAssignmentId& materialAssignmentId);

            //! Determines if a set of entities have the same material slot configuration, LODs, etc 
            //! @param primaryEntityId The entity whose material slots will be compared against all others in the set
            //! @param secondaryEntityIds Set of entities that will be compared against material slots on the primaryEntityId
            //! @returns True if all of the entities share the same material slot configuration
            bool DoEntitiesHaveMatchingMaterialSlots(const AZ::EntityId& primaryEntityId, const AzToolsFramework::EntityIdSet& entityIds);

            //! Returns the set of entities selected or pinned in the active entity inspector
            //! This function is only reliable when called from context menu or edit context attribute handlers guaranteed to be called from
            //! within the inspector
            AzToolsFramework::EntityIdSet GetSelectedEntitiesFromActiveInspector();

            //! Removes all entries from a set of entity IDs that do not have the same material slot configuration as the primary entity
            //! @param primaryEntityId The entity whose material slots will be compared against all others in the set
            //! @param secondaryEntityIds Set of entities that will be compared against material slots on the primaryEntityId
            //! @returns All of the entity IDs contained within secondaryEntityIds except for the ones whose materials did not match
            //! primaryEntityId
            AzToolsFramework::EntityIdSet GetEntitiesMatchingMaterialSlots(
                const AZ::EntityId& primaryEntityId, const AzToolsFramework::EntityIdSet& secondaryEntityIds);
        } // namespace EditorMaterialComponentUtil
    } // namespace Render
} // namespace AZ
