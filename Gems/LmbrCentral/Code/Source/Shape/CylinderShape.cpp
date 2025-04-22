/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CylinderShapeComponent.h"

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Sfmt.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeDisplay.h>

#include "Cry_GeoDistance.h"
#include <random>

namespace LmbrCentral
{
    void CylinderShape::Reflect(AZ::ReflectContext* context)
    {
        CylinderShapeConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CylinderShape>()
                ->Version(1)
                ->Field("Configuration", &CylinderShape::m_cylinderShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CylinderShape>("Cylinder Shape", "Cylinder shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CylinderShape::m_cylinderShapeConfig, "Cylinder Configuration", "Cylinder shape configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CylinderShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        CylinderShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void CylinderShape::Deactivate()
    {
        CylinderShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void CylinderShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        AZStd::unique_lock lock(m_mutex);
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void CylinderShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_currentTransform = world;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::TransformChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    float CylinderShape::GetHeight() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_cylinderShapeConfig.m_height;
    }

    float CylinderShape::GetRadius() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_cylinderShapeConfig.m_radius;
    }

    void CylinderShape::SetHeight(float height)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_cylinderShapeConfig.m_height = height;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void CylinderShape::SetRadius(float radius)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_cylinderShapeConfig.m_radius = radius;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    static AZ::Vector3 SqrtVector3(const AZ::Vector3& v)
    {
        return AZ::Vector3(AZ::Simd::Vec3::Sqrt(v.GetSimdValue()));
    }

    // reference: http://www.iquilezles.org/www/articles/diskbbox/diskbbox.htm
    AZ::Aabb CylinderShape::GetEncompassingAabb() const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_cylinderShapeConfig, &m_mutex);

        const AZ::Vector3 base = m_intersectionDataCache.m_baseCenterPoint;
        const AZ::Vector3 top = m_intersectionDataCache.m_baseCenterPoint + m_intersectionDataCache.m_axisVector;
        const AZ::Vector3 axis = m_intersectionDataCache.m_axisVector;
        
        if (m_cylinderShapeConfig.m_height <= 0.0f || m_cylinderShapeConfig.m_radius <= 0.0f)
        {
            return AZ::Aabb::CreateFromPoint(base);
        }
        else
        {
            const AZ::Vector3 e = m_intersectionDataCache.m_radius *
                SqrtVector3(AZ::Vector3::CreateOne() - axis * axis / axis.Dot(axis));

            return AZ::Aabb::CreateFromMinMax(
                (base - e).GetMin(top - e),
                (base + e).GetMax(top + e));
        }
    }

    void CylinderShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const
    {
        AZStd::shared_lock lock(m_mutex);
        const AZ::Vector3 extent(m_cylinderShapeConfig.m_radius, m_cylinderShapeConfig.m_radius, m_cylinderShapeConfig.m_height * 0.5f);
        bounds = AZ::Aabb::CreateFromMinMax(-extent, extent);
        transform = m_currentTransform;
    }

