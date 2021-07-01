/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    SpawnNodeable::SpawnNodeable(const SpawnNodeable& rhs)
        : m_spawnableAsset(rhs.m_spawnableAsset)
    {}

    SpawnNodeable& SpawnNodeable::operator=(SpawnNodeable& rhs)
    {
        m_spawnableAsset = rhs.m_spawnableAsset;
        return *this;
    }

    void SpawnNodeable::OnInitializeExecutionState()
    {
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }

        m_spawnTicket = AzFramework::EntitySpawnTicket(m_spawnableAsset);
    }

    void SpawnNodeable::OnDeactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();

        m_spawnTicket = AzFramework::EntitySpawnTicket();
    }

    void SpawnNodeable::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        AZStd::vector<Data::EntityIDType> swappedSpawnedEntityList;
        AZStd::vector<size_t> swappedSpawnBatchSizes;
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_idBatchMutex);

            swappedSpawnedEntityList.swap(m_spawnedEntityList);
            swappedSpawnBatchSizes.swap(m_spawnBatchSizes);
        }

        AZ::EntityId* batchBegin = swappedSpawnedEntityList.data();
        for (size_t batchSize : swappedSpawnBatchSizes)
        {
            if (batchSize == 0)
            {
                continue;
            }

            AZStd::vector<AZ::EntityId> spawnedEntitiesBatch(
                batchBegin, batchBegin + batchSize);

            CallOnSpawn(AZStd::move(spawnedEntitiesBatch));

            batchBegin += batchSize;
        }
    }

    void SpawnNodeable::OnSpawnAssetChanged()
    {
        if (m_spawnableAsset.GetId().IsValid())
        {
            AZStd::string rootSpawnableFile;
            AzFramework::StringFunc::Path::GetFileName(m_spawnableAsset.GetHint().c_str(), rootSpawnableFile);

            rootSpawnableFile += AzFramework::Spawnable::DotFileExtension;

            AZ::u32 rootSubId = AzFramework::SpawnableAssetHandler::BuildSubId(AZStd::move(rootSpawnableFile));

            if (m_spawnableAsset.GetId().m_subId != rootSubId)
            {
                AZ::Data::AssetId rootAssetId = m_spawnableAsset.GetId();
                rootAssetId.m_subId = rootSubId;

                m_spawnableAsset = AZ::Data::AssetManager::Instance().
                    FindOrCreateAsset<AzFramework::Spawnable>(rootAssetId, AZ::Data::AssetLoadBehavior::Default);
            }
            else
            {
                m_spawnableAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::Default);
            }
        }
    }

    void SpawnNodeable::RequestSpawn(Data::Vector3Type translation, Data::Vector3Type rotation, Data::NumberType scale)
    {
        if (m_spawnableAsset.GetAutoLoadBehavior() == AZ::Data::AssetLoadBehavior::NoLoad)
        {
            return;
        }

        auto preSpawnCB = [this, translation, rotation, scale]([[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
            AzFramework::SpawnableEntityContainerView view)
        {
            AZ::Entity* rootEntity = *view.begin();

            AzFramework::TransformComponent* entityTransform =
                rootEntity->FindComponent<AzFramework::TransformComponent>();

            if (entityTransform)
            {
                AZ::Vector3 rotationCopy = rotation;
                AZ::Quaternion rotationQuat = AZ::Quaternion::CreateFromEulerAnglesDegrees(rotationCopy);

                entityTransform->SetWorldTM(AZ::Transform(translation, rotationQuat, scale));
            }
        };

        auto spawnCompleteCB = [this]([[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
            AzFramework::SpawnableConstEntityContainerView view)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_idBatchMutex);
            m_spawnedEntityList.reserve(m_spawnedEntityList.size() + view.size());
            for (const AZ::Entity* entity : view)
            {
                m_spawnedEntityList.emplace_back(entity->GetId());
            }
            m_spawnBatchSizes.push_back(view.size());
        };

        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_preInsertionCallback = AZStd::move(preSpawnCB);
        optionalArgs.m_completionCallback = AZStd::move(spawnCompleteCB);
        AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(m_spawnTicket, AZStd::move(optionalArgs));
    }
}
