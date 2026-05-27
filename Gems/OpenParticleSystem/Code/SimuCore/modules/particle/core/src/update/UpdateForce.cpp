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
#include "core/math/Constants.h"

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
        AZ::Vector3 drag =
            particle.velocity * (-CalcDistributionTickValue(data->dragCoefficient, info.baseInfo, particle));
        particle.velocity += drag * info.tickTime;
    }

    void UpdateDragForce::UpdateDistPtr(UpdateDragForce* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->dragCoefficient, distribution);
    }

    void UpdateCurlNoiseForce::Execute(const UpdateCurlNoiseForce* data, const UpdateInfo& info, Particle& particle)
    {
        AZ::Vector3 randomOffset = data->randomizationVector * Random::RandomRange(0.0f, 4096.0f) / 1000.0f;
        float frequencyScaled = data->noiseFrequency / 1000.0f;
        AZ::Vector3 pos = info.localSpace ? particle.localPosition : particle.globalPosition;
        AZ::Vector3 samplePosition = (pos + randomOffset) * frequencyScaled;
        if (data->panNoise) {
            samplePosition -= data->panNoiseField * info.tickTime;
        }
        AZ::Matrix4x4 jacobian = SimplexNoise::JacobianSimplexNoise(samplePosition * 125.0f); // 3 * Vector4
        AZ::Vector3 force {
            jacobian.GetRow(1).GetElement(2) - jacobian.GetRow(2).GetElement(1),
            jacobian.GetRow(2).GetElement(0) - jacobian.GetRow(0).GetElement(2),
            jacobian.GetRow(0).GetElement(1) - jacobian.GetRow(1).GetElement(0)
        };
        float length = force.GetLength();
        if (length < NOISE_COEFFICIENT) {
            force = AZ::Vector3(0.0f, 0.0f, 1.0f);
            length = 1.0f;
        } else {
            force /= length;
            length = AZ::GetClamp(length, 0.0f, 1.0f);
        }
        AZ::Vector3 sampledNoise = force * length * data->noiseStrength;
        particle.velocity += sampledNoise * info.tickTime;
    }

    void UpdateCurlNoiseForce::UpdateDistPtr(const UpdateCurlNoiseForce* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void UpdatePointForce::Execute(const UpdatePointForce* data, const UpdateInfo& info, Particle& particle)
    {
        AZ::Vector3 direction = data->useLocalSpace ?
            info.emitterTrans.TransformPoint(data->position) - particle.globalPosition :
            data->position - particle.globalPosition;
        if (direction.GetLengthSq() > 0.f)
        {
            particle.velocity += direction / direction.GetLength() * (data->force * info.tickTime);
        }
    }

    void UpdatePointForce::UpdateDistPtr(const UpdatePointForce* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void UpdateVortexForce::Execute(const UpdateVortexForce* data, const UpdateInfo& info, Particle& particle)
    {
        if (data->vortexAxis == AZ::Vector3::CreateZero()) {
            return;
        }
        AZ::Vector3 axis = data->vortexAxis;
        AZ::Vector3 direction = data->origin - particle.localPosition;
        AZ::Vector3 dir = direction - direction.Dot(axis) * axis;

        float originPullVal = CalcDistributionTickValue(data->originPull, info.baseInfo, particle);
        float vortexRateVal = CalcDistributionTickValue(data->vortexRate, info.baseInfo, particle);

        if (originPullVal <= AZ::Constants::FloatEpsilon) {
            particle.velocity = (particle.velocity == AZ::Vector3::CreateZero()) ?
                vortexRateVal * dir.Cross(axis).GetNormalizedSafe() : particle.velocity;
        } else if (AZStd::abs(vortexRateVal) <= AZ::Constants::FloatEpsilon) {
            particle.velocity += dir.GetNormalizedSafe() * (originPullVal * info.tickTime);
        } else {
            float step = originPullVal * AZStd::abs(vortexRateVal) * info.tickTime / (AZ::Constants::HalfPi);
            step = (step - 1.f >= AZ::Constants::FloatEpsilon) ? 1.f : step;
            auto r = AZStd::lerp(dir.GetLength(),
                CalcDistributionTickValue(data->vortexRadius, info.baseInfo, particle), step);
            float theta = vortexRateVal * info.tickTime;
            AZ::Vector3 xAxis;
            AZ::Vector3 yAxis;
            GetAxis(axis, dir, xAxis, yAxis);
            AZ::Vector3 lastPosition = particle.localPosition;
            particle.localPosition = data->origin - direction.Dot(axis) * axis -
                xAxis * r * cos(theta) + yAxis * r * sin(theta);
            if (info.tickTime > AZ::Constants::FloatEpsilon) {
                particle.velocity = (particle.localPosition - lastPosition) / info.tickTime;
            }
            particle.localPosition -= particle.velocity * info.tickTime;
        }
    }

    void UpdateVortexForce::GetAxis(const AZ::Vector3& axis, AZ::Vector3 dir, AZ::Vector3& xAxis, AZ::Vector3& yAxis)
    {
        if (dir.IsClose(AZ::Vector3::CreateZero())) {
            if (AZStd::abs(axis.GetZ()) > AZ::Constants::FloatEpsilon) {
                xAxis = {
                    1.f,
                    1.f,
                    -(axis.GetX() + axis.GetY()) / axis.GetZ()
                };
            } else if (AZStd::abs(axis.GetY()) > AZ::Constants::FloatEpsilon) {
                xAxis = {
                    1.f,
                    -(axis.GetX() + axis.GetZ()) / axis.GetY(),
                    1.f
                };
            } else if (AZStd::abs(axis.GetX()) > AZ::Constants::FloatEpsilon) {
                xAxis = {
                    -(axis.GetY() + axis.GetZ()) / axis.GetX(),
                    1.f,
                    1.f
                };
            }
            xAxis.Normalize();
            yAxis = xAxis.Cross(axis).GetNormalizedSafe();
        } else {
            xAxis = dir.GetNormalized();
            yAxis = xAxis.Cross(axis).GetNormalizedSafe();
        }
    }
    
    void UpdateVortexForce::UpdateDistPtr(UpdateVortexForce* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->originPull, distribution);
        UpdateDistributionPtr(data->vortexRate, distribution);
        UpdateDistributionPtr(data->vortexRadius, distribution);
    }
}
