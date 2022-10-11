/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Physics/Material/Legacy/LegacyPhysicsMaterialSelection.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>
#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsPrefabConversionUtils.h>

#include <Source/EditorColliderComponent.h>
#include <Source/EditorShapeColliderComponent.h>
#include <Source/EditorHeightfieldColliderComponent.h>
#include <Source/PhysXCharacters/Components/EditorCharacterControllerComponent.h>

#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialPrefabConversion.h>

namespace PhysX
{
    static const AZ::TypeId EditorBlastFamilyComponentTypeId = AZ::TypeId::CreateString("{ECB1689A-2B65-44D1-9227-9E62962A7FF7}");
    static const AZ::TypeId EditorTerrainPhysicsColliderComponentTypeId = AZ::TypeId::CreateString("{C43FAB8F-3968-46A6-920E-E84AEDED3DF5}");
    static const AZ::TypeId EditorWhiteBoxColliderComponentTypeId = AZ::TypeId::CreateString("{4EF53472-6ED4-4740-B956-F6AE5B4A4BB1}");

    bool FixPhysicsMaterialSelection(
        Physics::Utils::PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& component,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap,
        const AZStd::vector<AZStd::string>& oldMemberChain,
        const AZStd::vector<AZStd::string>& newMemberChain)
    {
        PhysicsLegacy::MaterialSelection legacyMaterialSelection;
        if (Physics::Utils::LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(oldMemberChain, component, legacyMaterialSelection))
        {
            const Physics::MaterialSlots materialSlots =
                Physics::Utils::ConvertLegacyMaterialSelectionToMaterialSlots(legacyMaterialSelection, legacyMaterialIdToNewAssetIdMap);

            if (Physics::Utils::IsDefaultMaterialSlots(materialSlots))
            {
                return false;
            }

            if (!Physics::Utils::StoreObjectToPrefabComponent<Physics::MaterialSlots>(
                    newMemberChain, prefabInfo.m_template->GetPrefabDom(), component, materialSlots))
            {
                AZ_Warning(
                    "PhysXMaterialConversion", false, "Unable to set physics material slots to prefab '%s'.",
                    prefabInfo.m_prefabFullPath.c_str());
                return false;
            }

            // Remove legacy material selection field
            Physics::Utils::RemoveMemberChainInPrefabComponent(oldMemberChain, component);

            AZ_TracePrintf("PhysXMaterialConversion", "Legacy material selection will be replaced by physics material slots.\n");
            if (!legacyMaterialSelection.m_materialIdsAssignedToSlots.empty())
            {
                AZ_Assert(
                    legacyMaterialSelection.m_materialIdsAssignedToSlots.size() == materialSlots.GetSlotsCount(),
                    "Number of elements in legacy material selection (%zu) and material slots (%zu) do not match.",
                    legacyMaterialSelection.m_materialIdsAssignedToSlots.size(), materialSlots.GetSlotsCount());

                for (size_t i = 0; i < materialSlots.GetSlotsCount(); ++i)
                {
                    AZ_TracePrintf(
                        "PhysXMaterialConversion", "  Slot %zu '%.*s') Legacy material id '%s' -> material asset '%s'.\n", i + 1,
                        AZ_STRING_ARG(materialSlots.GetSlotName(i)),
                        legacyMaterialSelection.m_materialIdsAssignedToSlots[i].m_id.ToString<AZStd::string>().c_str(),
                        materialSlots.GetMaterialAsset(i).GetHint().c_str());
                }
            }

            return true;
        }
        return false;
    };

