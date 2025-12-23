/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/spawn/SpawnLocation.h"

#include <algorithm>
#include "particle/core/ParticleHelper.h"
#include "core/math/Random.h"
#include "core/math/Constants.h"

namespace SimuCore::ParticleCore {
    void SpawnLocBox::Execute(const SpawnLocBox* data, const SpawnInfo& info, Particle& particle)
    {
        auto fn = [&info]() {
            return info.randomStream->RandRange(-0.5f, 0.5f);
        };
        AZ::Vector3 randomSize(fn(), fn(), fn());
        particle.localPosition += data->center + randomSize * data->size;
    }

    void SpawnLocBox::UpdateDistPtr(const SpawnLocBox* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void SpawnLocPoint::Execute(const SpawnLocPoint* data, const SpawnInfo& info, Particle& particle)
    {
        particle.localPosition += CalcDistributionTickValue(data->pos, info.baseInfo, particle);
    }

    void SpawnLocPoint::UpdateDistPtr(SpawnLocPoint* data, const Distribution& distribution)
    {
        UpdateDistributionPtr(data->pos, distribution);
    }

    void SpawnLocSphere::Execute(const SpawnLocSphere* data, const SpawnInfo& info, Particle& particle)
    {
        float th = AZ::DegToRad(data->angle) * info.randomStream->Rand();
        float arcCosAp = info.randomStream->RandRange(-1.f, 1.f);
        float ap = acos(arcCosAp);
        float tmp = info.randomStream->RandRange(1 - data->radiusThickness, 1.f);
        tmp = pow(tmp, 1.0f / POWER_CUBE);
        float x = cos(th) * sin(ap) * tmp;
        float y = sin(th) * sin(ap) * tmp;
        float z = cos(ap) * tmp;

        AZ::Vector3 vel;
        switch (data->axis) {
            case Axis::X_POSITIVE:
                vel = AZ::Vector3(abs(z), x, y);
                break;
            case Axis::X_NEGATIVE:
                vel = AZ::Vector3(-abs(z), x, y);
                break;
            case Axis::Y_POSITIVE:
                vel = AZ::Vector3(x, abs(z), y);
                break;
            case Axis::Y_NEGATIVE:
                vel = AZ::Vector3(x, -abs(z), y);
                break;
            case Axis::Z_POSITIVE:
                vel = AZ::Vector3(x, y, abs(z));
                break;
            case Axis::Z_NEGATIVE:
                vel = AZ::Vector3(x, y, -abs(z));
                break;
            case Axis::NO_AXIS:
            default:
                vel = AZ::Vector3(x, y, z);
                break;
        }
        particle.localPosition += data->center + vel * data->radius * data->ratio;
    }

    void SpawnLocSphere::UpdateDistPtr(const SpawnLocSphere* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void SpawnLocSkeleton::Execute(const SpawnLocSkeleton* data, const SpawnInfo& info, Particle& particle)
    {
        if (data->sampleType == MeshSampleType::VERTEX && info.vertexCount > 0)
        {
            AZ::u32 vertexIndex = Random::RandomRange(0u, info.vertexCount);
            particle.localPosition = data->scale * info.vertexStream[vertexIndex];
        }
        else if (data->sampleType == MeshSampleType::AREA && info.indiceCount > 0)
        {
            auto pointP = SamplePointViaArea(info);
            particle.localPosition = data->scale * pointP;
        }
        else if (data->sampleType == MeshSampleType::BONE && info.boneCount > 0)
        {
            AZ::u32 boneIndex = Random::RandomRange(0u, info.boneCount);
            particle.localPosition = data->scale * info.boneStream[boneIndex];
        }
    }

    /**
     * reference: https://doi.org/10.1145/571647.571648
     * @param info
     * @return A point random sampled via mesh face area
     */
    AZ::Vector3 SpawnLocSkeleton::SamplePointViaArea(const SpawnInfo& info)
    {
        auto cumulativeCnt = info.indiceCount / FACE_DIMENSION;
        auto totalArea = *(info.areaStream + cumulativeCnt - 1);
        auto sampleArea = totalArea * Random::Rand();
        auto it = std::lower_bound(info.areaStream, info.areaStream + cumulativeCnt, sampleArea);
        AZ::u32 faceIdx = static_cast<AZ::u32>(it - info.areaStream);

        AZ::u32 aIdx = info.indiceStream[faceIdx * 3];
        auto pointA = info.vertexStream[aIdx];

        AZ::u32 bIdx = info.indiceStream[faceIdx * 3 + 1];
        auto pointB = info.vertexStream[bIdx];

        AZ::u32 cIdx = info.indiceStream[faceIdx * 3 + 2];
        auto pointC = info.vertexStream[cIdx];

        auto alpha = Random::Rand();
        auto beta = Random::Rand();

        auto a = 1 - sqrt(beta);
        auto b = (sqrt(beta)) * (1 - alpha);
        auto c = sqrt(beta) * alpha;
        auto pointP = a * pointA + b * pointB + c * pointC;
        return pointP;
    }

    void SpawnLocSkeleton::UpdateDistPtr(const SpawnLocSkeleton* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void SpawnLocCylinder::Execute(const SpawnLocCylinder* data, const SpawnInfo& info, Particle& particle)
    {
        if (data->height < 0) {
            return;
        }

        float th = AZ::DegToRad(data->angle) * info.randomStream->Rand();
        float ap = AZ::Constants::Pi * info.randomStream->Rand();
        float tmp = info.randomStream->RandRange(1 - data->radiusThickness, 1.f);
        float x = sin(th) * data->radius  * data->aspectRatio * tmp;
        float y = cos(th) * data->radius * tmp;
        float z = ap * data->height;
        if (data->height - 0.f <= AZ::Constants::FloatEpsilon) {
            particle.localPosition += data->center + AZ::Vector3(x, y, z);
            return;
        }

        AZ::Vector3 vector3;
        switch (data->axis) {
            case Axis::Z_POSITIVE:
                vector3 = AZ::Vector3(x, y, z);
                break;
            case Axis::Z_NEGATIVE:
                vector3 = AZ::Vector3(x, y, -z);
                break;
            case Axis::X_POSITIVE:
                vector3 = AZ::Vector3(z, x, y);
                break;
            case Axis::X_NEGATIVE:
                vector3 = AZ::Vector3(-z, x, y);
                break;
            case Axis::Y_POSITIVE:
                vector3 = AZ::Vector3(x, z, y);
                break;
            case Axis::Y_NEGATIVE:
                vector3 = AZ::Vector3(x, -z, y);
                break;
            case Axis::NO_AXIS:
            default:
                vector3 = AZ::Vector3(x, y, z);
                break;
        }

        particle.localPosition += data->center + vector3;
    }

    void SpawnLocCylinder::UpdateDistPtr(const SpawnLocCylinder* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }

    void SpawnLocTorus::Execute(const SpawnLocTorus* data, const SpawnInfo& info, Particle& particle)
    {
        float th = 2 * AZ::Constants::Pi * info.randomStream->Rand();
        float ap = 2 * AZ::Constants::Pi * info.randomStream->Rand();
        float tmpR = data->torusRadius < 0 ? 0 : data->torusRadius;
        float r = info.randomStream->RandRange(0, data->tubeRadius);
        /**
         * x(\theta,\phi) = (R+r*cos\theta)*cos\phi,
         * y(\theta,\phi) = (R+r*cos\theta)*sin\phi,
         * z(\theta,\phi) = r*sin\theta,
         * \theta, \phi are angles which make a full circle, so their values start and end at the same point,
         * R is the distance from the center of the tube to the center of the torus,
         * r is the radius of the tube.
         * Angle \theta represents rotation around the tube,
         * whereas \phi represents rotation around the torus' axis of revolution.
         * R is known as the "major radius" and r is known as the "minor radius".
         * The ratio R divided by r is known as the "aspect ratio".
         */
        float x = (tmpR + r * cos(th)) * cos(ap);
        float y = (tmpR + r * cos(th)) * sin(ap);
        float z = r * sin(th);

        AZ::Vector3 vec = x * data->xAxis + y * data->yAxis + z * data->torusAxis;
        particle.localPosition += data->center + vec;
    }
    
    void SpawnLocTorus::UpdateDistPtr(const SpawnLocTorus* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }
}
