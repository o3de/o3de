/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/update/UpdateKill.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    static bool BoxContainPoint(const AZ::Vector3& min, const AZ::Vector3& max, const AZ::Vector3& point)
    {
        return (point.GetX() > min.GetX()) && (point.GetY() > min.GetY()) && (point.GetZ() > min.GetZ()) &&
            (point.GetX() < max.GetX()) && (point.GetY() < max.GetY()) && (point.GetZ() < max.GetZ());
    }

    void KillInBox::Execute(const KillInBox* data, const UpdateInfo& info, Particle& particle)
    {
        AZ::Vector3 size = AZ::Vector3(
            std::fabs(data->boxSize.GetX()), std::fabs(data->boxSize.GetY()), std::fabs(data->boxSize.GetZ())
        );
        AZ::Vector3 min = data->useLocalSpace ?
            info.emitterTrans.TransformPoint(data->positionOffset) - size / 2.0f :
            data->positionOffset - size / 2.0f;
        AZ::Vector3 max = data->useLocalSpace ?
            info.emitterTrans.TransformPoint(data->positionOffset) + size / 2.0f :
            data->positionOffset + size / 2.0f;
        bool containLast = BoxContainPoint(min, max, particle.globalPosition);
        AZ::Vector3 newPosition = particle.globalPosition + particle.velocity * info.tickTime;
        bool containNow = BoxContainPoint(min, max, newPosition);
        if (!data->enableKill) {
            return;
        }
        if (data->invertBox) {
            if (!containLast || !containNow) {
                particle.needKill = true;
            }
        } else {
            if (containLast || containNow) {
                particle.needKill = true;
            }
        }
    }

    void KillInBox::UpdateDistPtr(const KillInBox* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }
} // namespace SimuCore
