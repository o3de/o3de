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
        Vector3 axis = data->initAxis;
        Vector3 rAxis = data->rotateAxis;
        particle.rotation = data->initAxis.IsEqual(VEC3_ZERO) ?
            Vector4(data->initAxis, Math::AngleToRadians(updateValue)) :
            Vector4(axis.Normalize(), Math::AngleToRadians(updateValue));
        particle.rotationVector = data->rotateAxis.IsEqual(VEC3_ZERO) ?
            Vector4(data->rotateAxis, 0) : Vector4(rAxis.Normalize(), 0);
    }

    void SpawnRotation::UpdateDistPtr(SpawnRotation* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->initAngle, distribution);
        UpdateDistributionPtr(data->rotateSpeed, distribution);
    }
}
