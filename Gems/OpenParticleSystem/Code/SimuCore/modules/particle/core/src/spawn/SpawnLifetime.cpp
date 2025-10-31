/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnLifetime.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void SpawnLifetime::Execute(const SpawnLifetime* data, const SpawnInfo& info, Particle& particle)
    {
        particle.lifeTime += CalcDistributionTickValue(data->lifeTime, info.baseInfo, particle);
    }

    void SpawnLifetime::UpdateDistPtr(SpawnLifetime* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->lifeTime, distribution);
    }
}
