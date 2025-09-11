/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnEvent.h"

namespace SimuCore::ParticleCore {
    void SpawnLocationEvent::Execute(const SpawnLocationEvent* data, const SpawnInfo& info, const Particle& particle)
    {
        if (data->whetherSendEvent) {
            AZ::u64 key = (static_cast<AZ::u64>(info.currentEmitter) << 32) +
                static_cast<AZ::u64>(ParticleEventType::SPAWN_LOCATION);
            auto iter = info.systemEventPool->events.find(key);
            if (iter == info.systemEventPool->events.end()) {
                info.systemEventPool->events[key] = AZStd::vector<ParticleEventInfo>();
            }
            ParticleEventInfo e;
            e.eventTimeBeforeTick = 0.0f;
            e.eventPosition = info.emitterTrans.TransformPoint(particle.localPosition);
            (void)info.systemEventPool->events[key].emplace_back(e);
        }
    }

    void SpawnLocationEvent::UpdateDistPtr(const SpawnLocationEvent* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }
}
