/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstdint>
#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    struct UpdateSubUv {
        using DataType = UpdateSubUv;
        static void Execute(const UpdateSubUv* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(const UpdateSubUv* data, const Distribution& distribution);

        AZ::u32 framePerSecond = 30;
        AZ::u32 frameNum = 1;

        // only update subUV when particle spawn
        bool spawnOnly{false};
        bool IndexByEventOrder{false};
    };
}

