/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstdint>
#include <core/math/VectorX.h>
#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    struct ParticleEventHandler {
        using DataType = ParticleEventHandler;
        static void Execute(const ParticleEventHandler* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam);
        static void UpdateDistPtr(const ParticleEventHandler* data, const Distribution& distribution);

        uint32_t emitterIndex = UINT32_MAX;
        uint32_t eventType = UINT32_MAX;
        uint32_t maxEventNum = 0;
        uint32_t emitNum = 1;
        bool useEventInfo = false;
    };

    struct InheritanceHandler {
        using DataType = InheritanceHandler;
        static void Execute(const InheritanceHandler* data, const EmitInfo& info, EmitSpawnParam& emitSpawnParam);
        static void UpdateDistPtr(const InheritanceHandler* data, const Distribution& distribution);

        Vector3 positionOffset;
        Vector3 velocityRatio;
        Vector4 colorRatio;
        uint32_t emitterIndex = UINT32_MAX;
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
