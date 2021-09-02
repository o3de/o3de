/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AxisAlignedBoxShape.h"

#include <AzCore/Math/Color.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeDisplay.h>
#include <random>

namespace LmbrCentral
{
    AxisAlignedBoxShape::AxisAlignedBoxShape()
        : BoxShape()
    {
    }

    void AxisAlignedBoxShape::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AxisAlignedBoxShape, BoxShape>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AxisAlignedBoxShape>("Axis Aligned Box Shape", "Axis Aligned Box shape configuration parameters")
                ;
            }
        }
    }

    void AxisAlignedBoxShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_currentTransform.SetRotation(AZ::Quaternion::CreateIdentity());
        m_currentNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_currentNonUniformScale, m_entityId, &AZ::NonUniformScaleRequests::GetScale);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        BoxShapeComponentRequestsBus::Handler::BusConnect(m_entityId);

        AZ::NonUniformScaleRequestBus::Event(m_entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent,
            m_nonUniformScaleChangedHandler);
    }

    void AxisAlignedBoxShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_currentTransform.SetRotation(AZ::Quaternion::CreateIdentity());
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::TransformChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }
    #if 0
    void AxisAlignedBoxShape::OnNonUniformScaleChanged(const AZ::Vector3& scale)
    {
        m_currentNonUniformScale = scale;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void AxisAlignedBoxShape::SetBoxDimensions(const AZ::Vector3& dimensions)
    {
        m_boxShapeConfig.m_dimensions = dimensions;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    AZ::Aabb AxisAlignedBoxShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);
        return m_intersectionDataCache.m_aabb;
    }

    void AxisAlignedBoxShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds)
    {
        const AZ::Vector3 extent(m_boxShapeConfig.m_dimensions * m_currentNonUniformScale * 0.5f);
        bounds = AZ::Aabb::CreateFromMinMax(-extent, extent);
        transform = m_currentTransform;
    }

    bool AxisAlignedBoxShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);

        return m_intersectionDataCache.m_aabb.Contains(point);
    }

    float AxisAlignedBoxShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);

        return m_intersectionDataCache.m_aabb.GetDistanceSq(point);
    }

    bool AxisAlignedBoxShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);

        const float rayLength = 1000.0f;
        AZ::Vector3 scaledDir = dir * rayLength;
        AZ::Vector3 startNormal;
        float end;

        float t;
        const bool intersection = AZ::Intersect::IntersectRayAABB(
                                      src, scaledDir, scaledDir.GetReciprocal(), m_intersectionDataCache.m_aabb, t, end, startNormal) > 0;

        distance = rayLength * t;
        return intersection;
    }

    AZ::Vector3 AxisAlignedBoxShape::GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_boxShapeConfig, m_currentNonUniformScale);

        float x = 0;
        float y = 0;
        float z = 0;

        // Points should be generated just inside the shape boundary
        constexpr float insideMargin = 0.999f;

        AZ::Vector3 boxMin = m_intersectionDataCache.m_scaledDimensions * -0.5f * insideMargin;
        AZ::Vector3 boxMax = m_intersectionDataCache.m_scaledDimensions * 0.5f * insideMargin;

        // As std:normal_distribution requires a std:random_engine to be passed in,
        // As std:normal_distribution requires a std:random_engine to be passed in, create one using a random seed that is guaranteed to be properly
        // random each time it is called
        time_t seedVal;
        seedVal = AZ::Sfmt::GetInstance().Rand64();
        std::default_random_engine generator;
        generator.seed(static_cast<unsigned int>(seedVal));

        switch(randomDistribution)
        {
        case AZ::RandomDistributionType::Normal:
            {
                const float mean = 0.0f; //Mean will always be 0

                //stdDev will be the sqrt of the max value (which is the total variation)
                float stdDev = sqrtf(boxMax.GetX());
                std::normal_distribution<float> normalDist =
                    std::normal_distribution<float>(mean, stdDev);
                x = normalDist(generator);
                //Normal distributions can sometimes produce values outside of our desired range
                //We just need to clamp
                x = AZStd::clamp<float>(x, boxMin.GetX(), boxMax.GetX());

                stdDev = sqrtf(boxMax.GetY());
                normalDist = std::normal_distribution<float>(mean, stdDev);
                y = normalDist(generator);

                y = AZStd::clamp<float>(y, boxMin.GetY(), boxMax.GetY());

                stdDev = sqrtf(boxMax.GetZ());
                normalDist = std::normal_distribution<float>(mean, stdDev);
                z = normalDist(generator);

                z = AZStd::clamp<float>(z, boxMin.GetZ(), boxMax.GetZ());
            }
            break;
        case AZ::RandomDistributionType::UniformReal:
            {
                std::uniform_real_distribution<float> uniformRealDist =
                    std::uniform_real_distribution<float>(boxMin.GetX(), boxMax.GetX());
                x = uniformRealDist(generator);

                uniformRealDist = std::uniform_real_distribution<float>(boxMin.GetY(), boxMax.GetY());
                y = uniformRealDist(generator);

                uniformRealDist = std::uniform_real_distribution<float>(boxMin.GetZ(), boxMax.GetZ());
                z = uniformRealDist(generator);
            }
            break;
        default:
            AZ_Warning("AxisAlignedBoxShape", false, "Unsupported random distribution type. Returning default vector (0,0,0)");
            break;
        }

        // transform to world space
        return m_currentTransform.TransformPoint(AZ::Vector3(x, y, z));
    }

    void AxisAlignedBoxShape::BoxIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const BoxShapeConfig& configuration, const AZ::Vector3& currentNonUniformScale)
    {
        AZ::Transform worldFromLocalNormalized = currentTransform;
        const float entityScale = worldFromLocalNormalized.ExtractUniformScale();

        m_currentPosition = worldFromLocalNormalized.GetTranslation();
        m_scaledDimensions = configuration.m_dimensions * currentNonUniformScale * entityScale;

        AZ::Vector3 boxMin = m_scaledDimensions * -0.5f;
        boxMin = worldFromLocalNormalized.TransformPoint(boxMin);

        AZ::Vector3 boxMax = m_scaledDimensions * 0.5f;
        boxMax = worldFromLocalNormalized.TransformPoint(boxMax);

        m_aabb = AZ::Aabb::CreateFromMinMax(boxMin, boxMax);
    }
    #endif
} // namespace LmbrCentral
