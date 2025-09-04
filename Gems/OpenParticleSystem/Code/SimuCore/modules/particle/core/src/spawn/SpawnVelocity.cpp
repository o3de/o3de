/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnVelocity.h"
#include "particle/core/ParticleHelper.h"
#include "core/math/Constants.h"

namespace SimuCore::ParticleCore {
    void SpawnVelDirection::Execute(const SpawnVelDirection* data, const SpawnInfo& info, Particle& particle)
    {
        particle.velocity = CalcDistributionTickValue(data->strength, info.baseInfo, particle) *
            CalcDistributionTickValue(data->direction, info.baseInfo, particle);
    }

    void SpawnVelDirection::UpdateDistPtr(SpawnVelDirection* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->strength, distribution);
        UpdateDistributionPtr(data->direction, distribution);
    }

    void SpawnVelSector::Execute(const SpawnVelSector* data, const SpawnInfo& info, Particle& particle)
    {
        // define a plane, the given direction is in the plane
        Vector3 defVector = Vector3::CreateAxisX();
        if (abs(abs(data->direction.Dot(defVector)) - 1.f) <= AZ::Constants::FloatEpsilon) {
            defVector = Vector3::CreateAxisY();
        }
        Vector3 normal = data->direction.Cross(defVector);

        // calculate default sector in the plane
        Vector3 axisUp = info.front.IsClose(Vector3::CreateAxisZ()) ? Vector3::CreateAxisY() : Vector3::CreateAxisZ();
        Transform d = AZ::Transform::CreateLookAt(Vector3(0.0f), normal, info.front.IsClose(Vector3::CreateAxisZ()) ? AZ::Transform::Axis::YPositive : AZ::Transform::Axis::ZPositive);
        Matrix3 rotationMatrix = Matrix3::CreateFromTransform(d);
        float theta = AZ::DegToRad(data->centralAngle) * info.randomStream->Rand();
        Vector3 vel(sin(theta), cos(theta), 0.0f); // xoy z+
        Vector3 spawnDirection = rotationMatrix * vel;

        // calculate right and front axis of this plane, rotate the sector's centralLine to specified direction
        Vector3 normUp = axisUp;
        normUp.Normalize();
        if (std::abs(normal.Dot(normUp)) > ALMOST_ONE) {
            normUp = { normUp.GetZ(), normUp.GetX(), normUp.GetY() };
        }
        Vector3 right = normal.Cross(normUp);
        right.Normalize();
        Vector3 front = right.Cross(normal);
        front.Normalize();
        double angleNeedRotated = atan2(front.Cross(data->direction).Dot(normal), front.Dot(data->direction));
        if (angleNeedRotated < 0) {
            angleNeedRotated = 2.f * AZ::Constants::Pi + angleNeedRotated;
        }
        Quaternion q(normal, static_cast<float>(angleNeedRotated) - AZ::DegToRad(data->centralAngle / 2.f));
        spawnDirection = q.TransformVector(spawnDirection);

        // rotate around the sector direction
        if (abs(data->rotateAngle) > 0) {
            Quaternion rotQuaternion(data->direction, AZ::DegToRad(data->rotateAngle));
            spawnDirection = rotQuaternion.TransformVector(spawnDirection);
        }
        particle.velocity = spawnDirection * CalcDistributionTickValue(data->strength, info.baseInfo, particle);
    }

    void SpawnVelSector::UpdateDistPtr(SpawnVelSector* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->strength, distribution);
    }

    void SpawnVelCone::Execute(const SpawnVelCone* data, const SpawnInfo& info, Particle& particle)
    {
        float th = 2.f * AZ::Constants::Pi * info.randomStream->Rand();
        float ap = (AZ::DegToRad(data->angle) / 2.f) * info.randomStream->Rand();
        Vector3 vel(cos(th) * sin(ap), sin(th) * sin(ap), -cos(ap)); // forward: -Z, right: +X, up: +Y
        Transform d = AZ::Transform::CreateLookAt(Vector3(0.0f), data->direction, AZ::Transform::Axis::YPositive);
        Vector3 spawnDirection = Matrix3::CreateFromTransform(d) * vel;
        particle.velocity = spawnDirection * CalcDistributionTickValue(data->strength, info.baseInfo, particle);
    }

    void SpawnVelCone::UpdateDistPtr(SpawnVelCone* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->strength, distribution);
    }

    void SpawnVelSphere::Execute(const SpawnVelSphere* data, const SpawnInfo& info, Particle& particle)
    {
        float th = 2.f * AZ::Constants::Pi * info.randomStream->Rand();
        float ap = AZ::Constants::Pi * info.randomStream->Rand();

        Vector3 vel(cos(th) * sin(ap), cos(ap), sin(th) * sin(ap));
        particle.velocity = vel * CalcDistributionTickValue(data->strength, info.baseInfo, particle);
    }
    
    void SpawnVelSphere::UpdateDistPtr(SpawnVelSphere* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->strength, distribution);
    }

    void SpawnVelConcentrate::Execute(const SpawnVelConcentrate* data, const SpawnInfo& info, Particle& particle)
    {
        Vector3 direction = data->centre - particle.localPosition;
        if (direction.GetLengthSq() > 0) {
            particle.velocity =
                direction / direction.GetLength() * CalcDistributionTickValue(data->rate, info.baseInfo, particle);
        }
    };

    void SpawnVelConcentrate::UpdateDistPtr(SpawnVelConcentrate* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->rate, distribution);
    }
}
