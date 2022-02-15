
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetSerializer.h>

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/SpawnAssistant/SpawnAssistantBus.h>

namespace AzFramework
{
    struct SpawnData final
    {
        AZ_TYPE_INFO(SpawnData, "{3D9BE49C-94D5-416B-89C5-29313641B3E0}");

        static void Reflect(AZ::ReflectContext* context);

        void OnSpawnAssetChanged();

        AZStd::string m_name;
        AZ::Data::Asset<Spawnable> m_prefab;
    };

    class SpawnAssistantComponent
        : public AZ::Component
        , public SpawnAssistantRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SpawnAssistantComponent, "{2448BFF3-F26D-44FE-8938-3D7EF3BC52AC}");

        static void Reflect(AZ::ReflectContext* context);

        SpawnAssistantComponent();
        ~SpawnAssistantComponent();

        // SpawnAssistantRequests::Handler overrides ...
        EntitySpawnTicket::Id Spawn(AZStd::string_view prefabName) override;
        void Despawn(EntitySpawnTicket::Id id) override;
        void DespawnEverything() override;


    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
        

    private:
        bool m_active{};
        AZStd::vector<SpawnData> m_spawnData;
        AZStd::unordered_map<EntitySpawnTicket::Id, EntitySpawnTicket*> m_ticketCache;
    };
} // namespace AzFramework
