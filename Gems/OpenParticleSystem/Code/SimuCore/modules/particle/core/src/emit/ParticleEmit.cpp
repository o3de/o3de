/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/emit/ParticleEmit.h"
#include "particle/core/Particle.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void EmitBurstList::Execute(const EmitBurstList* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam)
    {
        auto fn = [&info](int min, AZ::u32 max) {
            return info.randomStream->RandRange(static_cast<float>(min), static_cast<float>(max));
        };

        for (size_t index = 0; index < data->arrSize; index++) {
            auto singleBurst = data->burstList[index];
            float delta = info.baseInfo.currentTime - singleBurst.time;
            if (singleBurst.time < info.baseInfo.duration && delta >= 0 && delta <= info.tickTime) {
                emitSpawnParam.burstNum += singleBurst.minCount < 0 ?
                    singleBurst.count :
                    static_cast<AZ::u32>(fn(singleBurst.minCount, singleBurst.count));
                emitSpawnParam.realEmitTime = delta;
            }
        }
        emitSpawnParam.isProcessBurstList = emitSpawnParam.isProcessBurstList && data->isProcessBurstList;
    }

    void EmitBurstList::UpdateDistPtr([[maybe_unused]] EmitBurstList* data, const Distribution& distribution)
    {
        (void)distribution;
    }

    void EmitSpawn::Execute(EmitSpawn* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam)
    {
        emitSpawnParam.isProcessSpawnRate = emitSpawnParam.isProcessSpawnRate && data->isProcessSpawnRate;
        if (!info.started) {
            data->current = 0.f;
        }
        float last = data->current;
        float updateValue = CalcDistributionTickValue(data->spawnRate, info.baseInfo);
        data->current += updateValue * info.tickTime;
        emitSpawnParam.spawnNum += static_cast<AZ::u32>(data->current) - static_cast<AZ::u32>(last);
        if (emitSpawnParam.spawnNum > 0) {
            emitSpawnParam.realEmitTime = (data->current - std::floor(data->current)) / updateValue;
        }
    }

    void EmitSpawn::UpdateDistPtr(EmitSpawn* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->spawnRate, distribution);
    }

    void EmitSpawnOverMoving::Execute(EmitSpawnOverMoving* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam)
    {
        if (info.moveDistance <= 0.f) {
            data->currentNum = 0.f;
        }
        float lastNum = data->currentNum;
        data->currentNum += CalcDistributionTickValue(data->spawnRatePerUnit, info.baseInfo) * info.moveDistance;
        emitSpawnParam.numOverMoving += static_cast<AZ::u32>(data->currentNum) - static_cast<AZ::u32>(lastNum);
        emitSpawnParam.isProcessSpawnRate = emitSpawnParam.isProcessSpawnRate && data->isProcessSpawnRate;
        emitSpawnParam.isProcessBurstList = emitSpawnParam.isProcessBurstList && data->isProcessBurstList;
        emitSpawnParam.isIgnoreSpawnRate = emitSpawnParam.isIgnoreSpawnRate || data->isIgnoreSpawnRateWhenMoving;
    }

    void EmitSpawnOverMoving::UpdateDistPtr(EmitSpawnOverMoving* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->spawnRatePerUnit, distribution);
    }
}
