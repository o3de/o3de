/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/StringFunc/StringFunc.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>

#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <PhysX/MeshAsset.h>
#include <Source/Pipeline/MeshGroup.h>

#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialFbxManifestConversion.h>

namespace PhysX
{
    using LegacyMaterialNameToNewAssetIdsMap = AZStd::unordered_map<AZStd::string, AZStd::vector<AZ::Data::AssetId>>;

    static const char* DefaultLegacyPhysicsMaterialName = "<Default Physics Material>";

    AZ::Data::Asset<Physics::MaterialAsset> ConvertLegacyMaterialNameToMaterialAsset(
        const AZStd::string& legacyMaterialName,
        const LegacyMaterialNameToNewAssetIdsMap& legacyMaterialNameToNewAssetIdsMap)
    {
        if (legacyMaterialName.empty() ||
            legacyMaterialName == DefaultLegacyPhysicsMaterialName)
        {
            return {};
        }

        auto it = legacyMaterialNameToNewAssetIdsMap.find(legacyMaterialName);
        if (it == legacyMaterialNameToNewAssetIdsMap.end())
        {
            AZ_Warning(
                "PhysXMaterialConversion", false, "Unable to find a physics material asset to replace legacy material '%s' with.",
                legacyMaterialName.c_str());
            return {};
        }
        AZ_Assert(!it->second.empty(), "Asset materials list should include at least one element");

        // Warn the user that a name collision has been found and that
        // the default material will be used.
        if (it->second.size() > 1)
        {
            AZ_Warning(
                "PhysXMaterialConversion", false,
                "Material name collision found. Legacy material name '%s' has %zu possible physics material assets to be replaced with. Default material will be used.",
                legacyMaterialName.c_str(), it->second.size(), it->second[0].ToString<AZStd::string>().c_str());
            return {};
        }

        const AZ::Data::AssetId newMaterialAssetId = it->second[0];

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, newMaterialAssetId);
        AZ::Data::Asset<Physics::MaterialAsset> newMaterialAsset(newMaterialAssetId, assetInfo.m_assetType, assetInfo.m_relativePath);

