/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabPublicRequestHandler.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzToolsFramework/Prefab/PrefabLoader.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConversionPipeline.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConverterStackProfileNames.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void PrefabPublicRequestHandler::Reflect(AZ::ReflectContext* context)
        {
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<PrefabPublicRequestBus>("PrefabPublicRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                    ->Attribute(AZ::Script::Attributes::Module, "prefab")
                    ->Event("CreatePrefabInMemory", &PrefabPublicRequests::CreatePrefabInMemory)
                    ->Event("InstantiatePrefab", &PrefabPublicRequests::InstantiatePrefab)
                    ->Event("DeleteEntitiesAndAllDescendantsInInstance", &PrefabPublicRequests::DeleteEntitiesAndAllDescendantsInInstance)
                    ->Event("DetachPrefab", &PrefabPublicRequests::DetachPrefab)
                    ->Event("DetachPrefabAndRemoveContainerEntity", &PrefabPublicRequests::DetachPrefabAndRemoveContainerEntity)
                    ->Event("DuplicateEntitiesInInstance", &PrefabPublicRequests::DuplicateEntitiesInInstance)
                    ->Event("GetOwningInstancePrefabPath", &PrefabPublicRequests::GetOwningInstancePrefabPath)
                    ->Event("CreateInMemorySpawnableAsset", &PrefabPublicRequests::CreateInMemorySpawnableAsset)
                    ->Event("RemoveInMemorySpawnableAsset", &PrefabPublicRequests::RemoveInMemorySpawnableAsset)
                    ->Event("HasInMemorySpawnableAsset", &PrefabPublicRequests::HasInMemorySpawnableAsset)
                    ->Event("GetInMemorySpawnableAssetId", &PrefabPublicRequests::GetInMemorySpawnableAssetId)
                    ->Event("RemoveAllInMemorySpawnableAssets", &PrefabPublicRequests::RemoveAllInMemorySpawnableAssets)
                    ;
            }
        }

        void PrefabPublicRequestHandler::Connect()
        {
            m_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();
            AZ_Assert(m_prefabPublicInterface, "PrefabPublicRequestHandler - Could not retrieve instance of PrefabPublicInterface");

            PrefabPublicRequestBus::Handler::BusConnect();
        }

        void PrefabPublicRequestHandler::Disconnect()
        {
            PrefabPublicRequestBus::Handler::BusDisconnect();
            m_spawnableAssetContainer.Deactivate();
            m_prefabPublicInterface = nullptr;
        }

        CreatePrefabResult PrefabPublicRequestHandler::CreatePrefabInMemory(const EntityIdList& entityIds, AZStd::string_view filePath)
        {
            return m_prefabPublicInterface->CreatePrefabInMemory(entityIds, filePath);
        }

        InstantiatePrefabResult PrefabPublicRequestHandler::InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position)
        {
            return m_prefabPublicInterface->InstantiatePrefab(filePath, parent, position);
        }

        PrefabOperationResult PrefabPublicRequestHandler::DeleteEntitiesAndAllDescendantsInInstance(const EntityIdList& entityIds)
        {
            return m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(entityIds);
        }

        PrefabOperationResult PrefabPublicRequestHandler::DetachPrefab(const AZ::EntityId& containerEntityId)
        {
            return m_prefabPublicInterface->DetachPrefab(containerEntityId);
        }

        PrefabOperationResult PrefabPublicRequestHandler::DetachPrefabAndRemoveContainerEntity(const AZ::EntityId& containerEntityId)
        {
            return m_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(containerEntityId);
        }

        DuplicatePrefabResult PrefabPublicRequestHandler::DuplicateEntitiesInInstance(const EntityIdList& entityIds)
        {
            return m_prefabPublicInterface->DuplicateEntitiesInInstance(entityIds);
        }

        AZStd::string PrefabPublicRequestHandler::GetOwningInstancePrefabPath(AZ::EntityId entityId) const
        {
            return m_prefabPublicInterface->GetOwningInstancePrefabPath(entityId).Native();
        }

        bool PrefabPublicRequestHandler::TryActivateSpawnableAssetContainer()
        {
            bool activated = m_spawnableAssetContainer.IsActivated();
            if (!activated)
            {
                activated = m_spawnableAssetContainer.Activate(PrefabConversionUtils::IntegrationTests);
            }

            return activated;
        }

        CreateSpawnableResult PrefabPublicRequestHandler::CreateInMemorySpawnableAsset(AZStd::string_view prefabFilePath, AZStd::string_view spawnableName)
        {
            if (!TryActivateSpawnableAssetContainer())
            {
                return AZ::Failure(AZStd::string("Failed to activate Spawnable Asset Container"));
            }

            auto result = m_spawnableAssetContainer.CreateInMemorySpawnableAsset(prefabFilePath, spawnableName);
            if (result.IsSuccess())
            {
                return AZ::Success(result.GetValue().get().GetId());
            }
            else
            {
                return AZ::Failure(result.TakeError());
            }
        }

        PrefabOperationResult PrefabPublicRequestHandler::RemoveInMemorySpawnableAsset(AZStd::string_view spawnableName)
        {
            auto result = m_spawnableAssetContainer.GetAssetContainer().RemoveInMemorySpawnableAsset(spawnableName);
            if (result.IsSuccess())
            {
                return AZ::Success();
            }
            else
            {
                return AZ::Failure(result.TakeError());
            }
        }

        bool PrefabPublicRequestHandler::HasInMemorySpawnableAsset(AZStd::string_view spawnableName) const
        {
            return m_spawnableAssetContainer.GetAssetContainerConst().HasInMemorySpawnableAsset(spawnableName);
        }

        AZ::Data::AssetId PrefabPublicRequestHandler::GetInMemorySpawnableAssetId(AZStd::string_view spawnableName) const
        {
            return m_spawnableAssetContainer.GetAssetContainerConst().GetInMemorySpawnableAssetId(spawnableName);
        }

        void PrefabPublicRequestHandler::RemoveAllInMemorySpawnableAssets()
        {
            m_spawnableAssetContainer.GetAssetContainer().ClearAllInMemorySpawnableAssets();
        }

    } // namespace Prefab
} // namespace AzToolsFramework
