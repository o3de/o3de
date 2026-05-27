/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/update/UpdateRotateAroundPoint.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void UpdateRotateAroundPoint::Execute(
        const UpdateRotateAroundPoint* data, const UpdateInfo& info, Particle& particle)
    {
        particle.rotateAroundPoint.SetW(particle.rotateAroundPoint.GetW() + data->rotateRate * info.tickTime);
        float theta = particle.rotateAroundPoint.GetW();
        AZ::Vector3 lastPosition = particle.localPosition;
        particle.localPosition =
            data->xAxis * data->radius * cos(theta) + data->yAxis * data->radius * sin(theta) + data->center;
        if (info.tickTime > AZ::Constants::FloatEpsilon) {
            particle.velocity = (particle.localPosition - lastPosition) / info.tickTime;
        }
        particle.localPosition -= particle.velocity * info.tickTime;
    }
    
    void UpdateRotateAroundPoint::UpdateDistPtr(const UpdateRotateAroundPoint* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }
}
