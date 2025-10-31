/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnColor.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void SpawnColor::Execute(const SpawnColor* data, const SpawnInfo& info, Particle& particle)
    {
        particle.color = CalcDistributionTickValue(data->startColor, info.baseInfo, particle);
    }

    void SpawnColor::UpdateDistPtr(SpawnColor* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->startColor, distribution);
    }
}
