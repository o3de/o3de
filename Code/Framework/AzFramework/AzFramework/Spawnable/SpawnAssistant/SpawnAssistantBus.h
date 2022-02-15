#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace AzFramework
{
    class SpawnAssistantRequests
        : public AZ::EBusTraits
    {
    public:
        // Only a single handler is allowed
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //virtual void RegisterPrefab(SpawnData)
        virtual EntitySpawnTicket::Id Spawn(AZStd::string_view /*prefabName*/) = 0;
        virtual void Despawn(EntitySpawnTicket::Id /*id*/) = 0;
        virtual void DespawnEverything() = 0;
    };

    using SpawnAssistantRequestBus = AZ::EBus<SpawnAssistantRequests>;

    class SpawnAssistantNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual void BeforeSpawn(EntitySpawnTicket::Id /*id*/, AZStd::vector<AZ::EntityId> /*entityIdList*/) {}
        virtual void AfterSpawn(EntitySpawnTicket::Id /*id*/, AZStd::vector<AZ::EntityId> /*entityIdList*/) {}
    };

    using SpawnAssistantNotificationBus = AZ::EBus<SpawnAssistantNotifications>;
} // namespace AzFramework
