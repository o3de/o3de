/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityBus.h>
#include <AzFramework/Slice/SliceInstantiationBus.h>
#include <LmbrCentral/Scripting/SpawnerComponentBus.h>

namespace LmbrCentral
{
    /**
    * SpawnerComponent
    *
    * SpawnerComponent facilitates spawning of a design-time selected or run-time provided "*.dynamicslice" at an entity's location with an optional offset.
    */
    class SpawnerComponent
        : public AZ::Component
        , private SpawnerComponentRequestBus::Handler
        , private AzFramework::SliceInstantiationResultBus::MultiHandler
        , private AZ::EntityBus::MultiHandler
        , private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_COMPONENT(SpawnerComponent, SpawnerComponentTypeId);

        SpawnerComponent();
        SpawnerComponent(const AZ::Data::Asset<AZ::DynamicSliceAsset>& sliceAsset, bool spawnOnActivate);
        ~SpawnerComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor
        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        bool ReadInConfig(const AZ::ComponentConfig* spawnerConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outSpawnerConfig) const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SpawnerComponentRequestBus::Handler
        void SetDynamicSlice(const AZ::Data::Asset<AZ::DynamicSliceAsset>& dynamicSliceAsset) override;
        void SetDynamicSliceByAssetId(AZ::Data::AssetId& assetId) override;
        void SetSpawnOnActivate(bool spawnOnActivate) override;
        bool GetSpawnOnActivate() override;
        AzFramework::SliceInstantiationTicket Spawn() override;
        AzFramework::SliceInstantiationTicket SpawnRelative(const AZ::Transform& relative) override;
        AzFramework::SliceInstantiationTicket SpawnAbsolute(const AZ::Transform& world) override;
        AzFramework::SliceInstantiationTicket SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice) override;
        AzFramework::SliceInstantiationTicket SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative) override;
        AzFramework::SliceInstantiationTicket SpawnSliceAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world) override;
        void DestroySpawnedSlice(const AzFramework::SliceInstantiationTicket& ticket) override;
        void DestroyAllSpawnedSlices() override;
        AZStd::vector<AzFramework::SliceInstantiationTicket> GetCurrentlySpawnedSlices() override;
        bool HasAnyCurrentlySpawnedSlices() override;
        AZStd::vector<AZ::EntityId> GetCurrentEntitiesFromSpawnedSlice(const AzFramework::SliceInstantiationTicket& ticket) override;
        AZStd::vector<AZ::EntityId> GetAllCurrentlySpawnedEntities() override;
        bool IsReadyToSpawn() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SliceInstantiationResultBus::MultiHandler
        void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiationFailedOrCanceled(const AZ::Data::AssetId& sliceAssetId, bool canceled) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EntityBus::MultiHandler
        void OnEntityDestruction(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Serialized members
        AZ::Data::Asset<AZ::DynamicSliceAsset> m_sliceAsset;
        bool m_spawnOnActivate = false;
        bool m_destroyOnDeactivate = false;

    private:

        //////////////////////////////////////////////////////////////////////////
        // Private helpers
        AzFramework::SliceInstantiationTicket SpawnSliceInternalAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world);
        AzFramework::SliceInstantiationTicket SpawnSliceInternalRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Runtime-only members
        AZStd::vector<AzFramework::SliceInstantiationTicket> m_activeTickets; ///< tickets listed in order they were spawned
        AZStd::unordered_map<AZ::EntityId, AzFramework::SliceInstantiationTicket> m_entityToTicketMap; ///< map from entity to ticket that spawned it
        AZStd::unordered_map<AzFramework::SliceInstantiationTicket, AZStd::unordered_set<AZ::EntityId>> m_ticketToEntitiesMap; ///< map from ticket to entities it spawned
    };
} // namespace LmbrCentral
