/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RandomTimedSpawnerComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Scripting/SpawnerComponentBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Time/ITime.h>

namespace LmbrCentral
{
    void RandomTimedSpawnerConfiguration::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RandomTimedSpawnerConfiguration>()
                ->Version(1)
                ->Field("Enabled", &RandomTimedSpawnerConfiguration::m_enabled)
                ->Field("RandomDistribution", &RandomTimedSpawnerConfiguration::m_randomDistribution)
                ->Field("SpawnDelay", &RandomTimedSpawnerConfiguration::m_spawnDelay)
                ->Field("SpawnDelayVariation", &RandomTimedSpawnerConfiguration::m_spawnDelayVariation)
                ;
        }

        bool usePrefabSystem = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(usePrefabSystem, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
        if (!usePrefabSystem)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext
                    ->EBus<RandomTimedSpawnerComponentRequestBus>("RandomTimedSpawnerRequestBus")

                    ->Event("Enable", &RandomTimedSpawnerComponentRequestBus::Events::Enable)
                    ->Event("Disable", &RandomTimedSpawnerComponentRequestBus::Events::Disable)
                    ->Event("Toggle", &RandomTimedSpawnerComponentRequestBus::Events::Toggle)
                    ->Event("IsEnabled", &RandomTimedSpawnerComponentRequestBus::Events::IsEnabled)

                    ->Event("SetRandomDistribution", &RandomTimedSpawnerComponentRequestBus::Events::SetRandomDistribution)
                    ->Event("GetRandomDistribution", &RandomTimedSpawnerComponentRequestBus::Events::GetRandomDistribution)
                    ->VirtualProperty("RandomDistribution", "GetRandomDistribution", "SetRandomDistribution")

                    ->Event("SetSpawnDelay", &RandomTimedSpawnerComponentRequestBus::Events::SetSpawnDelay)
                    ->Event("GetSpawnDelay", &RandomTimedSpawnerComponentRequestBus::Events::GetSpawnDelay)
                    ->VirtualProperty("SpawnDelay", "GetSpawnDelay", "SetSpawnDelay")

                    ->Event("SetSpawnDelayVariation", &RandomTimedSpawnerComponentRequestBus::Events::SetSpawnDelayVariation)
                    ->Event("GetSpawnDelayVariation", &RandomTimedSpawnerComponentRequestBus::Events::GetSpawnDelayVariation)
                    ->VirtualProperty("SpawnDelayVariation", "GetSpawnDelayVariation", "SetSpawnDelayVariation");

                behaviorContext->Class<RandomTimedSpawnerComponent>()->RequestBus("RandomTimedSpawnerRequestBus");
            }
        }
    }

    void RandomTimedSpawnerComponent::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RandomTimedSpawnerComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &RandomTimedSpawnerComponent::m_config)
                ;
        }

        RandomTimedSpawnerConfiguration::Reflect(context);
    }

    void RandomTimedSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("RandomTimedSpawnerService", 0x56f2fa36));
    }
    void RandomTimedSpawnerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        //Only compatible with Box and Cylinder shapes
        incompatible.push_back(AZ_CRC("CapsuleShapeService", 0x9bc1122c));
        incompatible.push_back(AZ_CRC("SphereShapeService", 0x90c8dc80));
        incompatible.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
    }
    void RandomTimedSpawnerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        required.push_back(AZ_CRC("SpawnerService", 0xd2f1d7a3));
    }

    void RandomTimedSpawnerComponent::Activate()
    {
        const AZ::TimeUs elapsedTimeUs = AZ::GetElapsedTimeUs();
        m_currentTime = AZ::TimeUsToSecondsDouble(elapsedTimeUs);
        RandomTimedSpawnerComponentRequestBus::Handler::BusConnect(GetEntityId());

        CalculateNextSpawnTime();

        if (m_config.m_enabled)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void RandomTimedSpawnerComponent::Deactivate()
    {
        if (m_config.m_enabled)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }

        RandomTimedSpawnerComponentRequestBus::Handler::BusDisconnect();
    }

    void RandomTimedSpawnerComponent::OnTick([[maybe_unused]] float deltaTime, AZ::ScriptTimePoint time)
    {
        m_currentTime = time.GetSeconds();

        if (m_currentTime >= m_nextSpawnTime)
        {
            AZ::Transform spawnTransform = AZ::Transform::CreateIdentity();
            spawnTransform.SetTranslation(CalculateNextSpawnPosition());

            LmbrCentral::SpawnerComponentRequestBus::Event(GetEntityId(), &LmbrCentral::SpawnerComponentRequestBus::Events::SpawnAbsolute, spawnTransform);

            CalculateNextSpawnTime();
        }
    }

    void RandomTimedSpawnerComponent::Enable()
    {
        m_config.m_enabled = true;
        AZ::TickBus::Handler::BusConnect();
    }
    void RandomTimedSpawnerComponent::Disable()
    {
        m_config.m_enabled = false;
        AZ::TickBus::Handler::BusDisconnect();
    }
    void RandomTimedSpawnerComponent::Toggle()
    {
        m_config.m_enabled = !m_config.m_enabled;
        if (m_config.m_enabled)
        {
            AZ::TickBus::Handler::BusConnect();
        }
        else
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void RandomTimedSpawnerComponent::CalculateNextSpawnTime()
    {
        m_randomDistribution = std::uniform_real_distribution<double>(-m_config.m_spawnDelayVariation, m_config.m_spawnDelayVariation);
        double variation = m_randomDistribution(m_randomEngine);
        double spawnDelay = m_config.m_spawnDelay + variation;

        m_nextSpawnTime = m_currentTime + spawnDelay;
    }

    AZ::Vector3 RandomTimedSpawnerComponent::CalculateNextSpawnPosition()
    {
        AZ::Vector3 spawnPos = AZ::Vector3::CreateZero();

        //This spawnPos is untransformed
        LmbrCentral::ShapeComponentRequestsBus::EventResult(spawnPos, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, m_config.m_randomDistribution);

        return spawnPos;
    }

} //namespace LmbrCentral
