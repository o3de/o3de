/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableMetaData.h>
#include <AzFramework/Spawnable/SpawnableSystemComponent.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptMediator.h>

namespace AzFramework
{
    void SpawnableSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        Spawnable::Reflect(context);
        SpawnableMetaData::Reflect(context);
        EntitySpawnTicket::Reflect(context);
        Scripts::SpawnableScriptMediator::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<SpawnableSystemComponent, AZ::Component>();
            serializeContext->RegisterGenericType<AZ::Data::Asset<Spawnable>>();
        }
    }

    void SpawnableSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SpawnableSystemService"));
    }

    void SpawnableSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SpawnableSystemService"));
    }

    void SpawnableSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AssetDatabaseService"));
        services.push_back(AZ_CRC_CE("AssetCatalogService"));
    }

    void SpawnableSystemComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        ProcessSpawnableQueue();
        RootSpawnableNotificationBus::ExecuteQueuedEvents();
    }

    int SpawnableSystemComponent::GetTickOrder()
    {
        return AZ::ComponentTickBus::TICK_GAME;
    }

    void SpawnableSystemComponent::OnSystemTick()
    {
        // Handle only high priority spawning events such as those created from network. These need to happen even if the client
        // doesn't have focus to avoid time-out issues for instance.
        m_entitiesManager.ProcessQueue(SpawnableEntitiesManager::CommandQueuePriority::High);
    }

    uint64_t SpawnableSystemComponent::AssignRootSpawnable(AZ::Data::Asset<Spawnable> rootSpawnable)
    {
        uint32_t generation = 0;

        if (m_rootSpawnableId == rootSpawnable.GetId())
        {
            AZ_TracePrintf("Spawnables", "Root spawnable wasn't updated because it's already assigned to the requested asset.");
            return m_rootSpawnableContainer.GetCurrentGeneration();
        }

        if (rootSpawnable.QueueLoad())
        {
            m_rootSpawnableId = rootSpawnable.GetId();

            // Suspend and resume processing in the container that completion calls aren't received until
            // everything has been setup to accept callbacks from the call.
            m_rootSpawnableContainer.Reset(rootSpawnable);
            generation = m_rootSpawnableContainer.GetCurrentGeneration();

            // Don't send out the alert that the root spawnable has been assigned until the spawnable itself is ready. The common
            // use case is for handlers to do something with the information in the spawnable before the entities get spawned.
            m_rootSpawnableContainer.Alert(
                [rootSpawnable](uint32_t generation)
                {
                    RootSpawnableNotificationBus::Broadcast(
                        &RootSpawnableNotificationBus::Events::OnRootSpawnableAssigned, AZStd::move(rootSpawnable), generation);
                }, SpawnableEntitiesContainer::CheckIfSpawnableIsLoaded::Yes);
            m_rootSpawnableContainer.SpawnAllEntities();

            #if defined(AZ_ENABLE_TRACING)
                // Save the root spawnable's name before the compiler eats the rootSpawnable variable from AZStd::move(rootSpawnable)
                AZStd::string rootSpawnableName = rootSpawnable.GetHint();
            #endif

            m_rootSpawnableContainer.Alert(
                [newSpawnable = AZStd::move(rootSpawnable)](uint32_t generation)
                {
                    RootSpawnableNotificationBus::QueueBroadcast(
                        &RootSpawnableNotificationBus::Events::OnRootSpawnableReady, AZStd::move(newSpawnable), generation);
                });

            AZ_TracePrintf("Spawnables", "Root spawnable set to '%s' at generation %zu.\n", rootSpawnableName.c_str(), generation);     
        }
        else
        {
            AZ_Error("Spawnables", false, "Unable to queue root spawnable '%s' for loading.", rootSpawnable.GetHint().c_str());
        }

        return generation;
    }

    void SpawnableSystemComponent::ReleaseRootSpawnable()
    {
        if (m_rootSpawnableContainer.IsSet())
        {
            m_rootSpawnableContainer.Alert(
                [](uint32_t generation)
                {
                    RootSpawnableNotificationBus::QueueBroadcast(&RootSpawnableNotificationBus::Events::OnRootSpawnableReleased, generation);
                });
            m_rootSpawnableContainer.Clear();
        }
        m_rootSpawnableId = AZ::Data::AssetId();
    }

    void SpawnableSystemComponent::ProcessSpawnableQueue()
    {
        m_entitiesManager.ProcessQueue(
            SpawnableEntitiesManager::CommandQueuePriority::High | SpawnableEntitiesManager::CommandQueuePriority::Regular);
    }

    void SpawnableSystemComponent::ProcessSpawnableQueueUntilEmpty()
    {
        SpawnableEntitiesManager::CommandQueueStatus queueStatus;
        do
        {
            queueStatus = m_entitiesManager.ProcessQueue(
                SpawnableEntitiesManager::CommandQueuePriority::High | SpawnableEntitiesManager::CommandQueuePriority::Regular);
        } while (queueStatus == SpawnableEntitiesManager::CommandQueueStatus::HasCommandsLeft);
    }

    void SpawnableSystemComponent::OnRootSpawnableAssigned(
        [[maybe_unused]] AZ::Data::Asset<Spawnable> rootSpawnable,
        [[maybe_unused]] uint32_t generation)
    {
        AZ_TracePrintf("Spawnables", "New root spawnable '%s' assigned (generation: %i).\n", rootSpawnable.GetHint().c_str(), generation);
    }

    void SpawnableSystemComponent::OnRootSpawnableReady(
        [[maybe_unused]] AZ::Data::Asset<Spawnable> rootSpawnable, [[maybe_unused]] uint32_t generation)
    {
        AZ_TracePrintf("Spawnables", "Entities from new root spawnable '%s' are ready (generation: %i).\n", rootSpawnable.GetHint().c_str(), generation);
    }

    void SpawnableSystemComponent::OnRootSpawnableReleased([[maybe_unused]] uint32_t generation)
    {
        AZ_TracePrintf("Spawnables", "Generation %i of the root spawnable has been released.\n", generation);
    }

    void SpawnableSystemComponent::Activate()
    {
        // Register with AssetDatabase
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Spawnables can't be registered because the Asset Manager is not ready yet.");
        AZ::Data::AssetManager::Instance().RegisterHandler(&m_assetHandler, AZ::AzTypeInfo<Spawnable>::Uuid());

        // Register with AssetCatalog
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnableCatalogForAsset, AZ::AzTypeInfo<Spawnable>::Uuid());
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::AddExtension, Spawnable::FileExtension);

        // Register for the CriticalAssetsCompiled lifecycle event to trigger the loading of the root spawnable
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ_Assert(settingsRegistry, "Unable to change root spawnable callback because Settings Registry is not available.");

        auto LifecycleCallback = [this](const AZ::SettingsRegistryInterface::NotifyEventArgs&)
        {
            LoadRootSpawnableFromSettingsRegistry();
        };
        AZ::ComponentApplicationLifecycle::RegisterHandler(*settingsRegistry, m_criticalAssetsHandler,
            AZStd::move(LifecycleCallback), "CriticalAssetsCompiled");


        RootSpawnableNotificationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        m_registryChangeHandler = settingsRegistry->RegisterNotifier([this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
            {
                if (notifyEventArgs.m_jsonKeyPath.starts_with(RootSpawnableRegistryKey))
                {
                    LoadRootSpawnableFromSettingsRegistry();
                }
            });
    }

    void SpawnableSystemComponent::Deactivate()
    {
        ProcessSpawnableQueue();

        m_registryChangeHandler.Disconnect();

        AZ::TickBus::Handler::BusDisconnect();
        RootSpawnableNotificationBus::Handler::BusDisconnect();

        // Unregister Lifecycle event handler
        m_criticalAssetsHandler = {};

        if (m_rootSpawnableId.IsValid())
        {
            ReleaseRootSpawnable();

            // The SpawnableSystemComponent needs to guarantee there's no more processing left to do by the
            // entity manager before it can safely destroy it on shutdown, but also to make sure that are no
            // more calls to the callback registered to the root spawnable as that accesses this component.
            m_rootSpawnableContainer.Clear();
            ProcessSpawnableQueueUntilEmpty();
        }

        AZ::Data::AssetManager::Instance().UnregisterHandler(&m_assetHandler);
    }

    void SpawnableSystemComponent::LoadRootSpawnableFromSettingsRegistry()
    {
        auto registry = AZ::SettingsRegistry::Get();
        AZ_Assert(registry, "Unable to check for root spawnable because the Settings Registry is not available.");

        AZ::SettingsRegistryInterface::Type rootSpawnableKeyType = registry->GetType(RootSpawnableRegistryKey);
        if (rootSpawnableKeyType == AZ::SettingsRegistryInterface::Type::Object)
        {
            AZ::Data::Asset<Spawnable> rootSpawnable;
            if (registry->GetObject(rootSpawnable, RootSpawnableRegistryKey) && rootSpawnable.GetId().IsValid())
            {
                AssignRootSpawnable(AZStd::move(rootSpawnable));
            }
            else
            {
                AZ_Warning("Spawnables", false, "Root spawnable couldn't be queued for loading");
                ReleaseRootSpawnable();
            }
        }
        else if (rootSpawnableKeyType == AZ::SettingsRegistryInterface::Type::String)
        {
            AZStd::string rootSpawnableName;
            if (registry->Get(rootSpawnableName, RootSpawnableRegistryKey))
            {
                AZ::Data::AssetId rootSpawnableId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    rootSpawnableId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, rootSpawnableName.c_str(),
                    azrtti_typeid<Spawnable>(), false);
                if (rootSpawnableId.IsValid())
                {
                    AZ::Data::Asset<Spawnable> rootSpawnable = AZ::Data::Asset<Spawnable>(rootSpawnableId, azrtti_typeid<Spawnable>());
                    if (rootSpawnable.GetId().IsValid())
                    {
                        AssignRootSpawnable(AZStd::move(rootSpawnable));
                    }
                    else
                    {
                        AZ_Warning(
                            "Spawnables", false, "Root spawnable at '%s' couldn't be queued for loading.", rootSpawnableName.c_str());
                        ReleaseRootSpawnable();
                    }
                }
                else
                {
                    AZ_Warning(
                        "Spawnables", false, "Root spawnable with name '%s' wasn't found in the asset catalog.", rootSpawnableName.c_str());
                    ReleaseRootSpawnable();
                }
            }
        }
        else if (rootSpawnableKeyType == AZ::SettingsRegistryInterface::Type::NoType)
        {
            // [LYN-4146] - temporarily disabled
            /*AZ_Warning(
                "Spawnables", false,
                "No root spawnable assigned. The root spawnable can be assigned in the Settings Registry under the key '%s'.\n",
                RootSpawnableRegistryKey);*/
            ReleaseRootSpawnable();
        }
    }
} // namespace AzFramework
