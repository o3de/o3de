/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <LmbrCentral/Scripting/RandomTimedSpawnerComponentBus.h>

#include "RandomTimedSpawnerComponent.h"

namespace LmbrCentral
{
    class EditorRandomTimedSpawnerConfiguration
        : public RandomTimedSpawnerConfiguration
    {
    public:
        AZ_TYPE_INFO(EditorRandomTimedSpawnerConfiguration, "{AA68F544-917B-4F72-AEA7-3A906B9DEB2B}");
        AZ_CLASS_ALLOCATOR(EditorRandomTimedSpawnerConfiguration, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
    };

    class EditorRandomTimedSpawnerComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public RandomTimedSpawnerComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EditorRandomTimedSpawnerComponent, "{6D3E32F0-1971-416B-86DE-4B5EB6E2139E}", AzToolsFramework::Components::EditorComponentBase);
        
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // RandomTimedSpawnerComponentRequestBus
        void Enable() override { m_config.m_enabled = true; }
        void Disable() override { m_config.m_enabled = false; }
        void Toggle() override { m_config.m_enabled = !m_config.m_enabled; }
        bool IsEnabled() override { return m_config.m_enabled; }

        void SetRandomDistribution(AZ::RandomDistributionType randomDistribution) override { m_config.m_randomDistribution = randomDistribution; }
        AZ::RandomDistributionType GetRandomDistribution() override { return m_config.m_randomDistribution; }

        void SetSpawnDelay(double spawnDelay) override { m_config.m_spawnDelay = spawnDelay; }
        double GetSpawnDelay() override { return m_config.m_spawnDelay; }

        void SetSpawnDelayVariation(double spawnDelayVariation) override { m_config.m_spawnDelayVariation = spawnDelayVariation; }
        double GetSpawnDelayVariation() override { return m_config.m_spawnDelayVariation; }

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        //Reflected members
        EditorRandomTimedSpawnerConfiguration m_config;
    };

} //namespace LmbrCentral
