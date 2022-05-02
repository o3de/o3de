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
#include <AzCore/std/containers/unordered_map.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Spawnable/Spawnable.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

#include <Material/BlastMaterialAsset.h>
#include <Editor/EditorBlastFamilyComponent.h>

namespace Blast
{
    void FixPrefabsWithBlastComponentLegacyMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC("ed_blastFixPrefabsWithBlastComponentLegacyMaterials", FixPrefabsWithBlastComponentLegacyMaterials, AZ::ConsoleFunctorFlags::Null,
        "Finds prefabs that contain blast components using legacy blast material ids and fixes them by using new blast material assets.");

    AZStd::optional<AZStd::string> GetFullSourceAssetPathById(AZ::Data::AssetId assetId);

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetPrefabEntities(AzToolsFramework::Prefab::PrefabDom& prefabDom)
    {
        if (!prefabDom.IsObject())
        {
            return {};
        }

        AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> entities;
        entities.reserve(prefabDom.MemberCount());

        if (auto entitiesIter = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName);
            entitiesIter != prefabDom.MemberEnd() && entitiesIter->value.IsObject())
        {
            for (auto entityIter = entitiesIter->value.MemberBegin(); entityIter != entitiesIter->value.MemberEnd(); ++entityIter)
            {
                if (entityIter->value.IsObject())
                {
                    entities.push_back(&entityIter->value);
                }
            }
        }

