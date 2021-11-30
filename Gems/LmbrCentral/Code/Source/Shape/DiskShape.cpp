/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DiskShape.h"

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void DiskShape::Reflect(AZ::ReflectContext* context)
    {
        DiskShapeConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DiskShape>()
                ->Version(1)
                ->Field("Configuration", &DiskShape::m_diskShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DiskShape>("Disk Shape", "Disk shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DiskShape::m_diskShapeConfig, "Disk Configuration", "Disk shape configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void DiskShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        DiskShapeComponentRequestBus::Handler::BusConnect(m_entityId);
    }

    void DiskShape::Deactivate()
    {
        DiskShapeComponentRequestBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void DiskShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void DiskShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::TransformChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    DiskShapeConfig DiskShape::GetDiskConfiguration()
    {
        return m_diskShapeConfig;
    }

    void DiskShape::SetRadius(float radius)
    {
        m_diskShapeConfig.m_radius = radius;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float DiskShape::GetRadius()
    {
        return m_diskShapeConfig.m_radius;
    }

    const AZ::Vector3& DiskShape::GetNormal()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_diskShapeConfig);
        return m_intersectionDataCache.m_normal;
    }

    AZ::Aabb DiskShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_diskShapeConfig);

        const AZ::Vector3& normal = m_intersectionDataCache.m_normal;
        const float radius = m_intersectionDataCache.m_radius;

        AZ::Vector3 halfsize;
        halfsize.SetX(radius * sqrt(AZStd::max<float>(0.0f, 1.0f - normal.GetX() * normal.GetX())));
        halfsize.SetY(radius * sqrt(AZStd::max<float>(0.0f, 1.0f - normal.GetY() * normal.GetY())));
        halfsize.SetZ(radius * sqrt(AZStd::max<float>(0.0f, 1.0f - normal.GetZ() * normal.GetZ())));

        return AZ::Aabb::CreateCenterHalfExtents(m_currentTransform.GetTranslation(), halfsize);
    }

    void DiskShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds)
    {
        const float radius = m_diskShapeConfig.m_radius;
        bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-radius, -radius, 0.0f), AZ::Vector3(radius, radius, 0.0f));
        transform = m_currentTransform;
    }

    bool DiskShape::IsPointInside([[maybe_unused]] const AZ::Vector3& point)
    {
        return false; // 2D object cannot have points that are strictly inside in 3d space.
    }

    float DiskShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_diskShapeConfig);

        // Find closest point to the plane the disk is on
        AZ::Plane plane = AZ::Plane::CreateFromNormalAndPoint(m_intersectionDataCache.m_normal, m_currentTransform.GetTranslation());
        AZ::Vector3 closestPointToPlane;
        AZ::Intersect::ClosestPointPlane(point, plane, closestPointToPlane);

        // Find the point on the disk closest to the point on the plane
        AZ::Vector3 centerToClosestPoint = closestPointToPlane - m_currentTransform.GetTranslation();
        if (centerToClosestPoint.GetLengthSq() > m_intersectionDataCache.m_radius * m_intersectionDataCache.m_radius)
        {
            centerToClosestPoint.SetLength(m_intersectionDataCache.m_radius);
        }
        AZ::Vector3 closestPoint = m_currentTransform.GetTranslation() + centerToClosestPoint;

        // Return the distance squared between the closest point on the disk to the provided point.
        return closestPoint.GetDistanceSq(point);
    }

    bool DiskShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_diskShapeConfig);

        return AZ::Intersect::IntersectRayDisk(
            src, dir, m_intersectionDataCache.m_position, m_intersectionDataCache.m_radius, m_intersectionDataCache.m_normal, distance);
    }

    void DiskShape::DiskIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const DiskShapeConfig& configuration,
        [[maybe_unused]] const AZ::Vector3& currentNonUniformScale)
    {
        m_position = currentTransform.GetTranslation();
        m_normal = currentTransform.GetBasisZ().GetNormalized();
        m_radius = configuration.m_radius * currentTransform.GetUniformScale();
    }

    const DiskShapeConfig& DiskShape::GetDiskConfiguration() const
    {
        return m_diskShapeConfig;
    }

    void DiskShape::SetDiskConfiguration(const DiskShapeConfig& diskShapeConfig)
    {
        m_diskShapeConfig = diskShapeConfig;
    }

    const AZ::Transform& DiskShape::GetCurrentTransform() const
    {
        return m_currentTransform;
    }

    ShapeComponentConfig& DiskShape::ModifyShapeComponent()
    {
        return m_diskShapeConfig;
    }

    void DrawDiskShape(
        const ShapeDrawParams& shapeDrawParams, const DiskShapeConfig& diskConfig,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (shapeDrawParams.m_filled)
        {
            debugDisplay.SetColor(shapeDrawParams.m_shapeColor.GetAsVector4());
            debugDisplay.DrawDisk(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(), diskConfig.m_radius);
        }

        debugDisplay.SetColor(shapeDrawParams.m_wireColor.GetAsVector4());
        debugDisplay.DrawWireDisk(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(), diskConfig.m_radius);
    }
}
