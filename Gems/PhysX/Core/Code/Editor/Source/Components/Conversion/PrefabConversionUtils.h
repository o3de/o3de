/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Component/EntityIdSerializer.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace PhysX::Utils
{
    struct PrefabInfo
    {
        AzToolsFramework::Prefab::TemplateId m_templateId;
        AzToolsFramework::Prefab::Template* m_template = nullptr;
        AZStd::string m_prefabFullPath;
    };

    AZStd::vector<PrefabInfo> CollectPrefabs();

    void SavePrefab(PrefabInfo& prefabInfo);

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetPrefabEntities(AzToolsFramework::Prefab::PrefabDom& prefab);

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetEntityComponents(AzToolsFramework::Prefab::PrefabDomValue& entity);

    AZ::TypeId GetComponentTypeId(const AzToolsFramework::Prefab::PrefabDomValue& component);

    // Mapper to ensure the entity ids remain the same when loading and storing entities from a prefab.
    class PrefabEntityIdMapper final
        : public AZ::JsonEntityIdSerializer::JsonEntityIdMapper
    {
    public:
        AZ_RTTI(PrefabEntityIdMapper, "{CAA0D7E0-00B0-4B84-8480-A3475CE25043}", AZ::JsonEntityIdSerializer::JsonEntityIdMapper);

        PrefabEntityIdMapper() = default;

        AZ::JsonSerializationResult::Result MapJsonToId(
            AZ::EntityId& outputValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result MapIdToJson(
            rapidjson::Value& outputValue, [[maybe_unused]] const AZ::EntityId& inputValue, AZ::JsonSerializerContext& context) override;

    private:
        AZStd::unordered_map<AZ::EntityId, AZStd::string> m_entityIdMap;
    };

    bool LoadPrefabEntity(
        PrefabEntityIdMapper& prefabEntityIdMapper, const AzToolsFramework::Prefab::PrefabDomValue& prefabEntity, AZ::Entity& entity);

    bool StorePrefabEntity(
        const PrefabEntityIdMapper& prefabEntityIdMapper,
        AzToolsFramework::Prefab::PrefabDom& prefabDom,
        AzToolsFramework::Prefab::PrefabDomValue& prefabEntity,
        const AZ::Entity& entity);

} // namespace PhysX::Utils
