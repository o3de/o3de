
#pragma once

#include <AzCore/Component/Component.h>
#include <Multiplayer/IMultiplayerSpawner.h>

namespace Multiplayer
{
    class DefaultMultiplayerSpawnerComponent
        : public AZ::Component
        , public IMultiplayerSpawner
    {
    public:
        AZ_COMPONENT(Multiplayer::DefaultMultiplayerSpawnerComponent, "{0A6D0132-3FD2-4F13-B537-2B1DA99E34E9}");

        /*
        * Reflects component data into the reflection contexts, including the serialization, edit, and behavior contexts.
        */
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:

        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // IMultiplayerSpawner interface implementation
        NetworkEntityHandle OnPlayerJoin(uint64_t userId, const MultiplayerAgentDatum& agentDatum) override;
        void OnPlayerLeave(ConstNetworkEntityHandle entityHandle, const ReplicationSet& replicationSet, AzNetworking::DisconnectReason reason) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        NetworkSpawnable m_playerSpawnable;
        AZStd::vector<AZ::EntityId> m_spawnPoints;

        uint32_t m_spawnIndex = 0;
    };
} // namespace Multiplayer
