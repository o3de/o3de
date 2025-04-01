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
#include <AzCore/Math/Transform.h>
#include <AzCore/Slice/SliceAsset.h>

#include <LmbrCentral/Scripting/RandomTimedSpawnerComponentBus.h>

#include <AzCore/Math/Random.h>
#include <random>

namespace AZ
{
    class ReflectContext;
}

namespace LmbrCentral
{
    /**
    * Configuration for the RandomTimedSpawnerComponent
    */
    class RandomTimedSpawnerConfiguration
    {
    public:
        AZ_TYPE_INFO(RandomTimedSpawnerConfiguration, "4133644F-FADA-4C82-A2A2-B587B20E81FA");

        static void Reflect(AZ::ReflectContext* context);

        bool m_enabled = true;

        AZ::RandomDistributionType m_randomDistribution = AZ::RandomDistributionType::UniformReal;

        double m_spawnDelay = 5.0;
        double m_spawnDelayVariation = 0.0;
    };

    /**
    * A component to spawn slices at regular intervals
    * at random points inside of a volume.
    */
    class RandomTimedSpawnerComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public RandomTimedSpawnerComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RandomTimedSpawnerComponent, RandomTimedSpawnerComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        RandomTimedSpawnerComponent() {}
        explicit RandomTimedSpawnerComponent(RandomTimedSpawnerConfiguration *params)
        {
            m_config = *params;
        }
        ~RandomTimedSpawnerComponent() {};

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // RandomTimedSpawnerRequestBus
        void Enable() override;
        void Disable() override;
        void Toggle() override;
        bool IsEnabled() override { return m_config.m_enabled; }

        void SetRandomDistribution(AZ::RandomDistributionType randomDistribution) override { m_config.m_randomDistribution = randomDistribution; }
        AZ::RandomDistributionType GetRandomDistribution() override { return m_config.m_randomDistribution; }

        void SetSpawnDelay(double spawnDelay) override { m_config.m_spawnDelay = spawnDelay; }
        double GetSpawnDelay() override { return m_config.m_spawnDelay; }

        void SetSpawnDelayVariation(double spawnDelayVariation) override { m_config.m_spawnDelayVariation = spawnDelayVariation; }
        double GetSpawnDelayVariation() override { return m_config.m_spawnDelayVariation; }

    private:
        //Reflected members
        RandomTimedSpawnerConfiguration m_config;

        //Unreflected members
        double m_currentTime;
        double m_nextSpawnTime;

        std::default_random_engine m_randomEngine;
        std::uniform_real_distribution<double> m_randomDistribution;

        void CalculateNextSpawnTime();
        AZ::Vector3 CalculateNextSpawnPosition();
    };
} //namespace LmbrCentral
