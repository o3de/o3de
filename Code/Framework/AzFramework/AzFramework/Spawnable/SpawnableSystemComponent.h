/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Spawnable/RootSpawnableInterface.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/Spawnable/SpawnableEntitiesContainer.h>
#include <AzFramework/Spawnable/SpawnableEntitiesManager.h>

namespace AzFramework
{
    class SpawnableSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AZ::SystemTickBus::Handler
        , public AssetCatalogEventBus::Handler
        , public RootSpawnableInterface::Registrar
        , public RootSpawnableNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(SpawnableSystemComponent, "{12D0DA52-BB86-4AC3-8862-9493E0D0E207}");

        inline static constexpr const char* RootSpawnableRegistryKey = "/Amazon/AzCore/Bootstrap/RootSpawnable";
            
        SpawnableSystemComponent() = default;
        SpawnableSystemComponent(const SpawnableSystemComponent&) = delete;
        SpawnableSystemComponent(SpawnableSystemComponent&&) = delete;

        SpawnableSystemComponent& operator=(const SpawnableSystemComponent&) = delete;
        SpawnableSystemComponent& operator=(SpawnableSystemComponent&&) = delete;

        //
        // Component
        //

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        //
        // TickBus
        //

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

        //
        // SystemTickBus
        //

        void OnSystemTick() override;

        //
        // AssetCatalogEventBus
        //

        void OnCatalogLoaded(const char* catalogFile) override;

        //
        // RootSpawnableInterface
        //

        uint64_t AssignRootSpawnable(AZ::Data::Asset<Spawnable> rootSpawnable) override;
        void ReleaseRootSpawnable() override;
        void ProcessSpawnableQueue() override;

        //
        // RootSpawnbleNotificationBus
        //

        void OnRootSpawnableAssigned(AZ::Data::Asset<Spawnable> rootSpawnable, uint32_t generation) override;
        void OnRootSpawnableReleased(uint32_t generation) override;
        
    protected:
        void Activate() override;
        void Deactivate() override;

        void LoadRootSpawnableFromSettingsRegistry();

        SpawnableAssetHandler m_assetHandler;
        SpawnableEntitiesManager m_entitiesManager;
        SpawnableEntitiesContainer m_rootSpawnableContainer;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_registryChangeHandler;

        AZ::Data::AssetId m_rootSpawnableId;
        bool m_catalogAvailable{ false };
    };
} // namespace AzFramework
