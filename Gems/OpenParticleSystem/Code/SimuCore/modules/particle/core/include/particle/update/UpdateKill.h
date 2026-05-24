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
    struct KillInBox {
        using DataType = KillInBox;
        static void Execute(const KillInBox* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(const KillInBox* data, const Distribution& distribution);

        AZ::Vector3 positionOffset{ 0.f, 0.f, 0.f };
        AZ::Vector3 boxSize{ 1.f, 1.f, 1.f };
        bool enableKill = true;
        bool invertBox = false;
        bool useLocalSpace = true;
    };
} // namespace SimuCore
