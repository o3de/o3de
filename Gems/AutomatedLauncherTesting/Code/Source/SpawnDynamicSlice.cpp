/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SpawnDynamicSlice.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <LmbrCentral/Scripting/SpawnerComponentBus.h>


namespace AutomatedLauncherTesting
{
    AZ::Entity* SpawnDynamicSlice::CreateSpawner(const AZStd::string& path, const AZStd::string& entityName)
    {
        AZ::Entity* spawnerEntity = nullptr;

        AZ::Data::AssetId sliceAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, path.c_str(), AZ::Data::s_invalidAssetType, false);
        if (sliceAssetId.IsValid())
        {
            AZ_Printf("System", "Spawning dynamic slide %s", path.c_str());
            spawnerEntity = aznew AZ::Entity(entityName.c_str());
            spawnerEntity->Init();

            LmbrCentral::SpawnerConfig spawnerConfig;

            AZ::Data::Asset<AZ::DynamicSliceAsset> sliceAssetData = AZ::Data::AssetManager::Instance().GetAsset<AZ::DynamicSliceAsset>(sliceAssetId, spawnerConfig.m_sliceAsset.GetAutoLoadBehavior());
            sliceAssetData.BlockUntilLoadComplete();

            AZ::Component* spawnerComponent = nullptr;
            AZ::ComponentDescriptorBus::EventResult(spawnerComponent, LmbrCentral::SpawnerComponentTypeId, &AZ::ComponentDescriptorBus::Events::CreateComponent);

            spawnerConfig.m_sliceAsset = sliceAssetData;
            spawnerConfig.m_spawnOnActivate = true;
            spawnerComponent->SetConfiguration(spawnerConfig);

            spawnerEntity->AddComponent(spawnerComponent);

            spawnerEntity->Activate();
        }
        else
        {
            AZ_Warning("System", false, "Could not create asset for dynamic slide %s", path.c_str());
        }

        return spawnerEntity;
    }

} // namespace AutomatedLauncherTesting
