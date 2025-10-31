/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/limits.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/Spawnable/EntityAliasTypes.h>

namespace AZ
{
    class Entity;
}

namespace AzToolsFramework::Prefab
{
    class Instance;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabProcessorContext;
}

namespace AzToolsFramework::Prefab::SpawnableUtils
{
    static constexpr uint32_t InvalidEntityIndex = AZStd::numeric_limits<uint32_t>::max();

    AZTF_API bool CreateSpawnable(AzFramework::Spawnable& spawnable, const PrefabDom& prefabDom);
    AZTF_API bool CreateSpawnable(AzFramework::Spawnable& spawnable, const PrefabDom& prefabDom, AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets);

    AZTF_API AZ::Entity* CreateEntityAlias(
        AZStd::string sourcePrefabName,
        AzToolsFramework::Prefab::Instance& source,
        AZStd::string targetPrefabName,
        AzToolsFramework::Prefab::Instance& target,
        AzToolsFramework::Prefab::Instance& parentInstance,
        AZ::EntityId entityId,
        AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasType aliasType,
        AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasSpawnableLoadBehavior loadBehavior,
        const AZ::Crc32& tag,
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context);
    
        AZTF_API uint32_t FindEntityIndex(AZ::EntityId entity, const AzFramework::Spawnable& spawnable);

        AZTF_API void SortEntitiesByTransformHierarchy(AzFramework::Spawnable& spawnable);

    template <typename EntityPtr>
    AZTF_API void SortEntitiesByTransformHierarchy(AZStd::vector<EntityPtr>& entities);

} // namespace AzToolsFramework::Prefab::SpawnableUtils
