/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

//Forward decl of AZ::RandomDistributionType
namespace AZ
{
    enum class RandomDistributionType : AZ::u32;
}

namespace LmbrCentral
{
    inline constexpr AZ::TypeId EditorRandomTimedSpawnerComponentTypeId{ "{6D3E32F0-1971-416B-86DE-4B5EB6E2139E}" };
    inline constexpr AZ::TypeId RandomTimedSpawnerComponentTypeId{ "{8EE9EC2C-1CC9-4F88-968F-CFD20C380694}" };

    // Request bus for the RandomTimedSpawnerComponent
    class RandomTimedSpawnerComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this Ebus)
        // Using Defaults

        virtual ~RandomTimedSpawnerComponentRequests() {}
        
        //! Starts random spawning
        virtual void Enable() = 0;
       
        //! Stops random spawning
        virtual void Disable() = 0;
        
        //! Toggles random spawning
        virtual void Toggle() = 0;
        /**
         * Gets whether or not the component is spawning
         *
         * @return True if the component is spawning
         */
        virtual bool IsEnabled() = 0;
        
        /**
         * Sets the type of random distribution to use when determining where to 
         * place spawned entities.
         *
         * @randomDistribution The mathematical random distribution to use for spawning
         */
        virtual void SetRandomDistribution(AZ::RandomDistributionType randomDistribution) = 0;
        /**
         * Gets the type of random distribution used for determining 
         * entity spawn location.
         *
         * @return The AZ::RandomDistributionType currently in use for spawning
         */
        virtual AZ::RandomDistributionType GetRandomDistribution() = 0;
        
        /**
         * Sets the time (in seconds) between entity spawns
         *
         * @param spawnDelay The time between entity spawns
         */
        virtual void SetSpawnDelay(double spawnDelay) = 0;
        /**
         * Gets the time (in seconds) between entity spawns
         *
         * @return The time between entity spawns
         */
        virtual double GetSpawnDelay() = 0;

        /**
         * Sets the random variation in spawn delay (time in seconds)
         *
         * This value is used to calculate a random value that is added
         * onto the SpawnDelay parameter. The range of random values is
         * between -SpawnDelayVariation and SpawnDelayVariation.
         *
         * @param spawnDelayVariation The random variation applied to the spawn delay
         */
        virtual void SetSpawnDelayVariation(double spawnDelayVariation) = 0;
        /**
         * Gets the random variation in spawn delay (time in seconds)
         *
         * @return The random variation in spawn delay 
         */
        virtual double GetSpawnDelayVariation() = 0;
    };

    using RandomTimedSpawnerComponentRequestBus = AZ::EBus<RandomTimedSpawnerComponentRequests>;

} //namespace LmbrCentral
