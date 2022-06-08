/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/IConsole.h>

#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>

#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialFbxManifestConversion.h>
#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialPrefabConversion.h>

namespace PhysX
{
    void FixAssetsUsingPhysicsLegacyMaterials(const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC(
        "ed_physxFixAssetsUsingPhysicsLegacyMaterials",
        FixAssetsUsingPhysicsLegacyMaterials,
        AZ::ConsoleFunctorFlags::Null,
        "Finds assets that reference legacy physics material ids and fixes them by using new physics material assets.");

    Physics::Utils::LegacyMaterialIdToNewAssetIdMap CollectConvertedMaterialIds()
    {
        Physics::Utils::LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap;

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&legacyMaterialIdToNewAssetIdMap](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType != Physics::MaterialAsset::RTTI_Type())
            {
                return;
            }

            AZ::Data::Asset<Physics::MaterialAsset> materialAsset(assetId, assetInfo.m_assetType);
            materialAsset.QueueLoad();
            materialAsset.BlockUntilLoadComplete();

            if (materialAsset.IsReady())
            {
                if (const AZ::Uuid& legacyPhysicsMaterialId = materialAsset->GetLegacyPhysicsMaterialId().m_id;
                    !legacyPhysicsMaterialId.IsNull())
                {
                    legacyMaterialIdToNewAssetIdMap.emplace(legacyPhysicsMaterialId, assetId);
                }
            }
            else
            {
                AZ_Warning("PhysXMaterialConversion", false, "Unable to load physics material asset '%s'.", assetInfo.m_relativePath.c_str());
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, assetEnumerationCB, nullptr);

        return legacyMaterialIdToNewAssetIdMap;
    }

    void FixAssetsUsingPhysicsLegacyMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
    {
        AZ_TracePrintf("PhysXMaterialConversion", "Searching for converted physics material assets...\n");
        Physics::Utils::LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap = CollectConvertedMaterialIds();
        if (legacyMaterialIdToNewAssetIdMap.empty())
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No converted physics material assets found.\n");
            AZ_TracePrintf("PhysXMaterialConversion", "Command stopped as there are no physics materials with legacy information to be able to fix assets.\n");
            return;
        }
        AZ_TracePrintf("PhysXMaterialConversion", "Found %zu converted physics materials.\n", legacyMaterialIdToNewAssetIdMap.size());
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        FixPrefabsWithPhysicsLegacyMaterials(legacyMaterialIdToNewAssetIdMap);

        FixFbxManifestsWithPhysicsLegacyMaterials(legacyMaterialIdToNewAssetIdMap);

        Physics::Utils::PhysicsMaterialConversionRequestBus::Broadcast(
            &Physics::Utils::PhysicsMaterialConversionRequestBus::Events::FixPhysicsLegacyMaterials,
            legacyMaterialIdToNewAssetIdMap);
    }
} // namespace PhysX
