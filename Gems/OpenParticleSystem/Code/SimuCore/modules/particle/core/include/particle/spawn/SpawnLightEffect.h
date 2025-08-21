/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    struct SpawnLightEffect {
        using DataType = SpawnLightEffect;
        static void Execute(const SpawnLightEffect* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnLightEffect* data, const Distribution& distribution);

        ValueObjColor lightColor { { 1.f, 1.f, 1.f, 1.f } };
        ValueObjFloat intensity { 1.f };
        ValueObjFloat radianScale { 1.f };
    };
}

