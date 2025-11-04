/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnLightEffect.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void SpawnLightEffect::Execute(const SpawnLightEffect* data, const SpawnInfo& info, Particle& particle)
    {
        particle.hasLightEffect = true;
        particle.lightColor = CalcDistributionTickValue(data->lightColor, info.baseInfo, particle) *
            CalcDistributionTickValue(data->intensity, info.baseInfo, particle);
        particle.radianScale = CalcDistributionTickValue(data->radianScale, info.baseInfo, particle);
    }

    void SpawnLightEffect::UpdateDistPtr(SpawnLightEffect* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->lightColor, distribution);
        UpdateDistributionPtr(data->intensity, distribution);
        UpdateDistributionPtr(data->radianScale, distribution);
    }
}