        return newMaterialAsset;
    }

    Physics::MaterialSlots ConvertLegacyMaterialSelectionToMaterialSlots(
        const AZStd::vector<AZStd::string>& legacyMaterialSlots,
        const AZStd::vector<AZStd::string>& legacyPhysicsMaterials,
        const LegacyMaterialNameToNewAssetIdsMap& legacyMaterialNameToNewAssetIdsMap)
    {
        if (legacyMaterialSlots.empty() ||
            legacyMaterialSlots.size() != legacyPhysicsMaterials.size())
        {
            return {};
        }

        Physics::MaterialSlots newMaterialSlots;

        newMaterialSlots.SetSlots(legacyMaterialSlots);
        for (size_t i = 0; i < legacyPhysicsMaterials.size(); ++i)
        {
            newMaterialSlots.SetMaterialAsset(i,
                ConvertLegacyMaterialNameToMaterialAsset(
                    legacyPhysicsMaterials[i], legacyMaterialNameToNewAssetIdsMap));
        }

        return newMaterialSlots;
    }

    // Converts legacy material selection inside PhysX Mesh Group
    // into new material slots.
    struct FixPhysXMeshGroup
    {
        static bool Fix(
            Pipeline::MeshGroup& physxMeshGroup,
            const LegacyMaterialNameToNewAssetIdsMap& legacyMaterialNameToNewAssetIdsMap)
        {
            Physics::MaterialSlots materialSlots =
                ConvertLegacyMaterialSelectionToMaterialSlots(
                    physxMeshGroup.m_legacyMaterialSlots, physxMeshGroup.m_legacyPhysicsMaterials, legacyMaterialNameToNewAssetIdsMap);

            if (Physics::Utils::IsDefaultMaterialSlots(materialSlots))
            {
                return false;
            }

            AZ_TracePrintf("PhysXMaterialConversion", "Legacy material selection will be replaced by physics material slots.\n");
            if (!physxMeshGroup.m_legacyPhysicsMaterials.empty())
            {
                AZ_Assert(
                    physxMeshGroup.m_legacyPhysicsMaterials.size() == materialSlots.GetSlotsCount(),
                    "Number of elements in legacy material selection (%zu) and material slots (%zu) do not match.",
                    physxMeshGroup.m_legacyPhysicsMaterials.size(), materialSlots.GetSlotsCount());

                for (size_t i = 0; i < materialSlots.GetSlotsCount(); ++i)
                {
                    AZ_TracePrintf(
                        "PhysXMaterialConversion", "  Slot %zu '%.*s') Legacy material '%s' -> material asset '%s'.\n", i + 1,
                        AZ_STRING_ARG(materialSlots.GetSlotName(i)),
                        physxMeshGroup.m_legacyPhysicsMaterials[i].c_str(),
                        materialSlots.GetMaterialAsset(i).GetHint().c_str());
                }
            }

            physxMeshGroup.m_physicsMaterialSlots = materialSlots;
            physxMeshGroup.m_legacyMaterialSlots = {};
            physxMeshGroup.m_legacyPhysicsMaterials = {};

            return true;
        }
    };

    // Converts all legacy material selections found inside an FBX
    // manifest (PhysX Mesh Group) into new material slots.
    void FixFbxManifestPhysicsMaterials(
        const AZStd::string& fbxManifestFullPath,
        const LegacyMaterialNameToNewAssetIdsMap& legacyMaterialNameToNewAssetIdsMap)
    {
        AZ::SceneAPI::Containers::SceneManifest sceneManifest;
        if (!sceneManifest.LoadFromFile(fbxManifestFullPath))
        {
            AZ_Warning("PhysXMaterialConversion", false, "Unable to load FBX manifest '%s'.", fbxManifestFullPath.c_str());
            return;
        }

        AZ::SceneAPI::Containers::SceneManifest::ValueStorageData valueStorage = sceneManifest.GetValueStorage();
        auto physxMeshGroupFilterView = AZ::SceneAPI::Containers::MakeExactFilterView<Pipeline::MeshGroup>(valueStorage);

        bool fbxManifestModified = false;
        for (Pipeline::MeshGroup& physxMeshGroup : physxMeshGroupFilterView)
        {
            if (FixPhysXMeshGroup::Fix(physxMeshGroup, legacyMaterialNameToNewAssetIdsMap))
            {
                fbxManifestModified = true;
            }
        }

        if (fbxManifestModified)
        {
            AZ_TracePrintf("PhysXMaterialConversion", "Saving fbx manifest '%s'.\n", fbxManifestFullPath.c_str());

            // Request source control to edit prefab file
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, fbxManifestFullPath.c_str(), true,
                [fbxManifestFullPath, sceneManifest]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info) mutable
                {
                    // This is called from the main thread on the next frame from TickBus,
                    // that is why 'fbxManifestFullPath' and 'fbxManifest' are captured as a copy.
                    // The lambda also has to be mutable because `SaveToFile` is not const.
                    if (!info.IsReadOnly())
                    {
                        if (!sceneManifest.SaveToFile(fbxManifestFullPath))
                        {
                            AZ_Warning("PhysXMaterialConversion", false, "Unable to save prefab '%s'", fbxManifestFullPath.c_str());
                        }
                    }
                    else
                    {
                        AZ_Warning(
                            "PhysXMaterialConversion", false, "Unable to check out asset '%s' in source control.",
                            fbxManifestFullPath.c_str());
                    }
                });

            AZ_TracePrintf("PhysXMaterialConversion", "\n");
        }
    }

    // PhysX Mesh Group didn't use legacy Material Id to save the materials,
    // it used material names instead. Because of this the conversion of Mesh Groups
    // might have conflicts, because names can have collisions.
    // For example, 2 different libraries can have one material each with different
    // properties but using the same name, there is no way to know the material of
    // which library to use just by the name.
    // A warning will be printed clarifying that there was a name collision and that
    // the default material will be used.
    LegacyMaterialNameToNewAssetIdsMap ConvertMapToUseNames(
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        LegacyMaterialNameToNewAssetIdsMap legacyMaterialNameToNewAssetIdsMap;

        for (const auto& legacyMaterialIdToNewAssetIdPair : legacyMaterialIdToNewAssetIdMap)
        {
            const auto& newAssetId = legacyMaterialIdToNewAssetIdPair.second;

            AZStd::optional<AZStd::string> newAssetFullPath = Physics::Utils::GetFullSourceAssetPathById(newAssetId);
            if (!newAssetFullPath.has_value())
            {
                continue;
            }

            AZStd::string newAssetFileName;
            if (AZ::StringFunc::Path::GetFileName(newAssetFullPath->c_str(), newAssetFileName))
            {
                legacyMaterialNameToNewAssetIdsMap[newAssetFileName].push_back(newAssetId);
            }
        }

        return legacyMaterialNameToNewAssetIdsMap;
    }

    void FixFbxManifestsWithPhysicsLegacyMaterials(const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        const LegacyMaterialNameToNewAssetIdsMap legacyMaterialNameToNewAssetIdsMap =
            ConvertMapToUseNames(legacyMaterialIdToNewAssetIdMap);

        AZ_TracePrintf("PhysXMaterialConversion", "Searching for FBX manifests with PhysX mesh assets...\n");
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        const AZStd::set<AZStd::string> fbxManifests = Physics::Utils::CollectFbxManifestsFromAssetType(PhysX::Pipeline::MeshAsset::RTTI_Type());
        if (fbxManifests.empty())
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No FBX manifests found.\n");
            AZ_TracePrintf("PhysXMaterialConversion", "\n");
            return;
        }
        AZ_TracePrintf("PhysXMaterialConversion", "Found %zu FBX manifests to check.\n", fbxManifests.size());
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        for (const auto& fbxManifest : fbxManifests)
        {
            FixFbxManifestPhysicsMaterials(fbxManifest, legacyMaterialNameToNewAssetIdsMap);
        }

        AZ_TracePrintf("PhysXMaterialConversion", "FBX manifests conversion finished.\n");
        AZ_TracePrintf("PhysXMaterialConversion", "\n");
    }
} // namespace PhysX
