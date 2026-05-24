/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnRotation.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void SpawnRotation::Execute(const SpawnRotation* data, const SpawnInfo& info, Particle& particle)
    {
        particle.angularVel = CalcDistributionTickValue(data->rotateSpeed, info.baseInfo, particle);
        auto updateValue = CalcDistributionTickValue(data->initAngle, info.baseInfo, particle);
        AZ::Vector3 axis = data->initAxis;
        AZ::Vector3 rAxis = data->rotateAxis;
        particle.rotation = data->initAxis.IsClose(AZ::Vector3::CreateZero()) ?
            AZ::Vector4(data->initAxis, AZ::DegToRad(updateValue)) :
            AZ::Vector4(axis.GetNormalized(), AZ::DegToRad(updateValue));
        particle.rotationVector = data->rotateAxis.IsClose(AZ::Vector3::CreateZero()) ?
            AZ::Vector4(data->rotateAxis, 0) : AZ::Vector4(rAxis.GetNormalized(), 0);
    }

    void SpawnRotation::UpdateDistPtr(SpawnRotation* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->initAngle, distribution);
        UpdateDistributionPtr(data->rotateSpeed, distribution);
    }
}
