/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Editor/Source/Components/Conversion/PrefabConversionUtils.h>

namespace PhysX::Utils
{
    static AZStd::optional<AZStd::string> GetFullSourceAssetPathById(AZ::Data::AssetId assetId)
    {
        AZStd::string assetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);
        AZ_Assert(!assetPath.empty(), "Asset Catalog returned an invalid path from an enumerated asset.");
        if (assetPath.empty())
        {
            AZ_Warning(
                "PhysXPrefabUtils",
                false,
                "Not able to get asset path for asset with id %s.",
                assetId.ToString<AZStd::string>().c_str());
            return AZStd::nullopt;
        }

        AZStd::string assetFullPath;
        bool assetFullPathFound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            assetFullPathFound,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath,
            assetPath,
            assetFullPath);
        if (!assetFullPathFound)
        {
            AZ_Warning("PhysXPrefabUtils", false, "Source file of asset '%s' could not be found.", assetPath.c_str());
            return AZStd::nullopt;
        }

        return { AZStd::move(assetFullPath) };
    }

    AZStd::vector<PrefabInfo> CollectPrefabs()
    {
        AZStd::vector<PrefabInfo> prefabs;

        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&prefabs, prefabLoader, prefabSystemComponent](
                const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
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
                    prefabs.push_back({ templateId, &templateRef, AZStd::move(*assetFullPath) });
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, /*beginCB*/nullptr, assetEnumerationCB, /*endCB*/nullptr);

        return prefabs;
    }

    void SavePrefab(PrefabInfo& prefabInfo)
    {
        auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

        prefabInfo.m_template->MarkAsDirty(true);
        prefabSystemComponent->PropagateTemplateChanges(prefabInfo.m_templateId);

        // Request source control to edit prefab file
        AzToolsFramework::SourceControlCommandBus::Broadcast(
            &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
            prefabInfo.m_prefabFullPath.c_str(),
            /*allowMultiCheckout*/true,
            [prefabInfo]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info)
            {
                // This is called from the main thread on the next frame from TickBus,
                // that is why 'prefabInfo' is captured as a copy.
                if (!info.IsReadOnly())
                {
                    auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                    if (!prefabLoader->SaveTemplate(prefabInfo.m_templateId))
                    {
                        AZ_Warning("PhysXPrefabUtils", false, "Unable to save prefab '%s'", prefabInfo.m_prefabFullPath.c_str());
                    }
                }
                else
                {
                    AZ_Warning(
                        "PhysXPrefabUtils",
                        false,
                        "Unable to check out asset '%s' in source control.",
                        prefabInfo.m_prefabFullPath.c_str());
                }
            });
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

    AZ::JsonSerializationResult::Result PrefabEntityIdMapper::MapJsonToId(
        AZ::EntityId& outputValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        if (!inputValue.IsString())
        {
            return context.Report(
                AZ::JsonSerializationResult::Tasks::ReadField,
                AZ::JsonSerializationResult::Outcomes::TypeMismatch,
                "Unexpected json type for prefab id, expected a String type.");
        }

        const size_t entityIdHash = AZStd::hash<AZStd::string_view>()(inputValue.GetString());
        outputValue = AZ::EntityId(entityIdHash);
        m_entityIdMap[outputValue] = inputValue.GetString();

        return context.Report(
            AZ::JsonSerializationResult::Tasks::ReadField,
            AZ::JsonSerializationResult::Outcomes::Success,
            "Successfully mapped string id to entity id.");
    }

    AZ::JsonSerializationResult::Result PrefabEntityIdMapper::MapIdToJson(
        rapidjson::Value& outputValue, const AZ::EntityId& inputValue, AZ::JsonSerializerContext& context)
    {
        auto it = m_entityIdMap.find(inputValue);
        if (it == m_entityIdMap.end())
        {
            return context.Report(
                AZ::JsonSerializationResult::Tasks::WriteValue,
                AZ::JsonSerializationResult::Outcomes::Missing,
                "Missing entity id in the map.");
        }

        outputValue.SetString(it->second.c_str(), static_cast<rapidjson_ly::SizeType> (it->second.size()), context.GetJsonAllocator());

        return context.Report(
            AZ::JsonSerializationResult::Tasks::WriteValue,
            AZ::JsonSerializationResult::Outcomes::Success,
            "Successfully mapped entity id to string id.");
    }

    bool LoadPrefabEntity(
        PrefabEntityIdMapper& prefabEntityIdMapper, const AzToolsFramework::Prefab::PrefabDomValue& prefabEntity, AZ::Entity& entity)
    {
        AZ::JsonDeserializerSettings settings;
        settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&prefabEntityIdMapper));

        auto result = AZ::JsonSerialization::Load(&entity, azrtti_typeid<AZ::Entity>(), prefabEntity, settings);

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }

    bool StorePrefabEntity(
        const PrefabEntityIdMapper& prefabEntityIdMapper,
        AzToolsFramework::Prefab::PrefabDom& prefabDom,
        AzToolsFramework::Prefab::PrefabDomValue& prefabEntity,
        const AZ::Entity& entity)
    {
        AZ::JsonSerializerSettings settings;
        settings.m_metadata.Add(static_cast<const AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&prefabEntityIdMapper));

        auto result = AZ::JsonSerialization::Store(
            prefabEntity, prefabDom.GetAllocator(), &entity, /*defaultObject*/nullptr, azrtti_typeid<AZ::Entity>(), settings);

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }
} // namespace PhysX::Utils
