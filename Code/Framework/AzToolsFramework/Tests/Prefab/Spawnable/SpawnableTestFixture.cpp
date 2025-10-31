/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>
#include <Prefab/Spawnable/SpawnableTestFixture.h>

namespace UnitTest
{
    AZStd::unique_ptr<AZ::Entity> SpawnableTestFixture::CreateEntity(const char* entityName, AZ::EntityId parentId)
    {
        auto result = AZStd::make_unique<AZ::Entity>(entityName);
        auto transformComponent = result->CreateComponent<AzFramework::TransformComponent>();
        transformComponent->SetParent(parentId);
        
        return result;
    }

    AZ::Data::Asset<AzFramework::Spawnable> SpawnableTestFixture::CreateSpawnableAsset(uint64_t entityCount)
    {
        // The life cycle of a spawnable is managed through the asset.
        AzFramework::Spawnable* spawnable = new AzFramework::Spawnable(
            AZ::Data::AssetId::CreateString("{612F2AB1-30DF-44BB-AFBE-17A85199F09E}:0"), AZ::Data::AssetData::AssetStatus::Ready);

        AzFramework::Spawnable::EntityList& entities = spawnable->GetEntities();
        entities.reserve(entityCount);
        for (uint64_t i = 0; i < entityCount; i++)
        {
            entities.emplace_back(CreateEntity("Entity"));
        }

        return AZ::Data::Asset<AzFramework::Spawnable>(spawnable, AZ::Data::AssetLoadBehavior::Default);
    }
} // namespace UnitTest
