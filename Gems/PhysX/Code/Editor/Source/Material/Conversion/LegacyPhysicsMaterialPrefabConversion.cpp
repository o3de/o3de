/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Physics/Material/Legacy/LegacyPhysicsMaterialSelection.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Spawnable/Spawnable.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Source/EditorColliderComponent.h>
#include <Source/EditorShapeColliderComponent.h>
#include <Source/EditorHeightfieldColliderComponent.h>
#include <Source/PhysXCharacters/Components/EditorCharacterControllerComponent.h>

#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialLibraryConversion.h>
#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialPrefabConversion.h>

namespace PhysX
{
    static const AZ::TypeId EditorBlastFamilyComponentTypeId = AZ::TypeId::CreateString("{ECB1689A-2B65-44D1-9227-9E62962A7FF7}");
    static const AZ::TypeId EditorTerrainPhysicsColliderComponentTypeId = AZ::TypeId::CreateString("{C43FAB8F-3968-46A6-920E-E84AEDED3DF5}");

    struct PrefabInfo
    {
        AzToolsFramework::Prefab::TemplateId m_templateId;
        AzToolsFramework::Prefab::Template* m_template = nullptr;
        AZStd::string m_prefabFullPath;
    };

    AZStd::vector<PrefabInfo> CollectPrefabs()
    {
        AZStd::vector<PrefabInfo> prefabsWithLegacyMaterials;

        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&prefabsWithLegacyMaterials, prefabLoader,
             prefabSystemComponent](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
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
                if (auto templateResult = prefabSystemComponent->FindTemplate(templateId); templateResult.has_value())
                {
                    AzToolsFramework::Prefab::Template& templateRef = templateResult->get();
                    prefabsWithLegacyMaterials.push_back({ templateId, &templateRef, AZStd::move(*assetFullPath) });
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, assetEnumerationCB, nullptr);

        return prefabsWithLegacyMaterials;
    }

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetPrefabEntities(AzToolsFramework::Prefab::PrefabDom& prefab)
    {
        if (!prefab.IsObject())
        {
            return {};
        }

        AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> entities;

        if (auto entitiesIter = prefab.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName);
            entitiesIter != prefab.MemberEnd() && entitiesIter->value.IsObject())
        {
            entities.reserve(entitiesIter->value.MemberCount());

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

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetEntityComponents(AzToolsFramework::Prefab::PrefabDomValue& entity)
    {
        AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> components;

        if (auto componentsIter = entity.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::ComponentsName);
            componentsIter != entity.MemberEnd() && componentsIter->value.IsObject())
        {
            components.reserve(componentsIter->value.MemberCount());

            for (auto componentIter = componentsIter->value.MemberBegin(); componentIter != componentsIter->value.MemberEnd();
                 ++componentIter)
            {
                if (!componentIter->value.IsObject())
                {
                    continue;
                }

                components.push_back(&componentIter->value);
            }
        }

        return components;
    }

    AZ::TypeId GetComponentTypeId(const AzToolsFramework::Prefab::PrefabDomValue& component)
    {
        const auto typeFieldIter = component.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::TypeName);
        if (typeFieldIter == component.MemberEnd())
        {
            return AZ::TypeId::CreateNull();
        }

        AZ::TypeId typeId = AZ::TypeId::CreateNull();
        AZ::JsonSerialization::LoadTypeId(typeId, typeFieldIter->value);

