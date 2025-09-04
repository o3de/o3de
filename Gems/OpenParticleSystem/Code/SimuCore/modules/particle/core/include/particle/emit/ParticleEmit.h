/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>
#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    constexpr int BURST_MAX_NUMBER = 100;

    struct SingleBurst {
        float time = 0.f;
        AZ::u32 count = 0;
        int minCount = -1;
        AZ::u32 pad0 = 0u;
    };

    struct EmitBurstList {
        using DataType = EmitBurstList;
        static void Execute(const EmitBurstList* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam);
        static void UpdateDistPtr([[maybe_unused]] EmitBurstList* data, const Distribution& distribution);

        AZStd::array<SingleBurst, BURST_MAX_NUMBER> burstList;
        size_t arrSize = 0u;
        bool isProcessBurstList = true;
    };

    struct EmitSpawn {
        using DataType = EmitSpawn;
        static void Execute(EmitSpawn* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam);
        static void UpdateDistPtr(EmitSpawn* data, const Distribution& distribution);

        ValueObjFloat spawnRate { 10.f };
        float current = 0.f;
        bool isProcessSpawnRate = true;
    };

    struct EmitSpawnOverMoving {
        using DataType = EmitSpawnOverMoving;
        static void Execute(EmitSpawnOverMoving* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam);
        static void UpdateDistPtr(EmitSpawnOverMoving* data, const Distribution& distribution);

        ValueObjFloat spawnRatePerUnit { 10.f };
        float currentNum = 0.f;
        bool isIgnoreSpawnRateWhenMoving = false;
        bool isProcessSpawnRate = true;
        bool isProcessBurstList = true;
    };
}
