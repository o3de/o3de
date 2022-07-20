/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <AutomatedTesting/AutomatedTestingBus.h>
#include <Multiplayer/IMultiplayerSpawner.h>
#include <Source/Spawners/IPlayerSpawner.h>

namespace AzFramework
{
    struct PlayerConnectionConfig;
}

namespace Multiplayer
{
    struct EntityReplicationData;
    using ReplicationSet = AZStd::map<ConstNetworkEntityHandle, EntityReplicationData>;
    struct MultiplayerAgentDatum;
}

namespace AutomatedTesting
{
    class AutomatedTestingSystemComponent
        : public AZ::Component
        , public Multiplayer::IMultiplayerSpawner
        , protected AutomatedTestingRequestBus::Handler
    {
    public:
        AZ_COMPONENT(AutomatedTestingSystemComponent, "{81E31A03-5C09-41C5-BDF6-5E33456C7C2B}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AutomatedTestingRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // Multiplayer::IMultiplayerSpawner interface implementation
        Multiplayer::NetworkEntityHandle OnPlayerJoin(uint64_t userId, const Multiplayer::MultiplayerAgentDatum& agentDatum) override;
        void OnPlayerLeave(Multiplayer::ConstNetworkEntityHandle entityHandle, const Multiplayer::ReplicationSet& replicationSet, AzNetworking::DisconnectReason reason) override;
        ////////////////////////////////////////////////////////////////////////
        
        AZStd::unique_ptr<AutomatedTesting::IPlayerSpawner> m_playerSpawner;

    };
}
