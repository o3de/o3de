/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstdint>
#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    struct ParticleEventHandler {
        using DataType = ParticleEventHandler;
        static void Execute(const ParticleEventHandler* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam);
        static void UpdateDistPtr(const ParticleEventHandler* data, const Distribution& distribution);

        AZ::u32 emitterIndex = UINT32_MAX;
        AZ::u32 eventType = UINT32_MAX;
        AZ::u32 maxEventNum = 0;
        AZ::u32 emitNum = 1;
        bool useEventInfo = false;
    };

    struct InheritanceHandler {
        using DataType = InheritanceHandler;
        static void Execute(const InheritanceHandler* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam);
        static void UpdateDistPtr(const InheritanceHandler* data, const Distribution& distribution);

        AZ::Vector3 positionOffset;
        AZ::Vector3 velocityRatio;
        AZ::Vector4 colorRatio;
        AZ::u32 emitterIndex = UINT32_MAX;
        float spawnRate = 0.0f;
        bool calculateSpawnRate = false;
        bool spawnEnable = true;
        bool applyPosition = false;
        bool applyVelocity = false;
        bool overwriteVelocity = false;
        bool applySize = false;
        bool applyColorRGB = false;
        bool applyColorAlpha = false;
        bool applyAge = false;
        bool applyLifetime = false;
    };
}
