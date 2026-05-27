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
    struct UpdateConstForce {
        using DataType = UpdateConstForce;
        static void Execute(const UpdateConstForce* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(UpdateConstForce* data, const Distribution& distribution);

        ValueObjVec3 force { { 0.f, 0.f, 0.f } };
    };

    struct UpdateDragForce {
        using DataType = UpdateDragForce;
        static void Execute(const UpdateDragForce* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(UpdateDragForce* data, const Distribution& distribution);

        ValueObjFloat dragCoefficient { 1.f };
    };

    struct UpdateCurlNoiseForce {
        using DataType = UpdateCurlNoiseForce;
        static void Execute(const UpdateCurlNoiseForce* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(const UpdateCurlNoiseForce* data, const Distribution& distribution);

        AZ::Vector3 panNoiseField;
        AZ::Vector3 randomizationVector;
        float noiseStrength = 10.0f;
        float noiseFrequency = 5.0f;
        AZ::u32 randomSeed = 0;
        bool panNoise = false;
    };

    struct UpdatePointForce {
        using DataType = UpdatePointForce;
        static void Execute(const UpdatePointForce* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(const UpdatePointForce* data, const Distribution& distribution);

        AZ::Vector3 position = { 0.f, 0.f, 0.f };
        float force = 0.f;
        bool useLocalSpace = true;
    };

    struct UpdateVortexForce {
        using DataType = UpdateVortexForce;
        static void Execute(const UpdateVortexForce* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(UpdateVortexForce* data, const Distribution& distribution);

        ValueObjFloat originPull { 1.f };
        ValueObjFloat vortexRate { 1.f };
        ValueObjFloat vortexRadius { 1.f };
        AZ::u64 padding0 = 0;
        AZ::Vector3 vortexAxis = { 0.f, 0.f, 1.f };
        AZ::Vector3 origin = { 0.f, 0.f, 0.f };
    private:
        static void GetAxis(const AZ::Vector3& axis, AZ::Vector3 dir, AZ::Vector3& xAxis, AZ::Vector3& yAxis);
    };
}

