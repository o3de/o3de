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
    class ParticleDistribution {
    public:
        ParticleDistribution() = default;
        virtual ~ParticleDistribution() = default;

        virtual float Tick(const BaseInfo& info) = 0;
        virtual float Tick(const BaseInfo& info, const Particle& particle) = 0;
    };
} // SimuCore::ParticleCore