        return typeId;
    }

    AzToolsFramework::Prefab::PrefabDomValue* FindMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        if (memberChain.empty())
        {
            return nullptr;
        }

        auto memberIter = prefabComponent.FindMember(memberChain[0].c_str());
        if (memberIter == prefabComponent.MemberEnd())
        {
            return nullptr;
        }

        for (size_t i = 1; i < memberChain.size(); ++i)
        {
            auto memberFoundIter = memberIter->value.FindMember(memberChain[i].c_str());
            if (memberFoundIter == memberIter->value.MemberEnd())
            {
                return nullptr;
            }
            memberIter = memberFoundIter;
        }
        return &memberIter->value;
    }

    const AzToolsFramework::Prefab::PrefabDomValue* FindMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        return FindMemberChainInPrefabComponent(memberChain, const_cast<AzToolsFramework::Prefab::PrefabDomValue&>(prefabComponent));
    }

    void RemoveMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        if (memberChain.empty())
        {
            return;
        }
        else if (memberChain.size() == 1)
        {
            // If the member chain is only one member, just remove it from the component.
            prefabComponent.RemoveMember(memberChain[0].c_str());
            return;
        }

        // If the chain is 2 or more members, find the member previous to last.

        auto memberIter = prefabComponent.FindMember(memberChain[0].c_str());
        if (memberIter == prefabComponent.MemberEnd())
        {
            return;
        }

        for (size_t i = 1; i < memberChain.size() - 1; ++i)
        {
            auto memberFoundIter = memberIter->value.FindMember(memberChain[i].c_str());
            if (memberFoundIter == memberIter->value.MemberEnd())
            {
                return;
            }
            memberIter = memberFoundIter;
        }

        // Remove the last member in the chain.
        memberIter->value.RemoveMember(memberChain.back().c_str());
    }

    template<class T>
    bool LoadObjectFromPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent, T& object)
    {
        const auto* member = FindMemberChainInPrefabComponent(memberChain, prefabComponent);
        if (!member)
        {
            return false;
        }

        auto result = AZ::JsonSerialization::Load(&object, azrtti_typeid<T>(), *member);

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }

    template<class T>
    bool StoreObjectToPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain,
        AzToolsFramework::Prefab::PrefabDom& prefabDom,
        AzToolsFramework::Prefab::PrefabDomValue& prefabComponent,
        const T& object)
    {
        auto* member = FindMemberChainInPrefabComponent(memberChain, prefabComponent);
        if (!member)
        {
            return false;
        }

        T defaultObject;

        auto result = AZ::JsonSerialization::Store(*member, prefabDom.GetAllocator(), &object, &defaultObject, azrtti_typeid<T>());

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
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
                slotNames.push_back(AZStd::string::format("Material %d", i + 1));
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

    bool FixPhysicsMaterialSelection(
        PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& component,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap,
        const AZStd::vector<AZStd::string>& oldMemberChain,
        const AZStd::vector<AZStd::string>& newMemberChain)
    {
        auto isDefaultMaterialSlots = [](const Physics::MaterialSlots& materialSlots)
        {
            const Physics::MaterialSlots defaultMaterialSlots;
            return materialSlots.GetSlotsNames() == defaultMaterialSlots.GetSlotsNames() &&
                materialSlots.GetMaterialAsset(0) == defaultMaterialSlots.GetMaterialAsset(0);
        };

        PhysicsLegacy::MaterialSelection legacyMaterialSelection;
        if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(oldMemberChain, component, legacyMaterialSelection))
        {
            const Physics::MaterialSlots materialSlots =
                ConvertLegacyMaterialSelectionToMaterialSlots(legacyMaterialSelection, legacyMaterialIdToNewAssetIdMap);

            if (isDefaultMaterialSlots(materialSlots))
            {
                return false;
            }

            if (!StoreObjectToPrefabComponent<Physics::MaterialSlots>(
                    newMemberChain, prefabInfo.m_template->GetPrefabDom(), component, materialSlots))
            {
                AZ_Warning(
                    "PhysXMaterialConversion", false, "Unable to set physics material slots to prefab '%s'.",
                    prefabInfo.m_prefabFullPath.c_str());
                return false;
            }

            // Remove legacy material selection field
            RemoveMemberChainInPrefabComponent(oldMemberChain, component);

            AZ_TracePrintf("PhysXMaterialConversion", "Legacy material selection will be replaced by physics material slots.\n");
            if (!legacyMaterialSelection.m_materialIdsAssignedToSlots.empty())
            {
                AZ_Assert(
                    legacyMaterialSelection.m_materialIdsAssignedToSlots.size() == materialSlots.GetSlotsCount(),
                    "Number of elements in legacy material selection (%zu) and match slots (%zu) do not match.",
                    legacyMaterialSelection.m_materialIdsAssignedToSlots.size(), materialSlots.GetSlotsCount());

                for (size_t i = 0; i < materialSlots.GetSlotsCount(); ++i)
                {
                    AZ_TracePrintf(
                        "PhysXMaterialConversion", "  Slot %zu) Legacy material id '%s' -> material asset '%s'.\n", i + 1,
                        legacyMaterialSelection.m_materialIdsAssignedToSlots[i].m_id.ToString<AZStd::string>().c_str(),
                        materialSlots.GetMaterialAsset(i).GetHint().c_str());
                }
            }

            return true;
        }
        return false;
    };

    bool FixPhysicsMaterialId(
        PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& component,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap,
        const AZStd::vector<AZStd::string>& oldMemberChain,
        const AZStd::vector<AZStd::string>& newMemberChain)
    {
        PhysicsLegacy::MaterialId legacyMaterialId;
        if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialId>(oldMemberChain, component, legacyMaterialId))
        {
            AZ::Data::Asset<Physics::MaterialAsset> materialAsset =
                ConvertLegacyMaterialIdToMaterialAsset(legacyMaterialId, legacyMaterialIdToNewAssetIdMap);

            if (!materialAsset.GetId().IsValid())
            {
                return false;
            }

            if (!StoreObjectToPrefabComponent<AZ::Data::Asset<Physics::MaterialAsset>>(
                    newMemberChain, prefabInfo.m_template->GetPrefabDom(), component, materialAsset))
            {
                AZ_Warning(
                    "PhysXMaterialConversion", false, "Unable to set physics material asset to prefab '%s'.",
                    prefabInfo.m_prefabFullPath.c_str());
                return false;
            }

            // Remove legacy material id field
            RemoveMemberChainInPrefabComponent(oldMemberChain, component);

            AZ_TracePrintf(
                "PhysXMaterialConversion", "Legacy material id '%s' will be replaced by physics material asset '%s'.\n",
                legacyMaterialId.m_id.ToString<AZStd::string>().c_str(), materialAsset.GetHint().c_str());

            return true;
        }
        return false;
    };

    bool FixTerrainPhysicsColliderMaterials(
        PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& component,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        bool modifiedTerrainPrefab = false;

        // Fix terrain default material.
        // It has a legacy material selection, but because it only needs 1 material the new version has one new material asset,
        // instead of material slots. So in this particular case it has convert the first legacy material id of the material
        // selection to a new material asset.
        PhysicsLegacy::MaterialSelection legacyDefaultMaterialSelection;
        if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(
                { "Configuration", "DefaultMaterial" }, component, legacyDefaultMaterialSelection))
        {
            PhysicsLegacy::MaterialId legacyMaterialId;
            if (!legacyDefaultMaterialSelection.m_materialIdsAssignedToSlots.empty())
            {
                legacyMaterialId = legacyDefaultMaterialSelection.m_materialIdsAssignedToSlots[0];
            }

            AZ::Data::Asset<Physics::MaterialAsset> materialAsset =
                ConvertLegacyMaterialIdToMaterialAsset(legacyMaterialId, legacyMaterialIdToNewAssetIdMap);

            if (materialAsset.GetId().IsValid())
            {
                if (StoreObjectToPrefabComponent<AZ::Data::Asset<Physics::MaterialAsset>>(
                        { "Configuration", "DefaultMaterialAsset" }, prefabInfo.m_template->GetPrefabDom(), component, materialAsset))
                {
                    // Remove legacy material selection field
                    RemoveMemberChainInPrefabComponent({ "Configuration", "DefaultMaterial" }, component);

                    AZ_TracePrintf(
                        "PhysXMaterialConversion",
                        "Legacy selection with one material (id '%s') will be replaced by physics material asset '%s'.\n",
                        legacyMaterialId.m_id.ToString<AZStd::string>().c_str(), materialAsset.GetHint().c_str());

                    modifiedTerrainPrefab = true;
                }
                else
                {
                    AZ_Warning(
                        "PhysXMaterialConversion", false, "Unable to set physics material asset to prefab '%s'.",
                        prefabInfo.m_prefabFullPath.c_str());
                }
            }
        }

        // Fix terrain mappings, which is an array of legacy material ids, which will be converted to new material assets.
        if (auto* mappingMember = FindMemberChainInPrefabComponent({ "Configuration", "Mappings" }, component); mappingMember != nullptr)
        {
            for (rapidjson_ly::SizeType i = 0; i < mappingMember->Size(); ++i)
            {
                if (FixPhysicsMaterialId(prefabInfo, (*mappingMember)[i], legacyMaterialIdToNewAssetIdMap,
                    { "Material" }, { "MaterialAsset" }))
                {
                    modifiedTerrainPrefab = true;
                }
            }
        }

        return modifiedTerrainPrefab;
    };

    void FixPrefabPhysicsMaterials(PrefabInfo& prefabInfo, const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        bool prefabModified = false;
        for (auto* entity : GetPrefabEntities(prefabInfo.m_template->GetPrefabDom()))
        {
            for (auto* component : GetEntityComponents(*entity))
            {
                const AZ::TypeId componentTypeId = GetComponentTypeId(*component);

                if (componentTypeId == azrtti_typeid<EditorColliderComponent>())
                {
                    if (FixPhysicsMaterialSelection( prefabInfo, *component, legacyMaterialIdToNewAssetIdMap,
                        { "ColliderConfiguration", "MaterialSelection" },
                        { "ColliderConfiguration", "MaterialSlots" }))
                    {
                        prefabModified = true;
                    }
                }
                else if (componentTypeId == azrtti_typeid<EditorShapeColliderComponent>())
                {
                    if (FixPhysicsMaterialSelection(prefabInfo, *component, legacyMaterialIdToNewAssetIdMap,
                        { "ColliderConfiguration", "MaterialSelection" },
                        { "ColliderConfiguration", "MaterialSlots" }))
                    {
                        prefabModified = true;
                    }
                }
                else if (componentTypeId == azrtti_typeid<EditorHeightfieldColliderComponent>())
                {
                    if (FixPhysicsMaterialSelection(prefabInfo, *component, legacyMaterialIdToNewAssetIdMap,
                        { "ColliderConfiguration", "MaterialSelection" },
                        { "ColliderConfiguration", "MaterialSlots" }))
                    {
                        prefabModified = true;
                    }
                }
                else if (componentTypeId == azrtti_typeid<EditorCharacterControllerComponent>())
                {
                    if (FixPhysicsMaterialSelection(prefabInfo, *component, legacyMaterialIdToNewAssetIdMap,
                        { "Configuration", "Material" },
                        { "Configuration", "MaterialSlots" }))
                    {
                        prefabModified = true;
                    }
                }
                else if (componentTypeId == EditorTerrainPhysicsColliderComponentTypeId)
                {
                    if (FixTerrainPhysicsColliderMaterials(prefabInfo, *component, legacyMaterialIdToNewAssetIdMap))
                    {
                        prefabModified = true;
                    }
                }
                else if (componentTypeId == EditorBlastFamilyComponentTypeId)
                {
                    if (FixPhysicsMaterialId(prefabInfo, *component, legacyMaterialIdToNewAssetIdMap,
                        { "PhysicsMaterial" }, { "PhysicsMaterialAsset" }))
                    {
                        prefabModified = true;
                    }
                }
            }
        }

        if (prefabModified)
        {
            AZ_TracePrintf("PhysXMaterialConversion", "Saving modified prefab '%s'.\n", prefabInfo.m_prefabFullPath.c_str());

            auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            prefabInfo.m_template->MarkAsDirty(true);
            prefabSystemComponent->PropagateTemplateChanges(prefabInfo.m_templateId);

            // Request source control to edit prefab file
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, prefabInfo.m_prefabFullPath.c_str(), true,
                [prefabInfo]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info)
                {
                    // This is called from the main thread on the next frame from TickBus,
                    // that is why 'prefabWithLegacyMaterials' is captured as a copy.
                    if (!info.IsReadOnly())
                    {
                        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                        if (!prefabLoader->SaveTemplate(prefabInfo.m_templateId))
                        {
                            AZ_Warning("PhysXMaterialConversion", false, "Unable to save prefab '%s'", prefabInfo.m_prefabFullPath.c_str());
                        }
                    }
                    else
                    {
                        AZ_Warning(
                            "PhysXMaterialConversion", false, "Unable to check out asset '%s' in source control.",
                            prefabInfo.m_prefabFullPath.c_str());
                    }
                });

            AZ_TracePrintf("PhysXMaterialConversion", "\n");
        }
    }

    void FixPrefabsWithPhysicsLegacyMaterials(const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        bool prefabSystemEnabled = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
        if (!prefabSystemEnabled)
        {
            AZ_TracePrintf("PhysXMaterialConversion", "Prefabs system is not enabled. Prefabs won't be converted.\n");
            AZ_TracePrintf("PhysXMaterialConversion", "\n");
            return;
        }

        AZ_TracePrintf("PhysXMaterialConversion", "Searching for prefabs to convert...\n");
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        AZStd::vector<PrefabInfo> prefabs = CollectPrefabs();
        if (prefabs.empty())
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No prefabs found.\n");
            AZ_TracePrintf("PhysXMaterialConversion", "\n");
            return;
        }

        for (auto& prefab : prefabs)
        {
            FixPrefabPhysicsMaterials(prefab, legacyMaterialIdToNewAssetIdMap);
        }

        AZ_TracePrintf("PhysXMaterialConversion", "Prefab conversion finished.\n");
        AZ_TracePrintf("PhysXMaterialConversion", "\n");
    }
} // namespace PhysX
