/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/update/UpdateForce.h"
#include "core/math/Noise.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    void UpdateConstForce::Execute(const UpdateConstForce* data, const UpdateInfo& info, Particle& particle)
    {
        particle.velocity += CalcDistributionTickValue(data->force, info.baseInfo, particle) * info.tickTime;
    }

    void UpdateConstForce::UpdateDistPtr(UpdateConstForce* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->force, distribution);
    }

    void UpdateDragForce::Execute(const UpdateDragForce* data, const UpdateInfo& info, Particle& particle)
    {
        Vector3 drag =
            particle.velocity * (-CalcDistributionTickValue(data->dragCoefficient, info.baseInfo, particle));
        particle.velocity += drag * info.tickTime;
    }

    void UpdateDragForce::UpdateDistPtr(UpdateDragForce* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->dragCoefficient, distribution);
    }

    void UpdateCurlNoiseForce::Execute(const UpdateCurlNoiseForce* data, const UpdateInfo& info, Particle& particle)
    {
        Vector3 randomOffset = data->randomizationVector * Random::RandomRange(0.0f, 4096.0f) / 1000.0f;
        float frequencyScaled = data->noiseFrequency / 1000.0f;
        Vector3 pos = info.localSpace ? particle.localPosition : particle.globalPosition;
        Vector3 samplePosition = (pos + randomOffset) * frequencyScaled;
        if (data->panNoise) {
            samplePosition -= data->panNoiseField * info.tickTime;
        }
        Matrix4 jacobian = SimplexNoise::JacobianSimplexNoise(samplePosition * 125.0f); // 3 * Vector4
        Vector3 force {
            jacobian.m[1][2] - jacobian.m[2][1],
            jacobian.m[2][0] - jacobian.m[0][2],
            jacobian.m[0][1] - jacobian.m[1][0]
        };
        float length = force.Length();
        if (length < NOISE_COEFFICIENT) {
            force = Vector3(0.0f, 0.0f, 1.0f);
            length = 1.0f;
        } else {
            force /= length;
            length = Math::Clamp(length, 0.0f, 1.0f);
        }
        Vector3 sampledNoise = force * length * data->noiseStrength;
        particle.velocity += sampledNoise * info.tickTime;
    }

    void UpdateCurlNoiseForce::UpdateDistPtr(const UpdateCurlNoiseForce* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void UpdatePointForce::Execute(const UpdatePointForce* data, const UpdateInfo& info, Particle& particle)
    {
        Vector3 direction = data->useLocalSpace ?
            info.emitterTrans.TransformPoint(data->position) - particle.globalPosition :
            data->position - particle.globalPosition;
        if (direction.Length() > 0.f) {
            particle.velocity += direction / direction.Length() * (data->force * info.tickTime);
        }
    }

    void UpdatePointForce::UpdateDistPtr(const UpdatePointForce* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void UpdateVortexForce::Execute(const UpdateVortexForce* data, const UpdateInfo& info, Particle& particle)
    {
        if (data->vortexAxis == VEC3_ZERO) {
            return;
        }
        Vector3 axis = data->vortexAxis;
        Vector3 direction = data->origin - particle.localPosition;
        Vector3 dir = direction - direction.Dot(axis) * axis;

        float originPullVal = CalcDistributionTickValue(data->originPull, info.baseInfo, particle);
        float vortexRateVal = CalcDistributionTickValue(data->vortexRate, info.baseInfo, particle);

        if (originPullVal <= Math::EPSLON) {
            particle.velocity = (particle.velocity == VEC3_ZERO) ?
                vortexRateVal * dir.Cross(axis).Normalize() : particle.velocity;
        } else if (Math::Abs(vortexRateVal) <= Math::EPSLON) {
            particle.velocity += dir.Normalize() * (originPullVal * info.tickTime);
        } else {
            float step = originPullVal * Math::Abs(vortexRateVal) * info.tickTime / (Math::HALF_PI);
            step = (step - 1.f >= Math::EPSLON) ? 1.f : step;
            auto r = Math::Lerp<float>(dir.Length(),
                CalcDistributionTickValue(data->vortexRadius, info.baseInfo, particle), step);
            float theta = vortexRateVal * info.tickTime;
            Vector3 xAxis;
            Vector3 yAxis;
            GetAxis(axis, dir, xAxis, yAxis);
            Vector3 lastPosition = particle.localPosition;
            particle.localPosition = data->origin - direction.Dot(axis) * axis -
                xAxis * r * cos(theta) + yAxis * r * sin(theta);
            if (info.tickTime > Math::EPSLON) {
                particle.velocity = (particle.localPosition - lastPosition) / info.tickTime;
            }
            particle.localPosition -= particle.velocity * info.tickTime;
        }
    }

    void UpdateVortexForce::GetAxis(const Vector3& axis, Vector3 dir, Vector3& xAxis, Vector3& yAxis)
    {
        if (dir == VEC3_ZERO) {
            if (Math::Abs(axis.z) > Math::EPSLON) {
                xAxis.x = 1.f;
                xAxis.y = 1.f;
                xAxis.z = -(axis.x + axis.y) / axis.z;
            } else if (Math::Abs(axis.y) > Math::EPSLON) {
                xAxis.x = 1.f;
                xAxis.z = 1.f;
                xAxis.y = -(axis.x + axis.z) / axis.y;
            } else if (Math::Abs(axis.x) > Math::EPSLON) {
                xAxis.z = 1.f;
                xAxis.y = 1.f;
                xAxis.x = -(axis.y + axis.z) / axis.x;
            }
            xAxis = xAxis.Normalize();
            yAxis = xAxis.Cross(axis).Normalize();
        } else {
            xAxis = dir.Normalize();
            yAxis = xAxis.Cross(axis).Normalize();
        }
    }
    
    void UpdateVortexForce::UpdateDistPtr(UpdateVortexForce* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->originPull, distribution);
        UpdateDistributionPtr(data->vortexRate, distribution);
        UpdateDistributionPtr(data->vortexRadius, distribution);
    }
}
