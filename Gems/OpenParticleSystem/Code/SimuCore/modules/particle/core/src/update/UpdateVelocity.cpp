/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/update/UpdateVelocity.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void UpdateVelocity::Execute(const UpdateVelocity* data, const UpdateInfo& info, Particle& particle)
    {
        particle.velocity = CalcDistributionTickValue(data->strength, info.baseInfo, particle) *
            CalcDistributionTickValue(data->direction, info.baseInfo, particle);
    }

    void UpdateVelocity::UpdateDistPtr(UpdateVelocity* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->strength, distribution);
        UpdateDistributionPtr(data->direction, distribution);
    }

    void UpdateStochasticVelocity::Execute(
        const UpdateStochasticVelocity* data, const UpdateInfo& info, Particle& particle)
    {
        float length = 0.0f;
        if (data->scope > info.randomStream->UnitRandom()) {
            if (data->keepVelocity) {
                length = particle.velocity.GetLength();
            }
            particle.velocity += AZ::Vector3(
                data->randomness * info.randomStream->SymmetricRandom() * info.tickTime,
                data->randomness * info.randomStream->SymmetricRandom() * info.tickTime,
                data->randomness * info.randomStream->SymmetricRandom() * info.tickTime);
            if (data->keepVelocity) {
                particle.velocity *= length / particle.velocity.GetLength();
            }
        }
    }

    void UpdateStochasticVelocity::UpdateDistPtr(const UpdateStochasticVelocity* data,
        const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }
}