    AZ::Vector3 CylinderShape::GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution) const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_cylinderShapeConfig, &m_mutex);

        const float minAngle = 0.0f;
        const float maxAngle = AZ::Constants::TwoPi;
        float halfHeight = m_intersectionDataCache.m_height * 0.5f;
        float maxRadius = m_intersectionDataCache.m_radius;

        // As std:normal_distribution requires a std:random_engine to be passed in, create one using a random seed that is guaranteed to be properly
        // random each time it is called
        time_t seedVal;
        seedVal = AZ::Sfmt::GetInstance().Rand64();
        std::default_random_engine generator;
        generator.seed(static_cast<unsigned int>(seedVal));

        float randomZ = 0.0f;
        float randomAngle = 0.0f;
        float randomRadius = 0.0f;

        // Points should be generated just inside the shape boundary
        halfHeight *= 0.999f;
        maxRadius *= 0.999f;

        switch (randomDistribution)
        {
        case AZ::RandomDistributionType::Normal:
            {
                const float meanRadius = 0.0f; //Mean for the radius should be 0. Negative radius is still valid
                const float meanZ = 0.0f; //We want the average height of generated points to be between the min height and the max height
                const float meanAngle = 0.0f; //There really isn't a good mean angle
                const float stdDevRadius = sqrtf(maxRadius); //StdDev of the radius will be the sqrt of the radius (the radius is the total variation)
                const float stdDevZ = sqrtf(halfHeight); //Same principle applied to the stdDev of the height
                const float stdDevAngle = sqrtf(maxAngle); //And the angle as well

                //Generate a random radius
                std::normal_distribution<float> normalDist = std::normal_distribution<float>(meanRadius, stdDevRadius);
                randomRadius = normalDist(generator);
                //Normal distributions can produce values higher than the desired max
                //This is very unlikely but we clamp anyway
                randomRadius = AZStd::clamp(randomRadius, -maxRadius, maxRadius);

                //Generate a random height
                normalDist = std::normal_distribution<float>(meanZ, stdDevZ);
                randomZ = normalDist(generator);
                randomZ = AZStd::clamp(randomZ, -halfHeight, halfHeight);

                //Generate a random angle along the circle
                normalDist = std::normal_distribution<float>(meanAngle, stdDevAngle);
                randomAngle = normalDist(generator);
                //Don't bother to clamp the angle because it doesn't matter if the angle is above 360 deg or below 0 deg
            }
            break;
        case AZ::RandomDistributionType::UniformReal:
            {
                std::uniform_real_distribution<float> uniformRealDist = std::uniform_real_distribution<float>(-maxRadius, maxRadius);
                randomRadius = uniformRealDist(generator);

                uniformRealDist = std::uniform_real_distribution<float>(-halfHeight, halfHeight);
                randomZ = uniformRealDist(generator);

                uniformRealDist = std::uniform_real_distribution<float>(minAngle, maxAngle);
                randomAngle = uniformRealDist(generator);
            }
            break;
        default:
            AZ_Warning("CylinderShape", false, "Unsupported random distribution type. Returning default vector (0,0,0)");
            break;
        }

        const AZ::Vector3 localRandomPoint = AZ::Vector3(
            randomRadius * cosf(randomAngle),
            randomRadius * sinf(randomAngle),
            randomZ);

        return m_currentTransform.TransformPoint(localRandomPoint);
    }

    bool CylinderShape::IsPointInside(const AZ::Vector3& point) const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_cylinderShapeConfig, &m_mutex);

        return AZ::Intersect::PointCylinder(
            m_intersectionDataCache.m_baseCenterPoint,
            m_intersectionDataCache.m_axisVector,
            powf(m_intersectionDataCache.m_height, 2.0f),
            powf(m_intersectionDataCache.m_radius, 2.0f),
            point);
    }

    float CylinderShape::DistanceSquaredFromPoint(const AZ::Vector3& point) const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_cylinderShapeConfig, &m_mutex);

        if (m_cylinderShapeConfig.m_height <= 0.0f || m_cylinderShapeConfig.m_radius <= 0.0f)
        {
            AZ::Vector3 diff = m_intersectionDataCache.m_baseCenterPoint - point;
            return diff.GetLengthSq();
        }
        return Distance::Point_CylinderSq(
            point, m_intersectionDataCache.m_baseCenterPoint,
            m_intersectionDataCache.m_baseCenterPoint + m_intersectionDataCache.m_axisVector,
            m_intersectionDataCache.m_radius);
    }

    bool CylinderShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_cylinderShapeConfig, &m_mutex);

        float t1 = 0.0f, t2 = 0.0f;
        const bool intersection =
            AZ::Intersect::IntersectRayCappedCylinder(
                src, dir, m_intersectionDataCache.m_baseCenterPoint,
                m_intersectionDataCache.m_axisVector.GetNormalizedSafe(), m_intersectionDataCache.m_height,
                m_intersectionDataCache.m_radius, t1, t2) > 0;
        distance = AZ::GetMin(t1, t2);
        return intersection;
    }

    void CylinderShape::CylinderIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const CylinderShapeConfig& configuration,
        [[maybe_unused]] const AZ::Vector3& currentNonUniformScale)
    {
        const float entityScale = currentTransform.GetUniformScale();
        m_axisVector = currentTransform.GetBasisZ().GetNormalizedSafe() * entityScale;
        m_baseCenterPoint = currentTransform.GetTranslation() - m_axisVector * (configuration.m_height * 0.5f);
        m_axisVector = m_axisVector * configuration.m_height;
        m_radius = configuration.m_radius * entityScale;
        m_height = configuration.m_height * entityScale;
    }

    void DrawCylinderShape(
        const ShapeDrawParams& shapeDrawParams, const CylinderShapeConfig& cylinderShapeConfig,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (shapeDrawParams.m_filled)
        {
            debugDisplay.SetColor(shapeDrawParams.m_shapeColor.GetAsVector4());
            debugDisplay.DrawSolidCylinder(
                AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(),
                cylinderShapeConfig.m_radius, cylinderShapeConfig.m_height, false);
        }

        debugDisplay.SetColor(shapeDrawParams.m_wireColor.GetAsVector4());
        debugDisplay.DrawWireCylinder(
            AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(),
            cylinderShapeConfig.m_radius, cylinderShapeConfig.m_height);
    }
} // namespace LmbrCentral
