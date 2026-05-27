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
    struct SpawnLocBox {
        using DataType = SpawnLocBox;
        static void Execute(const SpawnLocBox* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(const SpawnLocBox* data, const Distribution& distribution);

        AZ::Vector3 center = { 0.f, 0.f, 0.f };
        AZ::Vector3 size = { 1.f, 1.f, 1.f };
    };

    struct SpawnLocPoint {
        using DataType = SpawnLocPoint;
        static void Execute(const SpawnLocPoint* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(SpawnLocPoint* data, const Distribution& distribution);

        ValueObjVec3 pos { { 0.f, 0.f, 0.f } };
    };

    struct SpawnLocSphere {
        using DataType = SpawnLocSphere;
        static void Execute(const SpawnLocSphere* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(const SpawnLocSphere* data, const Distribution& distribution);

        AZ::Vector3 center = { 0.f, 0.f, 0.f };
        float radius = 1.f;
        float ratio = 1.f;
        float angle = 360.f;
        float radiusThickness = 1.f;
        Axis axis = Axis::NO_AXIS;
    };

    struct SpawnLocSkeleton {
        using DataType = SpawnLocSkeleton;
        static void Execute(const SpawnLocSkeleton* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(const SpawnLocSkeleton* data, const Distribution& distribution);

        AZ::Vector3 scale {1.f, 1.f, 1.f};
        MeshSampleType sampleType = MeshSampleType::BONE;
    private:
        static AZ::Vector3 SamplePointViaArea(const SpawnInfo& info);
    };

    struct SpawnLocCylinder {
        using DataType = SpawnLocCylinder;
        static void Execute(const SpawnLocCylinder* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(const SpawnLocCylinder* data, const Distribution& distribution);

        AZ::Vector3 center = { 0.f, 0.f, 0.f };
        float radius = 1.f; // length
        float height = 1.f;
        float angle = 360.f;
        float radiusThickness = 0.f;
        float aspectRatio = 1.f; // length / width
        Axis axis = Axis::Z_POSITIVE;
    };

    struct SpawnLocTorus {
        using DataType = SpawnLocTorus;
        static void Execute(const SpawnLocTorus* data, const SpawnInfo& info, Particle& particle);
        static void UpdateDistPtr(const SpawnLocTorus* data, const Distribution& distribution);

        AZ::Vector3 center = { 0.f, 0.f, 0.f };
        AZ::Vector3 torusAxis = { 0.f, 0.f, 1.f };
        AZ::Vector3 xAxis = { 1.f, 0.f, 0.f };
        AZ::Vector3 yAxis = { 0.f, 1.f, 0.f };
        float torusRadius = 5.f;  // the distance from the center of the tube to the center of the torus
        float tubeRadius = 1.f;   // the radius of the tube
    };
}

