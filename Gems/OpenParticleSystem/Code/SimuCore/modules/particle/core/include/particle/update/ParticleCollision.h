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
    enum class CpuCollisionType : AZ::u8 {
        PLANE
    };

    enum class RadiusCalculationType : AZ::u8 {
        SPRITE,
        MESH,
        CUSTOM
    };

    enum class RadiusCalculationMethod : AZ::u8 {
        BOUNDS,
        MAXIMUM_AXIS,
        MINIMUM_AXIS
    };

    struct CollisionRadius {
        RadiusCalculationType type = RadiusCalculationType::SPRITE;
        RadiusCalculationMethod method = RadiusCalculationMethod::BOUNDS;
        float radius = 1.f;
        float radiusScale = 1.f;
    };

    struct CollisionPlane {
        AZ::Vector3 normal = { 0.f, 1.f, 0.f };
        AZ::Vector3 position = { 0.f, 0.f, 0.f };
    };

    struct CollisionParam {
        CollisionPlane localPlane;
        float lastDotPlane = 0.f;
        float newDotPlane = 0.f;
        float radius = 0.0f;
    };

    struct Bounce {
        float restitution = 1.f;
        float randomizeNormal = 0.f;
    };

    struct Friction {
        float friCoefficient = 0.f;
    };

    struct ParticleCollision {
        using DataType = ParticleCollision;
        static void Execute(const ParticleCollision* data, const UpdateInfo& info, Particle& particle);
        static void UpdateDistPtr(const ParticleCollision* data, const Distribution& distribution);

        CpuCollisionType collisionType = CpuCollisionType::PLANE;
        CollisionRadius collisionRadius;
        Bounce bounce;
        Friction friction;
        bool useTwoPlane = false;
        CollisionPlane collisionPlane1;
        CollisionPlane collisionPlane2;
    };
}

