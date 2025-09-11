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
    struct UpdateVelocity {
        using DataType = UpdateVelocity;
        static void Execute(const UpdateVelocity* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(UpdateVelocity* data, const Distribution& distribution);

        ValueObjVec3 direction { { 0.f, 0.f, 0.f } };
        AZ::u64 padding0 = 0;
        AZ::u64 padding1 = 0;
        ValueObjFloat strength { 1.f };
    };

    struct UpdateStochasticVelocity {
        using DataType = UpdateStochasticVelocity;
        static void Execute(const UpdateStochasticVelocity* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(const UpdateStochasticVelocity* data, const Distribution& distribution);

        float randomness = 0.f;
        float scope = 0.f;
        bool keepVelocity = false;
    };
}

