/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    struct SpawnVelDirection {
        using DataType = SpawnVelDirection;
        static void Execute(const SpawnVelDirection* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnVelDirection* data, const Distribution& distribution);

        ValueObjVec3 direction { { 0.f, 0.f, 0.f } };
        AZ::u64 padding0 = 0;
        AZ::u64 padding1 = 0;
        ValueObjFloat strength { 1.f };
    };

    struct SpawnVelSector {
        using DataType = SpawnVelSector;
        static void Execute(const SpawnVelSector* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnVelSector* data, const Distribution& distribution);

        ValueObjFloat strength { 1.f };
        float centralAngle = 60.f;
        float rotateAngle  = 0.f;
        AZ::Vector3 direction = { 0.f, 0.f, 0.f };
    };

    struct SpawnVelCone {
        using DataType = SpawnVelCone;
        static void Execute(const SpawnVelCone* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnVelCone* data, const Distribution& distribution);

        ValueObjFloat strength { 1.f };
        float angle = 60.f;
        AZ::u32 padding0 = 0;
        AZ::Vector3 direction = { 0.f, 0.f, 0.f };
    };

    struct SpawnVelSphere {
        using DataType = SpawnVelSphere;
        static void Execute(const SpawnVelSphere* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnVelSphere* data, const Distribution& distribution);

        ValueObjVec3 strength { { 1.f, 1.f, 1.f } };
    };

    struct SpawnVelConcentrate {
        using DataType = SpawnVelConcentrate;
        static void Execute(const SpawnVelConcentrate* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnVelConcentrate* data, const Distribution& distribution);

        ValueObjFloat rate { 1.f };
        AZ::u64 padding0 = 0;
        AZ::Vector3 centre = { 0.f, 0.f, 0.f };
    };
}

