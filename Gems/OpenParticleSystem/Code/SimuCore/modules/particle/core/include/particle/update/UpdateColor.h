/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SIMUPARTICLE_UPDATE_UPDATE_COLOR_H
#define SIMUPARTICLE_UPDATE_UPDATE_COLOR_H

#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    struct UpdateColor {
        using DataType = UpdateColor;
        static void Execute(const UpdateColor* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(UpdateColor* data, const Distribution& distribution);

        ValueObjColor currentColor { { 1.f, 1.f, 1.f, 1.f } };
    };
}

#endif // SIMUPARTICLE_UPDATE_UPDATE_COLOR_H
