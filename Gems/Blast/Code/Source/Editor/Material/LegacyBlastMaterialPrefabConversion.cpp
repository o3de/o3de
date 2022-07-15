/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>
#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsPrefabConversionUtils.h>

#include <Material/BlastMaterialAsset.h>
#include <Editor/EditorBlastFamilyComponent.h>

namespace Blast
{
    void FixPrefabsWithBlastComponentLegacyMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC("ed_blastFixPrefabsWithBlastComponentLegacyMaterials", FixPrefabsWithBlastComponentLegacyMaterials, AZ::ConsoleFunctorFlags::Null,
        "Finds prefabs that contain blast components using legacy blast material ids and fixes them by using new blast material assets.");

    Physics::Utils::LegacyMaterialIdToNewAssetIdMap CollectConvertedMaterialIds()
    {
        Physics::Utils::LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap;

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&legacyMaterialIdToNewAssetIdMap](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType != MaterialAsset::RTTI_Type())
            {
                return;
            }

            AZ::Data::Asset<MaterialAsset> materialAsset(assetId, assetInfo.m_assetType);
            materialAsset.QueueLoad();
            materialAsset.BlockUntilLoadComplete();

            if (materialAsset.IsReady())
            {
                if (const AZ::Uuid& legacyBlastMaterialId = materialAsset->GetLegacyBlastMaterialId().m_id;
                    !legacyBlastMaterialId.IsNull())
                {
                    legacyMaterialIdToNewAssetIdMap.emplace(legacyBlastMaterialId, assetId);
                }
            }
            else
            {
                AZ_Warning("BlastMaterialConversion", false, "Unable to load blast material asset '%s'.", assetInfo.m_relativePath.c_str());
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr,
            assetEnumerationCB,
            nullptr);

