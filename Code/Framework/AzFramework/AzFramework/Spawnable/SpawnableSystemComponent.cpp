/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Spawnable/SpawnableMetaData.h>
#include <AzFramework/Spawnable/SpawnableSystemComponent.h>

namespace AzFramework
{
    void SpawnableSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        Spawnable::Reflect(context);
        SpawnableMetaData::Reflect(context);

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

    void SpawnableSystemComponent::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        if (!m_rootSpawnableInitialized)
        {
            auto registry = AZ::SettingsRegistry::Get();
            AZ_Assert(registry, "Unable to check for root spawnable because the Settings Registry is not available.");
            if (registry->GetObject(m_rootSpawnable, RootSpawnableRegistryKey) && m_rootSpawnable.GetId().IsValid())
            {
                AZ_TracePrintf("Spawnables", "Root spawnable '%s' used.\n", m_rootSpawnable.GetHint().c_str());
                if (!m_rootSpawnable.QueueLoad())
                {
                    AZ_Error("Spawnables", false, "Unable to queue root spawnable for loading.\n");
                }
            }
            else
            {
                AZ_Warning("Spawnables", false, "No root spawnable assigned or root spawanble couldnt' be loaded.\n"
                    "The root spawnable can be assigned in the Settings Registry under the key '%s'.\n", RootSpawnableRegistryKey);
            }

            m_rootSpawnableInitialized = true;
        }
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

        AssetCatalogEventBus::Handler::BusConnect();
    }

    void SpawnableSystemComponent::Deactivate()
    {
        AssetCatalogEventBus::Handler::BusDisconnect();

        AZ_Assert(AZ::Data::AssetManager::IsReady(),
            "Spawnables can't be unregistered because the Asset Manager has been destroyed already or never started.");
        AZ::Data::AssetManager::Instance().UnregisterHandler(&m_assetHandler);
    }
} // namespace AzFramework
