/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzToolsFramework/Prefab/PrefabPublicRequestBus.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabInMemorySpawnableConverter.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabPublicInterface;

        class PrefabPublicRequestHandler final
            : public PrefabPublicRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabPublicRequestHandler, AZ::SystemAllocator);
            AZ_RTTI(PrefabPublicRequestHandler, "{83FBDDF9-10BE-4373-B1DC-44B47EE4805C}");

            static void Reflect(AZ::ReflectContext* context);

            void Connect();
            void Disconnect();

            CreatePrefabResult CreatePrefabInMemory(const EntityIdList& entityIds, AZStd::string_view filePath) override;
            InstantiatePrefabResult InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position) override;
            PrefabOperationResult DeleteEntitiesAndAllDescendantsInInstance(const EntityIdList& entityIds) override;
            PrefabOperationResult DetachPrefab(const AZ::EntityId& containerEntityId) override;
            PrefabOperationResult DetachPrefabAndRemoveContainerEntity(const AZ::EntityId& containerEntityId) override;
            DuplicatePrefabResult DuplicateEntitiesInInstance(const EntityIdList& entityIds) override;
            AZStd::string GetOwningInstancePrefabPath(AZ::EntityId entityId) const override;
            CreateSpawnableResult CreateInMemorySpawnableAsset(AZStd::string_view prefabFilePath, AZStd::string_view spawnableName) override;
            PrefabOperationResult RemoveInMemorySpawnableAsset(AZStd::string_view spawnableName) override;
            bool HasInMemorySpawnableAsset(AZStd::string_view spawnableName) const override;
            AZ::Data::AssetId GetInMemorySpawnableAssetId(AZStd::string_view spawnableName) const override;
            void RemoveAllInMemorySpawnableAssets() override;

        private:
            bool TryActivateSpawnableAssetContainer();

            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabInMemorySpawnableConverter m_spawnableAssetContainer;
            PrefabPublicInterface* m_prefabPublicInterface = nullptr;

        };
    } // namespace Prefab
} // namespace AzToolsFramework
