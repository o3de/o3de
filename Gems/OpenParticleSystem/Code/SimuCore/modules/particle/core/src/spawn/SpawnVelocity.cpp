/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnVelocity.h"
#include "particle/core/ParticleHelper.h"

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
        Vector3 defVector = VEC3_UNIT_X;
        if (abs(abs(data->direction.Dot(defVector)) - 1.f) <= Math::EPSLON) {
            defVector = VEC3_UNIT_Y;
        }
        Vector3 normal = data->direction.Cross(defVector);

        // calculate default sector in the plane
        Vector3 axisUp = info.front.IsEqual(VEC3_UNIT_Z) ? VEC3_UNIT_Y : VEC3_UNIT_Z;
        Transform d;
        d.LookAt(Vector3(0.0f), normal, axisUp);
        Matrix3 rotationMatrix = Matrix3(d.ToMatrix());
        float theta = Math::AngleToRadians(data->centralAngle) * info.randomStream->Rand();
        Vector3 vel(sin(theta), cos(theta), 0.0f); // xoy z+
        Vector3 spawnDirection = rotationMatrix * vel;

        // calculate right and front axis of this plane, rotate the sector's centralLine to specified direction
        Vector3 normUp = axisUp;
        normUp = normUp.Normalize();
        if (std::abs(normal.Dot(normUp)) > ALMOST_ONE) {
            normUp = { normUp.z, normUp.x, normUp.y };
        }
        Vector3 right = normal.Cross(normUp);
        right = right.Normalize();
        Vector3 front = right.Cross(normal);
        front = front.Normalize();
        double angleNeedRotated = atan2(front.Cross(data->direction).Dot(normal), front.Dot(data->direction));
        if (angleNeedRotated < 0) {
            angleNeedRotated = 2.f * Math::PI + angleNeedRotated;
        }
        Quaternion q(normal, static_cast<float>(angleNeedRotated) - Math::AngleToRadians(data->centralAngle / 2.f));
        spawnDirection = q.RotateVector3(spawnDirection);

        // rotate around the sector direction
        if (abs(data->rotateAngle) > 0) {
            Quaternion rotQuaternion(data->direction, Math::AngleToRadians(data->rotateAngle));
            spawnDirection = rotQuaternion.RotateVector3(spawnDirection);
        }
        particle.velocity = spawnDirection * CalcDistributionTickValue(data->strength, info.baseInfo, particle);
    }

    void SpawnVelSector::UpdateDistPtr(SpawnVelSector* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->strength, distribution);
    }

    void SpawnVelCone::Execute(const SpawnVelCone* data, const SpawnInfo& info, Particle& particle)
    {
        float th = 2.f * Math::PI * info.randomStream->Rand();
        float ap = (Math::AngleToRadians(data->angle) / 2.f) * info.randomStream->Rand();
        Vector3 vel(cos(th) * sin(ap), sin(th) * sin(ap), -cos(ap)); // forward: -Z, right: +X, up: +Y
        Transform d;
        // keep the same, forward: -Z, right: +X, up: +Y
        d.LookAt(Vector3(0.0f), data->direction, Vector3(0.0f, 1.0f, 0.0f));
        Vector3 spawnDirection = Matrix3(d.ToMatrix()) * vel;
        particle.velocity = spawnDirection * CalcDistributionTickValue(data->strength, info.baseInfo, particle);
    }

    void SpawnVelCone::UpdateDistPtr(SpawnVelCone* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->strength, distribution);
    }

    void SpawnVelSphere::Execute(const SpawnVelSphere* data, const SpawnInfo& info, Particle& particle)
    {
        float th = 2.f * Math::PI * info.randomStream->Rand();
        float ap = Math::PI * info.randomStream->Rand();

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
        if (direction.Length() > 0) {
            particle.velocity =
                direction / direction.Length() * CalcDistributionTickValue(data->rate, info.baseInfo, particle);
        }
    };

    void SpawnVelConcentrate::UpdateDistPtr(SpawnVelConcentrate* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->rate, distribution);
    }
}
