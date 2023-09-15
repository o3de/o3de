/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CapsuleShapeComponent.h"

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <CryCommon/Cry_GeoDistance.h>
#include <MathConversion.h>

namespace LmbrCentral
{
    const AZ::u32 g_capsuleDebugShapeSides = 16;
    const AZ::u32 g_capsuleDebugShapeCapSegments = 8;

    void CapsuleShape::Reflect(AZ::ReflectContext* context)
    {
        CapsuleShapeConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CapsuleShape>()
                ->Version(1)
                ->Field("Configuration", &CapsuleShape::m_capsuleShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CapsuleShape>("Capsule Shape", "Capsule shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShape::m_capsuleShapeConfig, "Capsule Configuration", "Capsule shape configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
        }
    }

    void CapsuleShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        CapsuleShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void CapsuleShape::Deactivate()
    {
        CapsuleShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void CapsuleShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        AZStd::unique_lock lock(m_mutex);
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void CapsuleShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
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

    void CapsuleShape::SetHeight(float height)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_capsuleShapeConfig.m_height = height;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void CapsuleShape::SetRadius(float radius)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_capsuleShapeConfig.m_radius = radius;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float CapsuleShape::GetHeight() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_capsuleShapeConfig.m_height;
    }

    float CapsuleShape::GetRadius() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_capsuleShapeConfig.m_radius;
    }

    CapsuleInternalEndPoints CapsuleShape::GetCapsulePoints() const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig, &m_mutex);

        return { m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_topPlaneCenterPoint };
    }

    AZ::Aabb CapsuleShape::GetEncompassingAabb() const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig, &m_mutex);

        const AZ::Aabb topAabb(AZ::Aabb::CreateCenterRadius(
            m_intersectionDataCache.m_topPlaneCenterPoint, m_intersectionDataCache.m_radius));
        AZ::Aabb baseAabb(AZ::Aabb::CreateCenterRadius(
            m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_radius));
        baseAabb.AddAabb(topAabb);
        return baseAabb;
    }

    void CapsuleShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const
    {
        float halfHeight = AZ::GetMax(m_capsuleShapeConfig.m_height * 0.5f, m_capsuleShapeConfig.m_radius);
        const AZ::Vector3 extent(m_capsuleShapeConfig.m_radius, m_capsuleShapeConfig.m_radius, halfHeight);
        bounds = AZ::Aabb::CreateFromMinMax(
            m_capsuleShapeConfig.m_translationOffset - extent, m_capsuleShapeConfig.m_translationOffset + extent);
        transform = m_currentTransform;
    }

    bool CapsuleShape::IsPointInside(const AZ::Vector3& point) const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig, &m_mutex);

        const float radiusSquared = m_intersectionDataCache.m_radius * m_intersectionDataCache.m_radius;

        // Check Bottom sphere
        if (AZ::Intersect::PointSphere(m_intersectionDataCache.m_basePlaneCenterPoint, radiusSquared, point))
        {
            return true;
        }

        // If the capsule is infact just a sphere then just stop (because the height of the cylinder <= 2 * radius of the cylinder)
        if (m_intersectionDataCache.m_isSphere)
        {
            return false;
        }

        // Check Top sphere
        if (AZ::Intersect::PointSphere(m_intersectionDataCache.m_topPlaneCenterPoint, radiusSquared, point))
        {
            return true;
        }

        // If its not in either sphere check the cylinder
        return AZ::Intersect::PointCylinder(
            m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_axisVector,
            m_intersectionDataCache.m_internalHeight * m_intersectionDataCache.m_internalHeight, radiusSquared, point);
    }

    float CapsuleShape::DistanceSquaredFromPoint(const AZ::Vector3& point) const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig, &m_mutex);

        const Lineseg lineSeg(
            AZVec3ToLYVec3(m_intersectionDataCache.m_basePlaneCenterPoint),
            AZVec3ToLYVec3(m_intersectionDataCache.m_topPlaneCenterPoint));

        float t = 0.0f;
        float distance = Distance::Point_Lineseg(AZVec3ToLYVec3(point), lineSeg, t);
        distance -= m_intersectionDataCache.m_radius;
        const float clampedDistance = AZStd::max(distance, 0.0f);
        return clampedDistance * clampedDistance;
    }

    bool CapsuleShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig, &m_mutex);

        if (m_intersectionDataCache.m_isSphere)
        {
            return AZ::Intersect::IntersectRaySphere(
                src, dir, m_intersectionDataCache.m_basePlaneCenterPoint,
                m_intersectionDataCache.m_radius, distance) > 0;
        }

        float t;
        const float rayLength = 1000.0f;
        const bool intersection = AZ::Intersect::IntersectSegmentCapsule(
            src, dir * rayLength, m_intersectionDataCache.m_basePlaneCenterPoint,
            m_intersectionDataCache.m_topPlaneCenterPoint, m_intersectionDataCache.m_radius, t) > 0;
        distance = rayLength * t;
        return intersection;
    }

    AZ::Vector3 CapsuleShape::GetTranslationOffset() const
    {
        return m_capsuleShapeConfig.m_translationOffset;
    }

    void CapsuleShape::SetTranslationOffset(const AZ::Vector3& translationOffset)
    {
        bool shapeChanged = false;
        {
            AZStd::unique_lock lock(m_mutex);
            if (!m_capsuleShapeConfig.m_translationOffset.IsClose(translationOffset))
            {
                m_capsuleShapeConfig.m_translationOffset = translationOffset;
                m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
                shapeChanged = true;
            }
        }

        if (shapeChanged)
        {
            ShapeComponentNotificationsBus::Event(
                m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
                ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        }
    }

    void CapsuleShape::CapsuleIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const CapsuleShapeConfig& configuration,
        [[maybe_unused]] const AZ::Vector3& currentNonUniformScale)
    {
        const float entityScale = currentTransform.GetUniformScale();
        m_axisVector = currentTransform.GetBasisZ().GetNormalizedSafe() * entityScale;
        const AZ::Vector3 offsetPosition = currentTransform.TransformPoint(configuration.m_translationOffset);

        const float internalCylinderHeight = configuration.m_height - configuration.m_radius * 2.0f;
        if (internalCylinderHeight > std::numeric_limits<float>::epsilon())
        {
            const AZ::Vector3 currentPositionToPlanesVector = m_axisVector * (internalCylinderHeight * 0.5f);
            m_topPlaneCenterPoint = offsetPosition + currentPositionToPlanesVector;
            m_basePlaneCenterPoint = offsetPosition - currentPositionToPlanesVector;
            m_axisVector = m_axisVector * internalCylinderHeight;
            m_isSphere = false;
        }
        else
        {
            m_basePlaneCenterPoint = offsetPosition;
            m_topPlaneCenterPoint = offsetPosition;
            m_isSphere = true;
        }

        // scale intersection data cache radius by entity transform for internal calculations
        m_radius = configuration.m_radius * entityScale;
        m_internalHeight = entityScale * internalCylinderHeight;
    }
} // namespace LmbrCentral