    bool FixPhysicsMaterialId(
        Physics::Utils::PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& component,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap,
        const AZStd::vector<AZStd::string>& oldMemberChain,
        const AZStd::vector<AZStd::string>& newMemberChain)
    {
        PhysicsLegacy::MaterialId legacyMaterialId;
        if (Physics::Utils::LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialId>(oldMemberChain, component, legacyMaterialId))
        {
            AZ::Data::Asset<Physics::MaterialAsset> materialAsset =
                Physics::Utils::ConvertLegacyMaterialIdToMaterialAsset(legacyMaterialId, legacyMaterialIdToNewAssetIdMap);

            if (!materialAsset.GetId().IsValid())
            {
                return false;
            }

            if (!Physics::Utils::StoreObjectToPrefabComponent<AZ::Data::Asset<Physics::MaterialAsset>>(
                    newMemberChain, prefabInfo.m_template->GetPrefabDom(), component, materialAsset))
            {
                AZ_Warning(
                    "PhysXMaterialConversion", false, "Unable to set physics material asset to prefab '%s'.",
                    prefabInfo.m_prefabFullPath.c_str());
                return false;
            }

            // Remove legacy material id field
            Physics::Utils::RemoveMemberChainInPrefabComponent(oldMemberChain, component);

            AZ_TracePrintf(
                "PhysXMaterialConversion", "Legacy material id '%s' will be replaced by physics material asset '%s'.\n",
                legacyMaterialId.m_id.ToString<AZStd::string>().c_str(), materialAsset.GetHint().c_str());

            return true;
        }
        return false;
    };

    bool FixTerrainPhysicsColliderMaterials(
        Physics::Utils::PrefabInfo& prefabInfo,
        AzToolsFramework::Prefab::PrefabDomValue& component,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        bool modifiedTerrainPrefab = false;

        // Fix terrain default material.
        // It has a legacy material selection, but because it only needs 1 material the new version has one new material asset,
        // instead of material slots. So in this particular case it has convert the first legacy material id of the material
        // selection to a new material asset.
        PhysicsLegacy::MaterialSelection legacyDefaultMaterialSelection;
        if (Physics::Utils::LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(
                { "Configuration", "DefaultMaterial" }, component, legacyDefaultMaterialSelection))
        {
            PhysicsLegacy::MaterialId legacyMaterialId;
            if (!legacyDefaultMaterialSelection.m_materialIdsAssignedToSlots.empty())
            {
                legacyMaterialId = legacyDefaultMaterialSelection.m_materialIdsAssignedToSlots[0];
            }

            AZ::Data::Asset<Physics::MaterialAsset> materialAsset =
                Physics::Utils::ConvertLegacyMaterialIdToMaterialAsset(legacyMaterialId, legacyMaterialIdToNewAssetIdMap);

            if (materialAsset.GetId().IsValid())
            {
                if (Physics::Utils::StoreObjectToPrefabComponent<AZ::Data::Asset<Physics::MaterialAsset>>(
                        { "Configuration", "DefaultMaterialAsset" }, prefabInfo.m_template->GetPrefabDom(), component, materialAsset))
                {
                    // Remove legacy material selection field
                    Physics::Utils::RemoveMemberChainInPrefabComponent({ "Configuration", "DefaultMaterial" }, component);

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
        if (auto* mappingMember = Physics::Utils::FindMemberChainInPrefabComponent({ "Configuration", "Mappings" }, component); mappingMember != nullptr)
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

    void FixPrefabPhysicsMaterials(
        Physics::Utils::PrefabInfo& prefabInfo,
        const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        bool prefabModified = false;
        for (auto* entity : Physics::Utils::GetPrefabEntities(prefabInfo.m_template->GetPrefabDom()))
        {
            for (auto* component : Physics::Utils::GetEntityComponents(*entity))
            {
                const AZ::TypeId componentTypeId = Physics::Utils::GetComponentTypeId(*component);

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
                else if (componentTypeId == EditorWhiteBoxColliderComponentTypeId)
                {
                    if (FixPhysicsMaterialSelection(prefabInfo, *component, legacyMaterialIdToNewAssetIdMap,
                        { "Configuration", "MaterialSelection" },
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
                    // that is why 'prefabInfo' is captured as a copy.
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

    void FixPrefabsWithPhysicsLegacyMaterials(const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
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

        AZStd::vector<Physics::Utils::PrefabInfo> prefabs = Physics::Utils::CollectPrefabs();
        if (prefabs.empty())
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No prefabs found.\n");
            AZ_TracePrintf("PhysXMaterialConversion", "\n");
            return;
        }
        AZ_TracePrintf("PhysXMaterialConversion", "Found %zu prefabs to check.\n", prefabs.size());
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        for (auto& prefab : prefabs)
        {
            FixPrefabPhysicsMaterials(prefab, legacyMaterialIdToNewAssetIdMap);
        }

        AZ_TracePrintf("PhysXMaterialConversion", "Prefab conversion finished.\n");
        AZ_TracePrintf("PhysXMaterialConversion", "\n");
    }
} // namespace PhysX
