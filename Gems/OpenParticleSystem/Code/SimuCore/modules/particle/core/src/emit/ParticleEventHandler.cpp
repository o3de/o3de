/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/emit/ParticleEventHandler.h"
#include "particle/core/Particle.h"
#include <AzCore/Math/Color.h>

namespace SimuCore::ParticleCore {
    void ParticleEventHandler::Execute(const ParticleEventHandler* data,
        const EmitInfo& info, EmitSpawnParam& emitSpawnParam)
    {
        AZ::u64 key = (static_cast<AZ::u64>(data->emitterIndex) << 32) + static_cast<AZ::u64>(data->eventType);
        auto iter = info.systemEventPool->events.find(key);
        if (iter != info.systemEventPool->events.end() && iter->second.size() != 0) {
            AZ::u32 eventNum = std::min(static_cast<AZ::u32>(iter->second.size()), data->maxEventNum);
            for (size_t i = 0; i < static_cast<size_t>(eventNum); i++) {
                ParticleEventInfo event = iter->second[i];
                event.emitNum = data->emitNum;
                event.useEventInfo = data->useEventInfo;
                (void)emitSpawnParam.eventSpawn.emplace_back(event);
            }
        }
    }

    void ParticleEventHandler::UpdateDistPtr(const ParticleEventHandler* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void InheritanceHandler::Execute(const InheritanceHandler* data,
        const EmitInfo& info, EmitSpawnParam& emitSpawnParam)
    {
        if (!data->spawnEnable || data->emitterIndex >= info.systemEventPool->inheritances.size()) {
            return;
        }
        AZStd::unordered_map<AZ::u32, InheritanceSpawn>* tempMap = new AZStd::unordered_map<AZ::u32, InheritanceSpawn>();
        tempMap->reserve(info.systemEventPool->inheritances.size());
        for (auto& systemInheritance : info.systemEventPool->inheritances[data->emitterIndex]) {
            InheritanceSpawn& spawn = (*tempMap)[static_cast<unsigned int>(systemInheritance.first)];
            auto iter = info.emitterInheritances->find(static_cast<unsigned int>(systemInheritance.first));
            if (data->calculateSpawnRate) {
                spawn.ribbonId = systemInheritance.first;
                spawn.position = systemInheritance.second.position + data->positionOffset;
                spawn.velocity = systemInheritance.second.velocity * data->velocityRatio;
                spawn.size = systemInheritance.second.size;
                spawn.color = systemInheritance.second.color * AZ::Color(data->colorRatio.GetX(), data->colorRatio.GetY(), data->colorRatio.GetZ(), data->colorRatio.GetW());
                spawn.currentLife = systemInheritance.second.currentLife;
                spawn.lifetime = systemInheritance.second.lifetime;
                spawn.applyPosition = data->applyPosition;
                spawn.applyVelocity = data->applyVelocity;
                spawn.overwriteVelocity = data->overwriteVelocity;
                spawn.applySize = data->applySize;
                spawn.applyColorRGB = data->applyColorRGB;
                spawn.applyColorAlpha = data->applyColorAlpha;
                spawn.applyAge = data->applyAge;
                spawn.applyLifetime = data->applyLifetime;

                if (iter != info.emitterInheritances->end()) {
                    spawn.currentNum = iter->second.currentNum;
                }
                float last = spawn.currentNum;
                spawn.currentNum += data->spawnRate * info.tickTime;
                spawn.emitNum = static_cast<AZ::u32>(spawn.currentNum) - static_cast<AZ::u32>(last);
                if (spawn.emitNum > 0) {
                    spawn.emitTime = (spawn.currentNum - std::floor(spawn.currentNum)) / data->spawnRate;
                }
            } else {
                if (iter == info.emitterInheritances->end()) {
                    spawn.position = systemInheritance.second.position + data->positionOffset;
                    spawn.velocity = systemInheritance.second.velocity * data->velocityRatio;
                    spawn.size = systemInheritance.second.size;
                    spawn.color = systemInheritance.second.color * AZ::Color(data->colorRatio.GetX(), data->colorRatio.GetY(), data->colorRatio.GetZ(), data->colorRatio.GetW());
                    spawn.currentLife = systemInheritance.second.currentLife;
                    spawn.lifetime = systemInheritance.second.lifetime;
                    spawn.applyPosition = data->applyPosition;
                    spawn.applyVelocity = data->applyVelocity;
                    spawn.overwriteVelocity = data->overwriteVelocity;
                    spawn.applySize = data->applySize;
                    spawn.applyColorRGB = data->applyColorRGB;
                    spawn.applyColorAlpha = data->applyColorAlpha;
                    spawn.applyAge = data->applyAge;
                    spawn.applyLifetime = data->applyLifetime;

                    spawn.currentNum = 1.0f;
                    spawn.emitNum = 1u;
                    spawn.emitTime = info.tickTime;
                } else {
                    spawn.currentNum = 1.0f;
                    spawn.emitNum = 0u;
                    spawn.emitTime = info.tickTime;
                }
            }
            for (AZ::u32 i = 0; i < spawn.emitNum; i++)
            {
                (void)emitSpawnParam.inheritanceSpawn.emplace_back(&spawn);
            }
        }
        info.emitterInheritances->swap(*tempMap);
        delete tempMap;
    }

    void InheritanceHandler::UpdateDistPtr(const InheritanceHandler* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }
}
