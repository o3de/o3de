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

namespace SimuCore::ParticleCore {
    void SpawnLocBox::Execute(const SpawnLocBox* data, const SpawnInfo& info, Particle& particle)
    {
        auto fn = [&info]() {
            return info.randomStream->RandRange(-0.5f, 0.5f);
        };
        Vector3 randomSize(fn(), fn(), fn());
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
        float th = Math::AngleToRadians(data->angle) * info.randomStream->Rand();
        float arcCosAp = info.randomStream->RandRange(-1.f, 1.f);
        float ap = acos(arcCosAp);
        float tmp = info.randomStream->RandRange(1 - data->radiusThickness, 1.f);
        tmp = pow(tmp, 1.0f / POWER_CUBE);
        float x = cos(th) * sin(ap) * tmp;
        float y = sin(th) * sin(ap) * tmp;
        float z = cos(ap) * tmp;

        Vector3 vel;
        switch (data->axis) {
            case Axis::X_POSITIVE:
                vel = Vector3(abs(z), x, y);
                break;
            case Axis::X_NEGATIVE:
                vel = Vector3(-abs(z), x, y);
                break;
            case Axis::Y_POSITIVE:
                vel = Vector3(x, abs(z), y);
                break;
            case Axis::Y_NEGATIVE:
                vel = Vector3(x, -abs(z), y);
                break;
            case Axis::Z_POSITIVE:
                vel = Vector3(x, y, abs(z));
                break;
            case Axis::Z_NEGATIVE:
                vel = Vector3(x, y, -abs(z));
                break;
            case Axis::NO_AXIS:
            default:
                vel = Vector3(x, y, z);
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
        if (data->sampleType == MeshSampleType::VERTEX && info.vertexCount > 0) {
            uint32_t vertexIndex = Random::RandomRange(0u, info.vertexCount);
            particle.localPosition.x = data->scale.x * info.vertexStream[vertexIndex].x;
            particle.localPosition.y = data->scale.y * info.vertexStream[vertexIndex].y;
            particle.localPosition.z = data->scale.z * info.vertexStream[vertexIndex].z;
        } else if (data->sampleType == MeshSampleType::AREA && info.indiceCount > 0) {
            auto pointP = SamplePointViaArea(info);
            particle.localPosition.x = data->scale.x * pointP.x;
            particle.localPosition.y = data->scale.y * pointP.y;
            particle.localPosition.z = data->scale.z * pointP.z;
        } else if (data->sampleType == MeshSampleType::BONE && info.boneCount > 0) {
            uint32_t boneIndex = Random::RandomRange(0u, info.boneCount);
            particle.localPosition.x = data->scale.x * info.boneStream[boneIndex].x;
            particle.localPosition.y = data->scale.y * info.boneStream[boneIndex].y;
            particle.localPosition.z = data->scale.z * info.boneStream[boneIndex].z;
        }
    }

    /**
     * reference: https://doi.org/10.1145/571647.571648
     * @param info
     * @return A point random sampled via mesh face area
     */
    Vector3 SpawnLocSkeleton::SamplePointViaArea(const SpawnInfo& info)
    {
        auto cumulativeCnt = info.indiceCount / FACE_DIMENSION;
        auto totalArea = *(info.areaStream + cumulativeCnt - 1);
        auto sampleArea = totalArea * Random::Rand();
        auto it = std::lower_bound(info.areaStream, info.areaStream + cumulativeCnt, sampleArea);
        uint32_t faceIdx = static_cast<uint32_t>(it - info.areaStream);

        uint32_t aIdx = info.indiceStream[faceIdx * 3];
        auto pointA = info.vertexStream[aIdx];

        uint32_t bIdx = info.indiceStream[faceIdx * 3 + 1];
        auto pointB = info.vertexStream[bIdx];

        uint32_t cIdx = info.indiceStream[faceIdx * 3 + 2];
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

        float th = Math::AngleToRadians(data->angle) * info.randomStream->Rand();
        float ap = Math::PI * info.randomStream->Rand();
        float tmp = info.randomStream->RandRange(1 - data->radiusThickness, 1.f);
        float x = sin(th) * data->radius  * data->aspectRatio * tmp;
        float y = cos(th) * data->radius * tmp;
        float z = ap * data->height;
        if (data->height - 0.f <= Math::EPSLON) {
            particle.localPosition += data->center + Vector3(x, y, z);
            return;
        }

        Vector3 vector3;
        switch (data->axis) {
            case Axis::Z_POSITIVE:
                vector3 = Vector3(x, y, z);
                break;
            case Axis::Z_NEGATIVE:
                vector3 = Vector3(x, y, -z);
                break;
            case Axis::X_POSITIVE:
                vector3 = Vector3(z, x, y);
                break;
            case Axis::X_NEGATIVE:
                vector3 = Vector3(-z, x, y);
                break;
            case Axis::Y_POSITIVE:
                vector3 = Vector3(x, z, y);
                break;
            case Axis::Y_NEGATIVE:
                vector3 = Vector3(x, -z, y);
                break;
            case Axis::NO_AXIS:
            default:
                vector3 = Vector3(x, y, z);
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
        float th = 2 * Math::PI * info.randomStream->Rand();
        float ap = 2 * Math::PI * info.randomStream->Rand();
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

        Vector3 vec = x * data->xAxis + y * data->yAxis + z * data->torusAxis;
        particle.localPosition += data->center + vec;
    }
    
    void SpawnLocTorus::UpdateDistPtr(const SpawnLocTorus* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }
}
