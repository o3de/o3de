/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SphereShapeComponent.h"

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void SphereShape::Reflect(AZ::ReflectContext* context)
    {
        SphereShapeConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SphereShape>()
                ->Version(1)
                ->Field("Configuration", &SphereShape::m_sphereShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SphereShape>("Sphere Shape", "Sphere shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SphereShape::m_sphereShapeConfig, "Sphere Configuration", "Sphere shape configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
        }
    }

    void SphereShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        SphereShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void SphereShape::Deactivate()
    {
        SphereShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void SphereShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        AZStd::unique_lock lock(m_mutex);
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void SphereShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
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

    void SphereShape::SetRadius(float radius)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_sphereShapeConfig.m_radius = radius;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float SphereShape::GetRadius()
    {
        AZStd::shared_lock lock(m_mutex);
        return m_sphereShapeConfig.m_radius;
    }

    AZ::Aabb SphereShape::GetEncompassingAabb()
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_sphereShapeConfig, m_mutex);

        return AZ::Aabb::CreateCenterRadius(m_currentTransform.GetTranslation(), m_intersectionDataCache.m_radius);
    }

    void SphereShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds)
    {
        AZStd::shared_lock lock(m_mutex);
        bounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), m_sphereShapeConfig.m_radius);
        transform = m_currentTransform;
    }

    bool SphereShape::IsPointInside(const AZ::Vector3& point)
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_sphereShapeConfig, m_mutex);

        return AZ::Intersect::PointSphere(
            m_intersectionDataCache.m_position, powf(m_intersectionDataCache.m_radius, 2.0f), point);
    }

    float SphereShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_sphereShapeConfig, m_mutex);

        const AZ::Vector3 pointToSphereCenter = m_intersectionDataCache.m_position - point;
        const float distance = pointToSphereCenter.GetLength() - m_intersectionDataCache.m_radius;
        return powf(AZStd::max(distance, 0.0f), 2.0f);
    }

    bool SphereShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_sphereShapeConfig, m_mutex);

        return AZ::Intersect::IntersectRaySphere(
            src, dir, m_intersectionDataCache.m_position, m_intersectionDataCache.m_radius, distance) > 0;
    }

    void SphereShape::SphereIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const SphereShapeConfig& configuration,
        [[maybe_unused]] const AZ::Vector3& currentNonUniformScale)
    {
        m_position = currentTransform.GetTranslation();
        m_radius = configuration.m_radius * currentTransform.GetUniformScale();
    }

    void DrawSphereShape(
        const ShapeDrawParams& shapeDrawParams, const SphereShapeConfig& sphereConfig,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (shapeDrawParams.m_filled)
        {
            debugDisplay.SetColor(shapeDrawParams.m_shapeColor.GetAsVector4());
            debugDisplay.DrawBall(AZ::Vector3::CreateZero(), sphereConfig.m_radius);
        }

        debugDisplay.SetColor(shapeDrawParams.m_wireColor.GetAsVector4());
        debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), sphereConfig.m_radius);
    }
} // namespace LmbrCentral
