/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Multiplayer/IMultiplayerSpawner.h>
#include <Multiplayer/Components/ISimplePlayerSpawner.h>
namespace Multiplayer
{
    /*!
     * \class SimplePlayerSpawnerComponent
     * \brief A network player spawner. Attach this component to any level's root entity which needs to spawn a network player.
     *        If no spawn points are provided the network players will be spawned at the world-space origin.
     */
    class SimplePlayerSpawnerComponent
        : public AZ::Component
        , public IMultiplayerSpawner
        , public ISimplePlayerSpawner
    {
    public:
        AZ_COMPONENT(Multiplayer::SimplePlayerSpawnerComponent, "{0A6D0132-3FD2-4F13-B537-2B1DA99E34E9}");

        /*
        * Reflects component data into the reflection contexts, including the serialization, edit, and behavior contexts.
        */
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////
        // ISimplePlayerSpawner implementation...
        AZ::Transform GetNextSpawnPoint() const override;
        const AZStd::vector<AZ::EntityId>& GetSpawnPoints() const override;
        uint32_t GetSpawnPointCount() const override;
        uint32_t GetNextSpawnPointIndex() const override;
        void SetNextSpawnPointIndex(uint32_t index) override;
        ////////////////////////////////////////////////////////////////////////

    protected:

        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // IMultiplayerSpawner interface implementation
        NetworkEntityHandle OnPlayerJoin(uint64_t userId, const MultiplayerAgentDatum& agentDatum) override;
        void OnPlayerLeave(ConstNetworkEntityHandle entityHandle, const ReplicationSet& replicationSet, AzNetworking::DisconnectReason reason) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        // Serialized properties
        NetworkSpawnable m_playerSpawnable;
        AZStd::vector<AZ::EntityId> m_spawnPoints;

        // Runtime properties
        uint32_t m_spawnIndex = 0;

        friend class SimplePlayerSpawnerTests;
    };
} // namespace Multiplayer
