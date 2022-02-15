
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnAssistant/SpawnAssistantComponent.h>

namespace AzFramework
{
    void SpawnData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpawnData>()
                ->Version(0)
                ->Field("Name", &SpawnData::m_name)
                ->Field("Prefab", &SpawnData::m_prefab)
            ;

            
        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SpawnData>("SpawnData", "A wrapper around spawnable asset to be used as a variable in Script Canvas.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::Category, "Spawning")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Placeholder.png")
                    // m_name
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnData::m_name, "Name", "")
                    // m_prefab
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnData::m_prefab, "Prefab", "")
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
                    ->Attribute(AZ::Edit::Attributes::HideProductFilesInAssetPicker, true)
                    ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "m_prefab")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SpawnData::OnSpawnAssetChanged);
            }
        }

    }

    void SpawnData::OnSpawnAssetChanged()
    {
        if (m_prefab.GetId().IsValid())
        {
            AZStd::string rootSpawnableFile;
            AzFramework::StringFunc::Path::GetFileName(m_prefab.GetHint().c_str(), rootSpawnableFile);

            rootSpawnableFile += AzFramework::Spawnable::DotFileExtension;

            AZ::u32 rootSubId = AzFramework::SpawnableAssetHandler::BuildSubId(AZStd::move(rootSpawnableFile));

            if (m_prefab.GetId().m_subId != rootSubId)
            {
                AZ::Data::AssetId rootAssetId = m_prefab.GetId();
                rootAssetId.m_subId = rootSubId;

                m_prefab = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AzFramework::Spawnable>(
                    rootAssetId, AZ::Data::AssetLoadBehavior::Default);
            }
            else
            {
                m_prefab.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::Default);
            }
        }
    }

    void SpawnAssistantComponent::Reflect(AZ::ReflectContext* context)
    {
        SpawnData::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SpawnAssistantComponent, AZ::Component>()
                ->Version(0)
                ->Field("SpawnData", &SpawnAssistantComponent::m_spawnData);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SpawnAssistantComponent>("Spawn Assistant", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Spawning")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnAssistantComponent::m_spawnData, "Spawn Data", "")
                    ;
            }
        }
    }

    SpawnAssistantComponent::SpawnAssistantComponent()
    {
    }

    SpawnAssistantComponent::~SpawnAssistantComponent()
    {
    }

    void SpawnAssistantComponent::Init()
    {
    }

    void SpawnAssistantComponent::Activate()
    {
        SpawnAssistantRequestBus::Handler::BusConnect();
        m_active = true;
    }

    void SpawnAssistantComponent::Deactivate()
    {
        m_active = false;

        SpawnAssistantRequestBus::Handler::BusDisconnect();

        DespawnEverything();
    }

    EntitySpawnTicket::Id SpawnAssistantComponent::Spawn(AZStd::string_view prefabName)
    {
        auto itData = AZStd::find_if(
            m_spawnData.begin(), m_spawnData.end(),
            [&prefabName](const SpawnData& data) -> bool
            {
                return data.m_name == prefabName;
            });

        if (itData == m_spawnData.end())
        {
            return 0;
        }

        auto preSpawnCB =
            [this](EntitySpawnTicket::Id ticketId, SpawnableEntityContainerView view)
        {
            if (m_active)
            {
                AZStd::vector<AZ::EntityId> entityIdList;
                entityIdList.reserve(view.size());
                for (const AZ::Entity* entity : view)
                {
                    entityIdList.emplace_back(entity->GetId());
                }
                SpawnAssistantNotificationBus::Broadcast(&SpawnAssistantNotifications::BeforeSpawn, ticketId, AZStd::move(entityIdList));
            }
        };

        auto spawnCompleteCB =
            [this](EntitySpawnTicket::Id ticketId, SpawnableConstEntityContainerView view)
        {
            if (m_active)
            {
                AZStd::vector<AZ::EntityId> entityIdList;
                entityIdList.reserve(view.size());
                for (const AZ::Entity* entity : view)
                {
                    entityIdList.emplace_back(entity->GetId());
                }
                SpawnAssistantNotificationBus::Broadcast(&SpawnAssistantNotifications::BeforeSpawn, ticketId, AZStd::move(entityIdList));
            }
        };

        auto ticket = aznew EntitySpawnTicket(itData->m_prefab);
        SpawnAllEntitiesOptionalArgs spawnArgs;
        spawnArgs.m_preInsertionCallback = AZStd::move(preSpawnCB);
        spawnArgs.m_completionCallback = AZStd::move(spawnCompleteCB);
        SpawnableEntitiesInterface::Get()->SpawnAllEntities(*ticket, spawnArgs);
        m_ticketCache.emplace(ticket->GetId(), ticket);
        return ticket->GetId();
    }

    void SpawnAssistantComponent::Despawn(EntitySpawnTicket::Id id)
    {
        auto const& ticketIt = m_ticketCache.find(id);
        if (ticketIt == m_ticketCache.end())
        {
            AZ_Error("SpawnAssistantComponent", false, "Ticket not found in cache");
            return;
        }

        auto despawnCB = [this](EntitySpawnTicket::Id ticketId)
        {
            if (m_active)
            {
                m_ticketCache.erase(ticketId);
            }
        };

        DespawnAllEntitiesOptionalArgs despawnArgs;
        despawnArgs.m_completionCallback = AZStd::move(despawnCB);
        SpawnableEntitiesInterface::Get()->DespawnAllEntities(*ticketIt->second);

    }
    
    void SpawnAssistantComponent::DespawnEverything()
    {
        for (const auto& [ticketId, ticket] : m_ticketCache)
        {
            delete ticket;
        }
        m_ticketCache.clear();
    }
} // namespace Invaders
#pragma optimize("", on)