        return legacyMaterialIdToNewAssetIdMap;
    }

    AZ::Data::Asset<MaterialAsset> ConvertLegacyMaterialIdToMaterialAsset(
        const BlastMaterialId& legacyMaterialId,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        if (legacyMaterialId.m_id.IsNull())
        {
            return {};
        }

        auto it = legacyMaterialIdToNewAssetIdMap.find(legacyMaterialId.m_id);
        if (it == legacyMaterialIdToNewAssetIdMap.end())
        {
            AZ_Warning(
                "BlastMaterialConversion", false, "Unable to find a blast material asset to replace legacy material id '%s' with.",
                legacyMaterialId.m_id.ToString<AZStd::string>().c_str());
            return {};
        }

        const AZ::Data::AssetId newMaterialAssetId = it->second;

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, newMaterialAssetId);
        AZ::Data::Asset<MaterialAsset> newMaterialAsset(newMaterialAssetId, assetInfo.m_assetType, assetInfo.m_relativePath);

        return newMaterialAsset;
    }

    bool FixBlastMaterialId(
        Physics::Utils::PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& component,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap,
        const AZStd::vector<AZStd::string>& oldMemberChain,
        const AZStd::vector<AZStd::string>& newMemberChain)
    {
        BlastMaterialId legacyMaterialId;
        if (Physics::Utils::LoadObjectFromPrefabComponent<BlastMaterialId>(oldMemberChain, component, legacyMaterialId))
        {
            AZ::Data::Asset<MaterialAsset> materialAsset =
                ConvertLegacyMaterialIdToMaterialAsset(legacyMaterialId, legacyMaterialIdToNewAssetIdMap);

            if (!materialAsset.GetId().IsValid())
            {
                return false;
            }

            if (!Physics::Utils::StoreObjectToPrefabComponent<AZ::Data::Asset<MaterialAsset>>(
                newMemberChain, prefabInfo.m_template->GetPrefabDom(), component, materialAsset))
            {
                AZ_Warning(
                    "BlastMaterialConversion", false, "Unable to set blast material asset to prefab '%s'.",
                    prefabInfo.m_prefabFullPath.c_str());
                return false;
            }

            // Remove legacy material id field
            Physics::Utils::RemoveMemberChainInPrefabComponent(oldMemberChain, component);

            AZ_TracePrintf(
                "BlastMaterialConversion", "Legacy material id '%s' will be replaced by blast material asset '%s'.\n",
                legacyMaterialId.m_id.ToString<AZStd::string>().c_str(), materialAsset.GetHint().c_str());

            return true;
        }
        return false;
    };

    void FixPrefabBlastMaterials(
        Physics::Utils::PrefabInfo& prefabInfo,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        bool prefabModified = false;
        for (auto* entity : Physics::Utils::GetPrefabEntities(prefabInfo.m_template->GetPrefabDom()))
        {
            for (auto* component : Physics::Utils::GetEntityComponents(*entity))
            {
                const AZ::TypeId componentTypeId = Physics::Utils::GetComponentTypeId(*component);

                if (componentTypeId == azrtti_typeid<EditorBlastFamilyComponent>())
                {
                    if (FixBlastMaterialId(prefabInfo, *component, legacyMaterialIdToNewAssetIdMap,
                        { "BlastMaterial" }, { "BlastMaterialAsset" }))
                    {
                        prefabModified = true;
                    }
                }
            }
        }

        if (prefabModified)
        {
            AZ_TracePrintf("BlastMaterialConversion", "Saving modified prefab '%s'.\n", prefabInfo.m_prefabFullPath.c_str());

            auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            prefabInfo.m_template->MarkAsDirty(true);
            prefabSystemComponent->PropagateTemplateChanges(prefabInfo.m_templateId);

            // Request source control to edit prefab file
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                prefabInfo.m_prefabFullPath.c_str(), true,
                [prefabInfo]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info)
                {
                    // This is called from the main thread on the next frame from TickBus,
                    // that is why 'prefabInfo' is captured as a copy.
                    if (!info.IsReadOnly())
                    {
                        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                        if (!prefabLoader->SaveTemplate(prefabInfo.m_templateId))
                        {
                            AZ_Warning("BlastMaterialConversion", false, "Unable to save prefab '%s'",
                                prefabInfo.m_prefabFullPath.c_str());
                        }
                    }
                    else
                    {
                        AZ_Warning("BlastMaterialConversion", false, "Unable to check out asset '%s' in source control.",
                            prefabInfo.m_prefabFullPath.c_str());
                    }
                }
            );

            AZ_TracePrintf("BlastMaterialConversion", "\n");
        }
    }

    void FixPrefabsWithBlastComponentLegacyMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
    {
        bool prefabSystemEnabled = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
        if (!prefabSystemEnabled)
        {
            AZ_TracePrintf("BlastMaterialConversion", "Prefabs system is not enabled. Prefabs won't be converted.\n");
            AZ_TracePrintf("BlastMaterialConversion", "\n");
            return;
        }

        AZ_TracePrintf("BlastMaterialConversion", "Searching for converted blast material assets...\n");
        Physics::Utils::LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap = CollectConvertedMaterialIds();
        if (legacyMaterialIdToNewAssetIdMap.empty())
        {
            AZ_TracePrintf("BlastMaterialConversion", "No converted blast material assets found.\n");
            AZ_TracePrintf("BlastMaterialConversion", "Command stopped as there are no blast materials with legacy information to be able to fix assets.\n");
            return;
        }
        AZ_TracePrintf("BlastMaterialConversion", "Found %zu converted blast materials.\n", legacyMaterialIdToNewAssetIdMap.size());
        AZ_TracePrintf("BlastMaterialConversion", "\n");

        AZ_TracePrintf("BlastMaterialConversion", "Searching for prefabs to convert...\n");
        AZ_TracePrintf("BlastMaterialConversion", "\n");
        AZStd::vector<Physics::Utils::PrefabInfo> prefabs = Physics::Utils::CollectPrefabs();
        if (prefabs.empty())
        {
            AZ_TracePrintf("BlastMaterialConversion", "No prefabs found.\n");
            AZ_TracePrintf("BlastMaterialConversion", "\n");
            return;
        }
        AZ_TracePrintf("BlastMaterialConversion", "Found %zu prefabs to check.\n", prefabs.size());
        AZ_TracePrintf("BlastMaterialConversion", "\n");

        for (auto& prefab : prefabs)
        {
            FixPrefabBlastMaterials(prefab, legacyMaterialIdToNewAssetIdMap);
        }

        AZ_TracePrintf("BlastMaterialConversion", "Prefab conversion finished.\n");
        AZ_TracePrintf("BlastMaterialConversion", "\n");
    }
} // namespace Blast
