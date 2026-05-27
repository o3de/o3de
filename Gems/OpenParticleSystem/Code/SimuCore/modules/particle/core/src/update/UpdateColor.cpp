/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/update/UpdateColor.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void UpdateColor::Execute(const UpdateColor* data, const UpdateInfo& info, Particle& particle)
    {
        particle.color = CalcDistributionTickValue(data->currentColor, info.baseInfo, particle);
    }

    void UpdateColor::UpdateDistPtr(UpdateColor* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->currentColor, distribution);
    }
}