/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnSize.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void SpawnSize::Execute(const SpawnSize* data, const SpawnInfo& info, Particle& particle)
    {
        particle.baseScale = CalcDistributionTickValue(data->size, info.baseInfo, particle);
        particle.scale = particle.baseScale;
    }

    void SpawnSize::UpdateDistPtr(SpawnSize* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->size, distribution);
    }
}
