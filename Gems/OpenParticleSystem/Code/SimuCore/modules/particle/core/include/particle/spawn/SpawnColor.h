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
    struct SpawnColor {
        using DataType = SpawnColor;
        static void Execute(const SpawnColor* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnColor* data, const Distribution& distribution);

        ValueObjColor startColor { { 1.f, 1.f, 1.f, 1.f } };
    };
}

