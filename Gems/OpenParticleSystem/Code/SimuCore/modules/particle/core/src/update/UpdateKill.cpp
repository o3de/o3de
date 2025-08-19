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
    static bool BoxContainPoint(const Vector3& min, const Vector3& max, const Vector3& point)
    {
        return (point.x > min.x) && (point.y > min.y) && (point.z > min.z) &&
            (point.x < max.x) && (point.y < max.y) && (point.z < max.z);
    }

    void KillInBox::Execute(const KillInBox* data, const UpdateInfo& info, Particle& particle)
    {
        Vector3 size = Vector3(
            std::fabs(data->boxSize.x), std::fabs(data->boxSize.y), std::fabs(data->boxSize.z)
        );
        Vector3 min = data->useLocalSpace ?
            info.emitterTrans.TransformPoint(data->positionOffset) - size / 2.0f :
            data->positionOffset - size / 2.0f;
        Vector3 max = data->useLocalSpace ?
            info.emitterTrans.TransformPoint(data->positionOffset) + size / 2.0f :
            data->positionOffset + size / 2.0f;
        bool containLast = BoxContainPoint(min, max, particle.globalPosition);
        Vector3 newPosition = particle.globalPosition + particle.velocity * info.tickTime;
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
