/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/update/ParticleCollision.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {

    static float GetBound(const Vector3& max, const Vector3& min, const Vector3 particleSize)
    {
        Vector3 maxExtend = max * particleSize;
        Vector3 minExtend = min * particleSize;
        Vector3 center = (maxExtend + minExtend) * 0.5f;
        float radius = (maxExtend - center).GetLength();
        return radius;
    }

    static float CalculateSpriteRadius(const ParticleCollision& data, const Particle& particle, const UpdateInfo& info)
    {
        AZ::Vector2 size;
        if (info.front.IsClose(Vector3::CreateAxisZ()) || info.front.IsClose(-Vector3::CreateAxisZ()))
        {
            size = AZ::Vector2(particle.scale.GetElement(0), particle.scale.GetElement(1));
        }
        else if (info.front.IsClose(Vector3::CreateAxisY()) || info.front.IsClose(-Vector3::CreateAxisY()))
        {
            size = AZ::Vector2(particle.scale.GetElement(0), particle.scale.GetElement(2));
        }
        else
        {
            size = AZ::Vector2(particle.scale.GetElement(1), particle.scale.GetElement(2));
        }

        switch (data.collisionRadius.method) {
            case RadiusCalculationMethod::MAXIMUM_AXIS:
                return data.collisionRadius.radiusScale * AZStd::max(size.GetX(), size.GetY()) / 2.f;
            case RadiusCalculationMethod::MINIMUM_AXIS:
                return data.collisionRadius.radiusScale * AZStd::min(size.GetX(), size.GetY()) / 2.f;
            case RadiusCalculationMethod::BOUNDS:
            default:
                return data.collisionRadius.radiusScale * size.GetLength() / 2.f;
        }
    }

    static float CalculateMeshRadius(const ParticleCollision& data, const Particle& particle, const UpdateInfo& info)
    {
        switch (data.collisionRadius.method) {
            case RadiusCalculationMethod::MAXIMUM_AXIS:
                return data.collisionRadius.radiusScale * particle.scale.GetMaxElement() / 2.f;
            case RadiusCalculationMethod::MINIMUM_AXIS:
                return data.collisionRadius.radiusScale * particle.scale.GetMinElement() / 2.f;
            case RadiusCalculationMethod::BOUNDS:
            default:
                return GetBound(info.maxExtend, info.minExtend, particle.scale) * data.collisionRadius.radiusScale;
        }
    }

    static float CalculateCollisionRadius(const ParticleCollision& data,
        const Particle& particle, const UpdateInfo& info)
    {
        switch (data.collisionRadius.type) {
            case RadiusCalculationType::MESH:
                return CalculateMeshRadius(data, particle, info);
            case RadiusCalculationType::CUSTOM:
                return data.collisionRadius.radius * data.collisionRadius.radiusScale;
            case RadiusCalculationType::SPRITE:
            default:
                return CalculateSpriteRadius(data, particle, info);
        }
    }

    static Vector3 CalculateNormal(const Vector3 planeNormal, const UpdateInfo& info, float angleCof)
    {
        float th = 2.f * AZ::Constants::Pi * info.randomStream->Rand();
        // normal (0, 1] -> (0, Pi]
        float ap = (AZ::Constants::Pi * angleCof / 2.f) * info.randomStream->Rand();
        Vector3 vel(cos(th) * sin(ap), sin(th) * sin(ap), -cos(ap));
        Transform d = AZ::Transform::CreateLookAt(Vector3(0.0f), planeNormal, AZ::Transform::Axis::YPositive);
        return (Matrix3::CreateFromTransform(d) * vel).GetNormalized();
    }

    static void HandleCollision(const ParticleCollision* data, Particle& particle, const UpdateInfo& info,
        const CollisionParam& collision)
    {
        // before collision
        float distance;
        if (collision.lastDotPlane >= collision.radius) {
            if (collision.newDotPlane >= collision.radius) {
                return;
            }
            distance = collision.lastDotPlane - collision.radius;
        } else if (collision.lastDotPlane <= -collision.radius) {
            if (collision.newDotPlane <= -collision.radius) {
                return;
            }
            distance = collision.lastDotPlane + collision.radius;
        } else if (collision.lastDotPlane < 0) {
            distance = collision.lastDotPlane + collision.radius;
            particle.localPosition = particle.localPosition - distance * collision.localPlane.normal;
            if (particle.velocity.Dot(collision.localPlane.normal) < 0.0f) {
                return;
            }
            distance = 0.0f;
        } else {
            distance = collision.lastDotPlane - collision.radius;
            particle.localPosition = particle.localPosition - distance * collision.localPlane.normal;
            if (particle.velocity.Dot(collision.localPlane.normal) > 0.0f) {
                return;
            }
            distance = 0.0f;
        }

        Vector3 normal;
        float vDotN = particle.velocity.Dot(collision.localPlane.normal);
        float time = std::abs(distance / vDotN);
        Vector3 collisionPos = particle.localPosition + particle.velocity * time;
        collisionPos = info.localSpace ?
            info.emitterTrans.TransformPoint(collisionPos) :
            particle.spawnTrans.TransformPoint(collisionPos);
        particle.isCollided = true;
        particle.collisionTimeBeforeTick = info.tickTime - time;
        particle.collisionPosition = collisionPos;

        // colliding
        if (data->bounce.randomizeNormal <= AZ::Constants::FloatEpsilon) {
            normal = collision.localPlane.normal;
            particle.localPosition = particle.localPosition - 2.0f * distance * normal;
        } else {
            normal = CalculateNormal(collision.localPlane.normal, info, data->bounce.randomizeNormal);
            vDotN = particle.velocity.Dot(normal);
        }

        // after collision
        particle.velocity = particle.velocity - 2.0f * vDotN * normal;
        particle.velocity *= data->bounce.restitution;
    }

    void ParticleCollision::Execute(const ParticleCollision* data, const UpdateInfo& info, Particle& particle)
    {
        AZStd::vector<CollisionPlane> planes;
        (void)planes.emplace_back(CollisionPlane{ data->collisionPlane1.normal, data->collisionPlane1.position });
        if (data->useTwoPlane) {
            (void)planes.emplace_back(CollisionPlane{ data->collisionPlane2.normal, data->collisionPlane2.position });
        }

        CollisionParam collisionParam;
        collisionParam.radius = CalculateCollisionRadius(*data, particle, info);

        for (auto& plane : planes) {
            if (info.localSpace) {
                collisionParam.localPlane = plane;
            } else {
                Transform inverse = particle.spawnTrans.GetInverse();
                collisionParam.localPlane.position = inverse.TransformPoint(plane.position);
                collisionParam.localPlane.normal = inverse.TransformVector(plane.normal);
            }
        
            Vector3 lastDir = particle.localPosition - collisionParam.localPlane.position;
            Vector3 newDir = lastDir + particle.velocity * info.tickTime;
            collisionParam.lastDotPlane = collisionParam.localPlane.normal.Dot(lastDir);
            collisionParam.newDotPlane = collisionParam.localPlane.normal.Dot(newDir);
            HandleCollision(data, particle, info, collisionParam);
        }
    }

    void ParticleCollision::UpdateDistPtr(const ParticleCollision* data, const Distribution& distribution)
    {
        (void)data;
        (void)distribution;
    }
}
