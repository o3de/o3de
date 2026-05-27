/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/update/UpdateSize.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void UpdateSizeLinear::Execute(const UpdateSizeLinear* data, const UpdateInfo& info, Particle& particle)
    {
        particle.scale = particle.baseScale * CalcDistributionTickValue(data->size, info.baseInfo, particle);
    }

    void UpdateSizeLinear::UpdateDistPtr(UpdateSizeLinear* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->size, distribution);
    }

    void UpdateSizeByVelocity::Execute(const UpdateSizeByVelocity* data, const UpdateInfo& info, Particle& particle)
    {
        float vel = std::max(AZ::Constants::FloatEpsilon, particle.velocity.GetLength());
        vel = std::min(vel, data->velocityRange);
        vel = data->velocityRange > AZ::Constants::FloatEpsilon ? vel / data->velocityRange : 1.f;
        LinearValue velSize = CalcDistributionTickValue(data->velScale, info.baseInfo, particle);
        AZ::Vector3 size;
        if (data->velScale.distType == DistributionType::CONSTANT) {
            size = velSize.minValue * (1.0f - vel) + velSize.maxValue * vel;
        }
        if (data->velScale.distType == DistributionType::CURVE) {
            size = velSize.value;
        }
        particle.scale = particle.baseScale * size;
    }

    void UpdateSizeByVelocity::UpdateDistPtr(UpdateSizeByVelocity* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->velScale, distribution);
    }

    void SizeScale::Execute(const SizeScale* data, const UpdateInfo& info, Particle& particle)
    {
        particle.scale = particle.baseScale * CalcDistributionTickValue(data->scaleFactor, info.baseInfo, particle);
    }

    void SizeScale::UpdateDistPtr(SizeScale* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->scaleFactor, distribution);
    }
}
