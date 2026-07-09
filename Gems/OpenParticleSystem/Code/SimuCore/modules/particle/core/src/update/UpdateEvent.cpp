/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/update/UpdateEvent.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void UpdateLocationEvent::Execute(const UpdateLocationEvent* data, const EventInfo& info)
    {
        if (!data->whetherSendEvent) {
            return;
        }
        AZ::u64 key = (static_cast<AZ::u64>(info.currentEmitter) << 32) +
            static_cast<AZ::u64>(ParticleEventType::UPDATE_LOCATION);
        auto& events = info.systemEventPool->events[key];
        ForEach(info.particleBuffer, info.begin, info.alive, [/*&info,*/ &events](Particle& particle) {

            if (particle.currentLife < particle.lifeTime) {
                ParticleEventInfo e;
                e.eventTimeBeforeTick = 0.0f;
                e.eventPosition = particle.globalPosition;
                e.color = particle.color;
                e.size = particle.scale;
                e.particleId = particle.id;
                e.lifeTime = particle.lifeTime;
                e.currentLife = particle.currentLife;
                e.locationEventIdx = particle.locationEventCount;
                particle.locationEventCount += 1;
                (void)events.emplace_back(e);
            }
        });
    }

    void UpdateDeathEvent::Execute(const UpdateDeathEvent* data, const EventInfo& info)
    {
        if (!data->whetherSendEvent) {
            return;
        }
        AZ::u64 key = (static_cast<AZ::u64>(info.currentEmitter) << 32) +
            static_cast<AZ::u64>(ParticleEventType::UPDATE_DEATH);
        auto& events = info.systemEventPool->events[key];
        ForEach(info.particleBuffer, info.begin, info.alive, [&info, &events](const Particle& particle) {
            if (particle.currentLife >= particle.lifeTime) {
                ParticleEventInfo e;
                e.eventTimeBeforeTick = particle.currentLife - particle.lifeTime;
                AZ::Vector3 tmpVel = info.localSpace ? info.emitterTrans.TransformVector(particle.velocity) :
                    particle.spawnTrans.TransformVector(particle.velocity);
                e.eventPosition = particle.globalPosition - tmpVel * e.eventTimeBeforeTick;
                e.particleId = particle.id;
                (void)events.emplace_back(e);
            }
        });
    }

    void UpdateCollisionEvent::Execute(const UpdateCollisionEvent* data, const EventInfo& info)
    {
        if (!data->whetherSendEvent) {
            return;
        }
        AZ::u64 key = (static_cast<AZ::u64>(info.currentEmitter) << 32) +
            static_cast<AZ::u64>(ParticleEventType::UPDATE_COLLISION);
        auto& events = info.systemEventPool->events[key];
        ForEach(info.particleBuffer, info.begin, info.alive, [/* &info,*/ &events](Particle& particle)
            {
            if (particle.currentLife < particle.lifeTime && particle.isCollided) {
                ParticleEventInfo e;
                e.eventTimeBeforeTick = particle.collisionTimeBeforeTick;
                e.eventPosition = particle.collisionPosition;
                e.particleId = particle.id;
                (void)events.emplace_back(e);
                particle.isCollided = false;
            }
        });
    }

    void UpdateInheritanceEvent::Execute(const UpdateInheritanceEvent* data, const EventInfo& info)
    {
        if (!data->whetherSendEvent) {
            return;
        }
        ForEach(info.particleBuffer, info.begin, info.alive, [&info](const Particle& particle) {
            if (particle.currentLife < particle.lifeTime) {
                InheritanceInfo inh;
                inh.position = particle.globalPosition;
                inh.velocity = particle.velocity;
                inh.size = particle.scale;
                inh.color = particle.color;
                inh.currentLife = particle.currentLife;
                inh.lifetime = particle.lifeTime;
                info.systemEventPool->inheritances[static_cast<size_t>(info.currentEmitter)][particle.id] = inh;
            }
        });
    }
}
