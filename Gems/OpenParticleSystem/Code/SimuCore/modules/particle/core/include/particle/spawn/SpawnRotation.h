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
    struct SpawnRotation {
        using DataType = SpawnRotation;
        static void Execute(const SpawnRotation* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnRotation* data, const Distribution& distribution);

        ValueObjFloat initAngle { 0.f };
        ValueObjFloat rotateSpeed { 0.f };
        AZ::Vector3 initAxis = { 0.f, 1.f, 0.f };
        AZ::Vector3 rotateAxis = { 1.f, 0.f, 0.f };
    };
}

