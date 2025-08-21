/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <string>
#include "particle/core/Particle.h"
#include "particle/core/ParticleDelegate.h"

namespace SimuCore::ParticleCore {
    struct Particle;
    class ParticlePool;

    class ParticleEmitEffector {
    public:
        using EmitterFunc = void(uint8_t*, const EmitInfo&, EmitSpawnParam&);
        using DistUpdateFunc = void(uint8_t*, const Distribution&);
        using EmitterDelegate = ParticleDelegate<EmitterFunc>;
        using DistUpdateDelegate = ParticleDelegate<DistUpdateFunc>;

        template<typename Effector>
        explicit ParticleEmitEffector(DataArgs<Effector>&&)
        {
            using DataType = typename Effector::DataType;
            fn.template Connect<&Effector::Execute, DataType>();
            distFn.template Connect<&Effector::UpdateDistPtr, DataType>();
            name = TypeInfo<Effector>::Name();
        }

        ~ParticleEmitEffector() = default;

        void Execute(uint8_t* data, const EmitInfo& info, EmitSpawnParam& spawnParam) const
        {
            fn(data, info, spawnParam);
        }

        void Update(uint8_t* data, const Distribution& distribution) const
        {
            distFn(data, distribution);
        }

        uint32_t DataSize() const
        {
            return fn.size;
        }

        const std::string& Name() const
        {
            return name;
        }

    private:
        EmitterDelegate fn;
        DistUpdateDelegate distFn;
        std::string name;
    };

    class ParticleSpawnEffector {
    public:
        using SpawnFunc = void(uint8_t*, const SpawnInfo&, Particle&);
        using DistUpdateFunc = void(uint8_t*, const Distribution&);
        using SpawnDelegate = ParticleDelegate<SpawnFunc>;
        using DistUpdateDelegate = ParticleDelegate<DistUpdateFunc>;

        template<typename Effector>
        explicit ParticleSpawnEffector(DataArgs<Effector>&&)
        {
            using DataType = typename Effector::DataType;
            fn.template Connect<&Effector::Execute, DataType>();
            distFn.template Connect<&Effector::UpdateDistPtr, DataType>();
            name = TypeInfo<Effector>::Name();
        }

        ~ParticleSpawnEffector() = default;

        void Execute(uint8_t* data, const SpawnInfo& info, Particle& particle) const
        {
            fn(data, info, particle);
        }

        void Update(uint8_t* data, const Distribution& distribution) const
        {
            distFn(data, distribution);
        }

        uint32_t DataSize() const
        {
            return fn.size;
        }

        std::string Name() const
        {
            return name;
        }

    private:
        SpawnDelegate fn;
        DistUpdateDelegate distFn;
        std::string name;
    };

    class ParticleUpdateEffector {
    public:
        using UpdateFunc = void(uint8_t*, const UpdateInfo&, Particle&);
        using DistUpdateFunc = void(uint8_t*, const Distribution&);
        using UpdateDelegate = ParticleDelegate<UpdateFunc>;
        using DistUpdateDelegate = ParticleDelegate<DistUpdateFunc>;

        template<typename Effector>
        explicit ParticleUpdateEffector(DataArgs<Effector>&&)
        {
            using DataType = typename Effector::DataType;
            fn.template Connect<&Effector::Execute, DataType>();
            distFn.template Connect<&Effector::UpdateDistPtr, DataType>();
            name = TypeInfo<Effector>::Name();
        }

        ~ParticleUpdateEffector() = default;

        void Execute(uint8_t* data, const UpdateInfo& info, Particle& particle) const
        {
            fn(data, info, particle);
        }

        void Update(uint8_t* data, const Distribution& distribution) const
        {
            distFn(data, distribution);
        }

        uint32_t DataSize() const
        {
            return fn.size;
        }

        std::string Name() const
        {
            return name;
        }

    private:
        UpdateDelegate fn;
        DistUpdateDelegate distFn;
        std::string name;
    };

    class ParticleEventEffector {
    public:
        using EventFunc = void(uint8_t*, const EventInfo&);
        using EventDelegate = ParticleDelegate<EventFunc>;

        template<typename Effector>
        explicit ParticleEventEffector(DataArgs<Effector>&&)
        {
            using DataType = typename Effector::DataType;
            fn.template Connect<&Effector::Execute, DataType>();
            name = TypeInfo<Effector>::Name();
        }

        ~ParticleEventEffector() = default;

        void Execute(uint8_t* data, const EventInfo& info) const
        {
            fn(data, info);
        }

        uint32_t DataSize() const
        {
            return fn.size;
        }

        const std::string& Name() const
        {
            return name;
        }

    private:
        EventDelegate fn;
        std::string name;
    };
}
