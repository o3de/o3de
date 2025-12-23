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
    struct UpdateRotateAroundPoint {
        using DataType = UpdateRotateAroundPoint;
        static void Execute(const UpdateRotateAroundPoint* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(const UpdateRotateAroundPoint* data, const Distribution& distribution);

        AZ::Vector3 xAxis = { 1.f, 0.f, 0.f };   // Two axes determine a plane.
        AZ::Vector3 yAxis = { 0.f, 1.f, 0.f };   // The rotation axis is obtained by multiplying two axes.
        AZ::Vector3 center = { 0.f, 0.f, 0.f };
        float rotateRate = 1.f;
        float radius = 1.f;
    };
}
