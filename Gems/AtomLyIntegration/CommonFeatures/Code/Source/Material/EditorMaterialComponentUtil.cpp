/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/EditorMaterialComponentUtil.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/std/ranges/elements_view.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentUtil
        {
            bool LoadMaterialEditDataFromAssetId(const AZ::Data::AssetId& assetId, MaterialEditData& editData)
            {
                editData = MaterialEditData();

                if (!assetId.IsValid())
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentUtil", false, "Attempted to load material data for invalid asset id.");
                    return false;
                }

                editData.m_materialAssetId = assetId;

                // Load the originating product asset from which the new source has set will be generated
                auto materialAssetOutcome = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::MaterialAsset>(editData.m_materialAssetId);
                if (!materialAssetOutcome)
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentUtil", false, "Failed to load material asset: %s", editData.m_materialAssetId.ToString<AZStd::string>().c_str());
                    return false;
                }

                editData.m_materialAsset = materialAssetOutcome.GetValue();
                editData.m_materialTypeAsset = editData.m_materialAsset->GetMaterialTypeAsset();
                editData.m_materialParentAsset = {};

                editData.m_materialSourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(editData.m_materialAsset.GetId());
                if (AzFramework::StringFunc::Path::IsExtension(
                        editData.m_materialSourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension))
                {
                    if (!AZ::RPI::JsonUtils::LoadObjectFromFile(editData.m_materialSourcePath, editData.m_materialSourceData))
                    {
                        AZ_Error("AZ::Render::EditorMaterialComponentUtil", false, "Material source data could not be loaded: '%s'.", editData.m_materialSourcePath.c_str());
                        return false;
                    }
                }

                if (!editData.m_materialSourceData.m_parentMaterial.empty())
                {
                    // There is a parent for this material
                    auto parentMaterialResult = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::MaterialAsset>(editData.m_materialSourcePath, editData.m_materialSourceData.m_parentMaterial);
                    if (!parentMaterialResult)
                    {
                        AZ_Error("AZ::Render::EditorMaterialComponentUtil", false, "Parent material asset could not be loaded: '%s'.", editData.m_materialSourceData.m_parentMaterial.c_str());
                        return false;
                    }

                    editData.m_materialParentAsset = parentMaterialResult.GetValue();
                    editData.m_materialParentSourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(editData.m_materialParentAsset.GetId());
                }

                // We need a valid path to the material type source data to get the property layout and assign to the new material
                editData.m_materialTypeSourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(editData.m_materialTypeAsset.GetId());
                if (editData.m_materialTypeSourcePath.empty())
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentUtil", false, "Failed to locate source material type asset: %s", editData.m_materialAssetId.ToString<AZStd::string>().c_str());
                    return false;
                }

                // With the introduction of the material pipeline, abstract material types, and intermediate assets, the material could
                // be referencing a generated material type in the intermediate asset folder. We need to map back to the original
                // material type.
                editData.m_originalMaterialTypeSourcePath =
                    AZ::RPI::MaterialUtils::PredictOriginalMaterialTypeSourcePath(editData.m_materialTypeSourcePath);
                if (editData.m_originalMaterialTypeSourcePath.empty())
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentUtil", false, "Build to locate originating source material type asset: %s", editData.m_materialAssetId.ToString<AZStd::string>().c_str());
                    return false;
                }

                // Load the material type source data
                auto materialTypeOutcome = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(editData.m_materialTypeSourcePath);
                if (!materialTypeOutcome.IsSuccess())
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentUtil", false, "Failed to load material type source data: %s", editData.m_materialTypeSourcePath.c_str());
                    return false;
                }
                editData.m_materialTypeSourceData = materialTypeOutcome.TakeValue();
                return true;
            }

            bool SaveSourceMaterialFromEditData(const AZStd::string& path, const MaterialEditData& editData)
            {
                if (path.empty() || !editData.m_materialAsset.IsReady() || !editData.m_materialTypeAsset.IsReady() ||
                    editData.m_materialTypeSourcePath.empty() || editData.m_originalMaterialTypeSourcePath.empty())
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentUtil", false, "Can not export: %s", path.c_str());
                    return false;
                }

                // Construct the material source data object that will be exported
                AZ::RPI::MaterialSourceData exportData;
                exportData.m_materialTypeVersion = editData.m_materialTypeAsset->GetVersion();

                // Source material files that should reference the originating source material type instead of the potential intermediate
                // material type asset.
                exportData.m_materialType = AtomToolsFramework::GetPathToExteralReference(path, editData.m_originalMaterialTypeSourcePath);
                exportData.m_parentMaterial = AtomToolsFramework::GetPathToExteralReference(path, editData.m_materialParentSourcePath);

                // Copy all of the properties from the material asset to the source data that will be exported
                bool result = true;
                editData.m_materialTypeSourceData.EnumerateProperties([&](const AZ::RPI::MaterialPropertySourceData* propertyDefinition, const AZ::RPI::MaterialNameContext& nameContext)
                    {
                        AZ::Name propertyId{propertyDefinition->GetName()};
                        nameContext.ContextualizeProperty(propertyId);

                        const AZ::RPI::MaterialPropertyIndex propertyIndex =
                            editData.m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyId);

                        AZ::RPI::MaterialPropertyValue propertyValue =
                            editData.m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()];

                        AZ::RPI::MaterialPropertyValue propertyValueDefault = propertyDefinition->m_value;
                        if (editData.m_materialParentAsset.IsReady())
                        {
                            propertyValueDefault = editData.m_materialParentAsset->GetPropertyValues()[propertyIndex.GetIndex()];
                        }

                        // Check for and apply any property overrides before saving property values
                        auto propertyOverrideItr = editData.m_materialPropertyOverrideMap.find(propertyId);
                        if (propertyOverrideItr != editData.m_materialPropertyOverrideMap.end())
                        {
                            propertyValue = AZ::RPI::MaterialPropertyValue::FromAny(propertyOverrideItr->second);
                        }

                        if (!AtomToolsFramework::ConvertToExportFormat(path, propertyId, *propertyDefinition, propertyValue))
                        {
                            AZ_Error("AZ::Render::EditorMaterialComponentUtil", false, "Failed to export: %s", path.c_str());
                            result = false;
                            return false;
                        }

                        // Don't export values if they are the same as the material type or parent
                        if (propertyValueDefault == propertyValue)
                        {
                            return true;
                        }

                        exportData.SetPropertyValue(propertyId, propertyValue);
                        return true;
                    });

                return result && AZ::RPI::JsonUtils::SaveObjectToFile(path, exportData);
            }

            AZ::Data::AssetId GetMaterialTypeAssetIdFromMaterialAssetId(const AZ::Data::AssetId& materialAssetId)
            {
                if (materialAssetId.IsValid())
                {
                    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> dependencyResult = AZ::Failure(AZStd::string());
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        dependencyResult, &AZ::Data::AssetCatalogRequestBus::Events::GetAllProductDependencies, materialAssetId);
                    if (dependencyResult)
                    {
                        for (const auto& dependency : dependencyResult.GetValue())
                        {
                            AZ::Data::AssetInfo info;
                            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                                info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, dependency.m_assetId);
                            if (info.m_assetType == azrtti_typeid<AZ::RPI::MaterialTypeAsset>())
                            {
                                // Immediately return the first material type that's encountered because the material system currently
                                // supports only one material type for any hierarchy of materials.
                                return info.m_assetId;
                            }
                        }
                    }
                }

                return {};
            }

            bool DoEntitiesHaveMatchingMaterialTypes(
                const AZ::EntityId& primaryEntityId,
                const AzToolsFramework::EntityIdSet& secondaryEntityIds,
                const MaterialAssignmentId& materialAssignmentId)
            {
                AZ::Data::AssetId primaryMaterialAssetId = {};
                MaterialComponentRequestBus::EventResult(
                    primaryMaterialAssetId, primaryEntityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId,
                    materialAssignmentId);
                AZ::Data::AssetId primaryMaterialTypeAssetId = GetMaterialTypeAssetIdFromMaterialAssetId(primaryMaterialAssetId);

                return primaryMaterialTypeAssetId.IsValid() && AZStd::all_of(
                    secondaryEntityIds.begin(), secondaryEntityIds.end(),
                    [&](const AZ::EntityId& secondaryEntityId)
                    {
                        AZ::Data::AssetId secondaryMaterialAssetId = {};
                        MaterialComponentRequestBus::EventResult(
                            secondaryMaterialAssetId, secondaryEntityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId,
                            materialAssignmentId);
                        AZ::Data::AssetId secondaryMaterialTypeAssetId = GetMaterialTypeAssetIdFromMaterialAssetId(secondaryMaterialAssetId);
                        return primaryMaterialTypeAssetId == secondaryMaterialTypeAssetId;
                    });
            }

            bool DoEntitiesHaveMatchingMaterials(
                const AZ::EntityId& primaryEntityId,
                const AzToolsFramework::EntityIdSet& secondaryEntityIds,
                const MaterialAssignmentId& materialAssignmentId)
            {
                AZ::Data::AssetId primaryMaterialAssetId = {};
                MaterialComponentRequestBus::EventResult(
                    primaryMaterialAssetId, primaryEntityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId,
                    materialAssignmentId);

                return primaryMaterialAssetId.IsValid() && AZStd::all_of(
                    secondaryEntityIds.begin(), secondaryEntityIds.end(),
                    [&](const AZ::EntityId& secondaryEntityId)
                    {
                        AZ::Data::AssetId secondaryMaterialAssetId = {};
                        MaterialComponentRequestBus::EventResult(
                            secondaryMaterialAssetId, secondaryEntityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId,
                            materialAssignmentId);
                        return primaryMaterialAssetId == secondaryMaterialAssetId;
                    });
            }

            bool DoEntitiesHaveMatchingMaterialSlots(
                const AZ::EntityId& primaryEntityId, const AzToolsFramework::EntityIdSet& secondaryEntityIds)
            {
                MaterialAssignmentMap primaryMaterialSlots;
                MaterialComponentRequestBus::EventResult(
                    primaryMaterialSlots, primaryEntityId, &MaterialComponentRequestBus::Events::GetDefaultMaterialMap);

                return AZStd::all_of(
                    secondaryEntityIds.begin(), secondaryEntityIds.end(),
                    [&](const AZ::EntityId& secondaryEntityId)
                    {
                        MaterialAssignmentMap secondaryMaterialSlots;
                        MaterialComponentRequestBus::EventResult(
                            secondaryMaterialSlots, secondaryEntityId,
                            &MaterialComponentRequestBus::Events::GetDefaultMaterialMap);

                        if (primaryMaterialSlots.size() != secondaryMaterialSlots.size())
                        {
                            return false;
                        }

                        for (const auto& slotPair : primaryMaterialSlots)
                        {
                            const auto& slotItr = secondaryMaterialSlots.find(slotPair.first);
                            if (slotItr == secondaryMaterialSlots.end())
                            {
                                return false;
                            }
                        }
                        return true;
                    });
            }

            AzToolsFramework::EntityIdSet GetSelectedEntitiesFromActiveInspector()
            {
                AzToolsFramework::EntityIdList entityIds;
                AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
                    &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedAndPinnedEntities, entityIds);
                return AzToolsFramework::EntityIdSet(entityIds.begin(), entityIds.end());
            }

            AzToolsFramework::EntityIdSet GetEntitiesMatchingMaterialSlots(
                const AZ::EntityId& primaryEntityId, const AzToolsFramework::EntityIdSet& secondaryEntityIds)
            {
                AzToolsFramework::EntityIdSet entityIds = secondaryEntityIds;

                AZStd::erase_if(
                    entityIds,
                    [&](const AZ::EntityId& secondaryEntityId)
                    {
                        return !DoEntitiesHaveMatchingMaterialSlots(primaryEntityId, { secondaryEntityId });
                    });

                return entityIds;
            }
        } // namespace EditorMaterialComponentUtil
    } // namespace Render
} // namespace AZ