        return entities;
    }

    template<class ComponentType>
    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetPrefabComponents(AzToolsFramework::Prefab::PrefabDomValue& prefabEntity)
    {
        AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> components;
        components.reserve(prefabEntity.MemberCount());

        if (auto componentsIter = prefabEntity.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::ComponentsName);
            componentsIter != prefabEntity.MemberEnd() && componentsIter->value.IsObject())
        {
            for (auto componentIter = componentsIter->value.MemberBegin(); componentIter != componentsIter->value.MemberEnd(); ++componentIter)
            {
                if (!componentIter->value.IsObject())
                {
                    continue;
                }

                // Check the component type
                const auto typeFieldIter = componentIter->value.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::TypeName);
                if (typeFieldIter == componentIter->value.MemberEnd())
                {
                    continue;
                }

                AZ::Uuid typeId = AZ::Uuid::CreateNull();
                AZ::JsonSerialization::LoadTypeId(typeId, typeFieldIter->value);

                // Filter by component type
                if (typeId == azrtti_typeid<ComponentType>())
                {
                    components.push_back(&componentIter->value);
                }
            }
        }

        return components;
    }

    BlastMaterialId GetLegacyBlastMaterialIdFromComponent(const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        auto legacyMaterialFieldIter = prefabComponent.FindMember("BlastMaterial");
        if (legacyMaterialFieldIter == prefabComponent.MemberEnd() || !legacyMaterialFieldIter->value.IsObject())
        {
            return {};
        }

        auto legacyMaterialIdFieldIter = legacyMaterialFieldIter->value.FindMember("BlastMaterialId");
        if (legacyMaterialIdFieldIter == legacyMaterialFieldIter->value.MemberEnd())
        {
            return {};
        }

        AZ::Uuid legacyMaterialId = AZ::Uuid::CreateNull();
        AZ::JsonSerialization::LoadTypeId(legacyMaterialId, legacyMaterialIdFieldIter->value);

        return { legacyMaterialId };
    }

    bool SetBlastMaterialAssetToComponent(
        AzToolsFramework::Prefab::PrefabDom& prefabDom, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent, AZ::Data::AssetId materialAssetId)
    {
        auto blastMaterialAssetFieldIter = prefabComponent.FindMember("BlastMaterialAsset");
        if (blastMaterialAssetFieldIter == prefabComponent.MemberEnd() || !blastMaterialAssetFieldIter->value.IsObject())
        {
            return false;
        }

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, materialAssetId);

        AZ::Data::Asset<MaterialAsset> materialAsset(materialAssetId, assetInfo.m_assetType, assetInfo.m_relativePath);
        AZ::Data::Asset<MaterialAsset> defaultObject;

        auto result = AZ::JsonSerialization::Store(
            blastMaterialAssetFieldIter->value,
            prefabDom.GetAllocator(),
            &materialAsset,
            &defaultObject,
            azrtti_typeid<AZ::Data::Asset<MaterialAsset>>());

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }

    bool PrefabHasLegacyMaterialId(AzToolsFramework::Prefab::PrefabDom& prefabDom)
    {
        for (auto* entity : GetPrefabEntities(prefabDom))
        {
            for (auto* component : GetPrefabComponents<Blast::EditorBlastFamilyComponent>(*entity))
            {
                if (BlastMaterialId legacyMaterialId = GetLegacyBlastMaterialIdFromComponent(*component);
                    !legacyMaterialId.m_id.IsNull())
                {
                    return true;
                }
            }
        }

        return false;
    }

    struct PrefabInfo
    {
        AzToolsFramework::Prefab::TemplateId m_templateId;
        AzToolsFramework::Prefab::Template* m_template = nullptr;
        AZStd::string m_prefabFullPath;
    };

    AZStd::vector<PrefabInfo> CollectPrefabsWithLegacyMaterials()
    {
        AZStd::vector<PrefabInfo> prefabsWithLegacyMaterials;

        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&prefabsWithLegacyMaterials, prefabLoader, prefabSystemComponent](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType != AzFramework::Spawnable::RTTI_Type())
            {
                return;
            }
            
            AZStd::optional<AZStd::string> assetFullPath = GetFullSourceAssetPathById(assetId);
            if (!assetFullPath.has_value())
            {
                return;
            }

            if (auto templateId = prefabLoader->LoadTemplateFromFile(assetFullPath->c_str());
                templateId != AzToolsFramework::Prefab::InvalidTemplateId)
            {
                if (auto templateResult = prefabSystemComponent->FindTemplate(templateId);
                    templateResult.has_value())
                {
                    if (AzToolsFramework::Prefab::Template& templateRef = templateResult->get();
                        PrefabHasLegacyMaterialId(templateRef.GetPrefabDom()))
                    {
                        prefabsWithLegacyMaterials.push_back({ templateId, &templateRef, AZStd::move(*assetFullPath) });
                    }
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr,
            assetEnumerationCB,
            nullptr);

        return prefabsWithLegacyMaterials;
    }

    using LegacyMaterialIdToNewAssetIdMap = AZStd::unordered_map<AZ::Uuid, AZ::Data::AssetId>;

    LegacyMaterialIdToNewAssetIdMap CollectConvertedMaterialIds()
    {
        LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap;

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

    void FixPrefabBlastMaterials(
        PrefabInfo& prefabWithLegacyMaterials,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        AZ_TracePrintf("BlastMaterialConversion", "Fixing prefab '%s'.\n", prefabWithLegacyMaterials.m_prefabFullPath.c_str());

        bool prefabDomModified = false;

        for (auto* entity : GetPrefabEntities(prefabWithLegacyMaterials.m_template->GetPrefabDom()))
        {
            for (auto* component : GetPrefabComponents<Blast::EditorBlastFamilyComponent>(*entity))
            {
                const BlastMaterialId legacyMaterialId = GetLegacyBlastMaterialIdFromComponent(*component);

                auto it = legacyMaterialIdToNewAssetIdMap.find(legacyMaterialId.m_id);
                if (it == legacyMaterialIdToNewAssetIdMap.end())
                {
                    AZ_Warning("BlastMaterialConversion", false, "Unable to find a blast material asset to replace legacy material id '%s' with.",
                        legacyMaterialId.m_id.ToString<AZStd::string>().c_str());
                    continue;
                }
                const AZ::Data::AssetId newMaterialAssetId = it->second;

                AZStd::optional<AZStd::string> newMaterialAssetFullPath = GetFullSourceAssetPathById(newMaterialAssetId);
                AZ_Assert(newMaterialAssetFullPath.has_value(), "Cannot get full source asset path from id %s",
                    newMaterialAssetId.ToString<AZStd::string>().c_str());

                AZ_TracePrintf("BlastMaterialConversion", "Legacy material id '%s' will be replaced by blast material asset '%s'.\n",
                    legacyMaterialId.m_id.ToString<AZStd::string>().c_str(),
                    newMaterialAssetFullPath->c_str());

                // Remove legacy material id field
                component->RemoveMember("BlastMaterial");

                if (!SetBlastMaterialAssetToComponent(prefabWithLegacyMaterials.m_template->GetPrefabDom(), *component, newMaterialAssetId))
                {
                    AZ_Warning("BlastMaterialConversion", false, "Unable to set material asset id value to %s' in prefab.",
                        newMaterialAssetId.ToString<AZStd::string>().c_str());
                    continue;
                }

                prefabDomModified = true;
            }
        }

        if (prefabDomModified)
        {
            auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            prefabWithLegacyMaterials.m_template->MarkAsDirty(true);
            prefabSystemComponent->PropagateTemplateChanges(prefabWithLegacyMaterials.m_templateId);

            // Request source control to edit prefab file
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                prefabWithLegacyMaterials.m_prefabFullPath.c_str(), true,
                [prefabWithLegacyMaterials]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info)
                {
                    // This is called from the main thread on the next frame from TickBus,
                    // that is why 'prefabWithLegacyMaterials' is captured as a copy.
                    if (!info.IsReadOnly())
                    {
                        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                        if (!prefabLoader->SaveTemplate(prefabWithLegacyMaterials.m_templateId))
                        {
                            AZ_Warning("BlastMaterialConversion", false, "Unable to save prefab '%s'",
                                prefabWithLegacyMaterials.m_prefabFullPath.c_str());
                        }
                    }
                    else
                    {
                        AZ_Warning("BlastMaterialConversion", false, "Unable to check out asset '%s' in source control.",
                            prefabWithLegacyMaterials.m_prefabFullPath.c_str());
                    }
                }
            );
        }
        else
        {
            AZ_TracePrintf("BlastMaterialConversion", "No changes were done to the prefab.\n");
        }

        AZ_TracePrintf("BlastMaterialConversion", "\n");
    }

    void FixPrefabsWithBlastComponentLegacyMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
    {
        bool prefabSystemEnabled = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
        if (!prefabSystemEnabled)
        {
            AZ_Error("BlastMaterialConversion", false, "Prefabs system is not enabled.");
            return;
        }

        AZ_TracePrintf("BlastMaterialConversion", "Searching for prefabs with legacy blast material assets...\n");
        AZStd::vector<PrefabInfo> prefabsWithLegacyMaterials = CollectPrefabsWithLegacyMaterials();
        if (prefabsWithLegacyMaterials.empty())
        {
            AZ_TracePrintf("BlastMaterialConversion", "No prefabs found that contain legacy blast materials.\n");
            return;
        }
        AZ_TracePrintf("BlastMaterialConversion", "Found %zu prefabs containing legacy blast materials.\n", prefabsWithLegacyMaterials.size());
        AZ_TracePrintf("BlastMaterialConversion", "\n");

        AZ_TracePrintf("BlastMaterialConversion", "Searching for converted blast material assets...\n");
        LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap = CollectConvertedMaterialIds();
        if (legacyMaterialIdToNewAssetIdMap.empty())
        {
            AZ_TracePrintf("BlastMaterialConversion", "No converted blast material assets found.\n");
            return;
        }
        AZ_TracePrintf("BlastMaterialConversion", "Found %zu converted blast materials.\n", legacyMaterialIdToNewAssetIdMap.size());
        AZ_TracePrintf("BlastMaterialConversion", "\n");

        for (auto& prefabWithLegacyMaterials : prefabsWithLegacyMaterials)
        {
            FixPrefabBlastMaterials(prefabWithLegacyMaterials, legacyMaterialIdToNewAssetIdMap);
        }
    }
} // namespace Blast
