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
    struct UpdateSizeLinear {
        using DataType = UpdateSizeLinear;
        static void Execute(const UpdateSizeLinear* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(UpdateSizeLinear* data, const Distribution& distribution);

        ValueObjVec3 size { { 1.f, 1.f, 1.f } };
    };

    struct UpdateSizeByVelocity {
        using DataType = UpdateSizeByVelocity;
        static void Execute(const UpdateSizeByVelocity* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(UpdateSizeByVelocity* data, const Distribution& distribution);

        ValueObjLinear velScale { {
            AZ::Vector3(1.f),
            AZ::Vector3(0.1f),
            AZ::Vector3(1.f) } };
        float velocityRange = 1.f;
    };

    struct SizeScale {
        using DataType = SizeScale;
        static void Execute(const SizeScale* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(SizeScale* data, const Distribution& distribution);

        ValueObjVec3 scaleFactor { { 1.f, 1.f, 1.f } };
    };
}

