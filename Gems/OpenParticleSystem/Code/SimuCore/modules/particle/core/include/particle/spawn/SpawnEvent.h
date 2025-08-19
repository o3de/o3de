/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SIMUPARTICLE_SPAWN_SPAWN_EVENT_H
#define SIMUPARTICLE_SPAWN_SPAWN_EVENT_H

#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    struct SpawnLocationEvent {
        using DataType = SpawnLocationEvent;
        static void Execute(const SpawnLocationEvent* data, const SpawnInfo& info, const Particle& particle);
        static void UpdateDistPtr(const SpawnLocationEvent* data, const Distribution& distribution);

        bool whetherSendEvent = true;
    };
}

#endif // SIMUPARTICLE_SPAWN_SPAWN_EVENT_H
