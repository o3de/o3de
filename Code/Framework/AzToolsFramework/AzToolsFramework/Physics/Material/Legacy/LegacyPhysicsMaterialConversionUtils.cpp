/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>

namespace Physics::Utils
{
    AZStd::optional<AZStd::string> GetFullSourceAssetPathById(AZ::Data::AssetId assetId)
    {
        AZStd::string assetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);
        AZ_Assert(!assetPath.empty(), "Asset Catalog returned an invalid path from an enumerated asset.");
        if (assetPath.empty())
        {
            AZ_Warning("PhysXMaterialConversion", false, "Not able to get asset path for asset with id %s.",
                assetId.ToString<AZStd::string>().c_str());
            return AZStd::nullopt;
        }

        AZStd::string assetFullPath;
        bool assetFullPathFound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            assetFullPathFound,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath,
            assetPath, assetFullPath);
        if (!assetFullPathFound)
        {
            AZ_Warning("PhysXMaterialConversion", false, "Source file of asset '%s' could not be found.", assetPath.c_str());
            return AZStd::nullopt;
        }

        return { AZStd::move(assetFullPath) };
    }

    AZ::Data::Asset<Physics::MaterialAsset> ConvertLegacyMaterialIdToMaterialAsset(
        const PhysicsLegacy::MaterialId& legacyMaterialId,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        if (legacyMaterialId.m_id.IsNull())
        {
            return {};
        }

        auto it = legacyMaterialIdToNewAssetIdMap.find(legacyMaterialId.m_id);
        if (it == legacyMaterialIdToNewAssetIdMap.end())
        {
            AZ_Warning(
                "PhysXMaterialConversion", false, "Unable to find a physics material asset to replace legacy material id '%s' with.",
                legacyMaterialId.m_id.ToString<AZStd::string>().c_str());
            return {};
        }

        const AZ::Data::AssetId newMaterialAssetId = it->second;

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, newMaterialAssetId);
        AZ::Data::Asset<Physics::MaterialAsset> newMaterialAsset(newMaterialAssetId, assetInfo.m_assetType, assetInfo.m_relativePath);

        return newMaterialAsset;
    }

    Physics::MaterialSlots ConvertLegacyMaterialSelectionToMaterialSlots(
        const PhysicsLegacy::MaterialSelection& legacyMaterialSelection,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        if (legacyMaterialSelection.m_materialIdsAssignedToSlots.empty())
        {
            return {};
        }

        Physics::MaterialSlots newMaterialSlots;

        if (legacyMaterialSelection.m_materialIdsAssignedToSlots.size() == 1)
        {
            // MaterialSlots by default has one slot called "Entire Object", keep that
            // name if the selection only has one entry.
            newMaterialSlots.SetMaterialAsset(0,
                ConvertLegacyMaterialIdToMaterialAsset(
                    legacyMaterialSelection.m_materialIdsAssignedToSlots[0], legacyMaterialIdToNewAssetIdMap));
        }
        else
        {
            AZStd::vector<AZStd::string> slotNames;
            slotNames.reserve(legacyMaterialSelection.m_materialIdsAssignedToSlots.size());
            for (size_t i = 0; i < legacyMaterialSelection.m_materialIdsAssignedToSlots.size(); ++i)
            {
                // Using Material 1, Material 2, etc. for slot names when there is more than one entry.
                slotNames.push_back(AZStd::string::format("Material %zu", i + 1));
            }

            newMaterialSlots.SetSlots(slotNames);
            for (size_t i = 0; i < legacyMaterialSelection.m_materialIdsAssignedToSlots.size(); ++i)
            {
                newMaterialSlots.SetMaterialAsset(i,
                    ConvertLegacyMaterialIdToMaterialAsset(
                        legacyMaterialSelection.m_materialIdsAssignedToSlots[i], legacyMaterialIdToNewAssetIdMap));
            }
        }

        return newMaterialSlots;
    }

    AZStd::set<AZStd::string> CollectFbxManifestsFromAssetType(AZ::TypeId assetType)
    {
        AZStd::set<AZStd::string> fbxManifests;

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&fbxManifests, assetType](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType != assetType)
            {
                return;
            }

            AZStd::optional<AZStd::string> fbxFullPath = GetFullSourceAssetPathById(assetId);
            if (!fbxFullPath.has_value())
            {
                return;
            }

            const AZStd::string fbxManifestFullPath = AZStd::string::format("%s.%s", fbxFullPath->c_str(), FbxAssetInfoExtension);

            // Already collected?
            if (fbxManifests.find(fbxManifestFullPath) != fbxManifests.end())
            {
                return;
            }

            if (!AZ::IO::SystemFile::Exists(fbxManifestFullPath.c_str()))
            {
                return;
            }

            fbxManifests.insert(AZStd::move(fbxManifestFullPath));
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, assetEnumerationCB, nullptr);

        return fbxManifests;
    }

    bool IsDefaultMaterialSlots(const Physics::MaterialSlots& materialSlots)
    {
        const Physics::MaterialSlots defaultMaterialSlots;
        return materialSlots.GetSlotsNames() == defaultMaterialSlots.GetSlotsNames() &&
            materialSlots.GetMaterialAsset(0) == defaultMaterialSlots.GetMaterialAsset(0);
    };
} // namespace Physics::Utils
