/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include "particle/core/Particle.h"
#include "particle/core/ParticleDelegate.h"

namespace SimuCore::ParticleCore {
    struct Particle;
    class ParticlePool;

    class ParticleEmitEffector {
    public:
        using EmitterFunc = void(AZ::u8*, const EmitInfo&, EmitSpawnParam&);
        using DistUpdateFunc = void(AZ::u8*, const Distribution&);
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

        void Execute(AZ::u8* data, const EmitInfo& info, EmitSpawnParam& spawnParam) const
        {
            fn(data, info, spawnParam);
        }

        void Update(AZ::u8* data, const Distribution& distribution) const
        {
            distFn(data, distribution);
        }

        AZ::u32 DataSize() const
        {
            return fn.size;
        }

        const AZStd::string& Name() const
        {
            return name;
        }

    private:
        EmitterDelegate fn;
        DistUpdateDelegate distFn;
        AZStd::string name;
    };

    class ParticleSpawnEffector {
    public:
        using SpawnFunc = void(AZ::u8*, const SpawnInfo&, Particle&);
        using DistUpdateFunc = void(AZ::u8*, const Distribution&);
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

        void Execute(AZ::u8* data, const SpawnInfo& info, Particle& particle) const
        {
            fn(data, info, particle);
        }

        void Update(AZ::u8* data, const Distribution& distribution) const
        {
            distFn(data, distribution);
        }

        AZ::u32 DataSize() const
        {
            return fn.size;
        }

        AZStd::string Name() const
        {
            return name;
        }

    private:
        SpawnDelegate fn;
        DistUpdateDelegate distFn;
        AZStd::string name;
    };

    class ParticleUpdateEffector {
    public:
        using UpdateFunc = void(AZ::u8*, const UpdateInfo&, Particle&);
        using DistUpdateFunc = void(AZ::u8*, const Distribution&);
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

        void Execute(AZ::u8* data, const UpdateInfo& info, Particle& particle) const
        {
            fn(data, info, particle);
        }

        void Update(AZ::u8* data, const Distribution& distribution) const
        {
            distFn(data, distribution);
        }

        AZ::u32 DataSize() const
        {
            return fn.size;
        }

        AZStd::string Name() const
        {
            return name;
        }

    private:
        UpdateDelegate fn;
        DistUpdateDelegate distFn;
        AZStd::string name;
    };

    class ParticleEventEffector {
    public:
        using EventFunc = void(AZ::u8*, const EventInfo&);
        using EventDelegate = ParticleDelegate<EventFunc>;

        template<typename Effector>
        explicit ParticleEventEffector(DataArgs<Effector>&&)
        {
            using DataType = typename Effector::DataType;
            fn.template Connect<&Effector::Execute, DataType>();
            name = TypeInfo<Effector>::Name();
        }

        ~ParticleEventEffector() = default;

        void Execute(AZ::u8* data, const EventInfo& info) const
        {
            fn(data, info);
        }

        AZ::u32 DataSize() const
        {
            return fn.size;
        }

        const AZStd::string& Name() const
        {
            return name;
        }

    private:
        EventDelegate fn;
        AZStd::string name;
    };
}
