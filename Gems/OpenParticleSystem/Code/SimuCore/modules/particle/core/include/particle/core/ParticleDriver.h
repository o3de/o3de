/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "particle/core/Particle.h"
#include "particle/core/ParticleDelegate.h"

namespace SimuCore::ParticleCore {
    class ParticleDriver {
    public:
        using BufferCreateFn = void(AZ::u8*, const BufferCreate&, GpuInstance&);
        using BufferCreateDelegate = ParticleDelegate<BufferCreateFn>;

        using BufferUpdateFn = void(AZ::u8*, const BufferUpdate&, const GpuInstance&);
        using BufferUpdateDelegate = ParticleDelegate<BufferUpdateFn>;

        using BufferDestroyFn = void(AZ::u8*, const GpuInstance&);
        using BufferDestroyDelegate = ParticleDelegate<BufferDestroyFn>;

        template<auto Func, typename Driver>
        static void BindBufferCreate()
        {
            bufferCreateFn.template Connect<Func, Driver>();
        }

        template<auto Func, typename Driver>
        static void BindBufferUpdate()
        {
            bufferUpdateFn.template Connect<Func, Driver>();
        }

        template<auto Func, typename Driver>
        static void BindBufferDestroy()
        {
            bufferDestroyFn.template Connect<Func, Driver>();
        }

        static BufferCreateDelegate bufferCreateFn;
        static BufferUpdateDelegate bufferUpdateFn;
        static BufferDestroyDelegate bufferDestroyFn;
    };
}
