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
    struct SpawnSize {
        using DataType = SpawnSize;
        static void Execute(const SpawnSize* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnSize* data, const Distribution& distribution);

        ValueObjVec3 size { { 1.f, 1.f, 1.f } };
    };
}

