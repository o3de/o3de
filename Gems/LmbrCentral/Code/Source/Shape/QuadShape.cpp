/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QuadShape.h"

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    QuadShape::QuadShape()
        : m_nonUniformScaleChangedHandler([this](const AZ::Vector3& scale) {this->OnNonUniformScaleChanged(scale); })
    {
    }

    void QuadShape::Reflect(AZ::ReflectContext* context)
    {
        QuadShapeConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<QuadShape>()
                ->Version(1)
                ->Field("Configuration", &QuadShape::m_quadShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<QuadShape>("Quad Shape", "Quad shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &QuadShape::m_quadShapeConfig, "Quad Configuration", "Quad shape configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void QuadShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_currentNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_currentNonUniformScale, m_entityId, &AZ::NonUniformScaleRequests::GetScale);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        QuadShapeComponentRequestBus::Handler::BusConnect(m_entityId);

        AZ::NonUniformScaleRequestBus::Event(m_entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent,
            m_nonUniformScaleChangedHandler);
    }

    void QuadShape::Deactivate()
    {
        m_nonUniformScaleChangedHandler.Disconnect();
        QuadShapeComponentRequestBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void QuadShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        AZStd::unique_lock lock(m_mutex);
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void QuadShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
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

    void QuadShape::OnNonUniformScaleChanged(const AZ::Vector3& scale)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_currentNonUniformScale = scale;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    const QuadShapeConfig& QuadShape::GetQuadConfiguration() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_quadShapeConfig;
    }

    void QuadShape::SetQuadWidth(float width)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_quadShapeConfig.m_width = width;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float QuadShape::GetQuadWidth() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_quadShapeConfig.m_width;
    }

    void QuadShape::SetQuadHeight(float height)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_quadShapeConfig.m_height = height;
            m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float QuadShape::GetQuadHeight() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_quadShapeConfig.m_height;
    }

    const AZ::Quaternion& QuadShape::GetQuadOrientation() const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_quadShapeConfig, &m_mutex, m_currentNonUniformScale);

        return m_intersectionDataCache.m_quaternion;
    }

    AZ::Aabb QuadShape::GetEncompassingAabb() const
    {
        AZStd::shared_lock lock(m_mutex);
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        auto corners = m_quadShapeConfig.GetCorners();

        for (AZ::Vector3& corner : corners)
        {
            aabb.AddPoint(m_currentTransform.TransformPoint(corner * m_currentNonUniformScale));
        }

        return aabb;
    }

    void QuadShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const
    {
        AZStd::shared_lock lock(m_mutex);
        bounds = AZ::Aabb::CreateCenterHalfExtents(
            AZ::Vector3(0.0f, 0.0f, 0.0f),
            AZ::Vector3(m_quadShapeConfig.m_width * 0.5f, m_quadShapeConfig.m_height * 0.5f, 0.0f) * m_currentNonUniformScale
        );
        transform = m_currentTransform;
    }

    bool QuadShape::IsPointInside([[maybe_unused]] const AZ::Vector3& point) const
    {
        return false; // 2D object cannot have points that are strictly inside in 3d space.
    }

    float QuadShape::DistanceSquaredFromPoint(const AZ::Vector3& point) const
    {
        AZStd::shared_lock lock(m_mutex);
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_quadShapeConfig, &m_mutex, m_currentNonUniformScale);

        // translate and rotate the point into the space of the quad.
        AZ::Vector3 tPoint = m_currentTransform.GetRotation().GetInverseFull().TransformVector(point - m_currentTransform.GetTranslation());

        float halfWidth = m_intersectionDataCache.m_scaledWidth * 0.5f;
        float halfHeight = m_intersectionDataCache.m_scaledHeight * 0.5f;

        // Get the distances in x, y, and z.
        float xDist = AZ::GetMax<float>(AZ::GetMax<float>(-halfWidth - tPoint.GetX(), 0.0f), tPoint.GetX() - halfWidth);
        float yDist = AZ::GetMax<float>(AZ::GetMax<float>(-halfHeight - tPoint.GetY(), 0.0f), tPoint.GetY() - halfHeight);
        float zDist = tPoint.GetZ();

        return xDist * xDist + yDist * yDist + zDist * zDist;
    }

    bool QuadShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const
    {
        AZStd::shared_lock lock(m_mutex);
        auto corners = m_quadShapeConfig.GetCorners();

        for (AZ::Vector3& corner : corners)
        {
            corner = m_currentTransform.TransformPoint(corner * m_currentNonUniformScale);
        }

        float floatDistance;
        bool hit = AZ::Intersect::IntersectRayQuad(src, dir, corners[0], corners[1], corners[2], corners[3], floatDistance) > 0;
        distance = floatDistance;
        return hit;
    }

    void QuadShape::QuadIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const QuadShapeConfig& configuration,
        [[maybe_unused]] const AZ::Vector3& currentNonUniformScale)
    {
        m_position = currentTransform.GetTranslation();
        m_quaternion = currentTransform.GetRotation();
        m_scaledWidth = configuration.m_width * currentTransform.GetUniformScale() * currentNonUniformScale.GetX();
        m_scaledHeight = configuration.m_height * currentTransform.GetUniformScale() * currentNonUniformScale.GetY();
    }

    void QuadShape::SetQuadConfiguration(const QuadShapeConfig& QuadShapeConfig)
    {
        m_quadShapeConfig = QuadShapeConfig;
    }

    const AZ::Transform& QuadShape::GetCurrentTransform() const
    {
        return m_currentTransform;
    }

    ShapeComponentConfig& QuadShape::ModifyShapeComponent()
    {
        return m_quadShapeConfig;
    }

    void DrawQuadShape(
        const ShapeDrawParams& shapeDrawParams, const QuadShapeConfig& quadConfig,
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& nonUniformScale)
    {
        // By default, debugDisplay draws quads facing the y axis, but we need it facing z.
        debugDisplay.PushMatrix(AZ::Transform::CreateRotationX(AZ::Constants::HalfPi));

        float scaledWidth = quadConfig.m_width * nonUniformScale.GetX();
        float scaledHeight = quadConfig.m_height * nonUniformScale.GetY();

        if (shapeDrawParams.m_filled)
        {
            debugDisplay.SetColor(shapeDrawParams.m_shapeColor.GetAsVector4());
            debugDisplay.DrawQuad(scaledWidth, scaledHeight, false);
        }

        debugDisplay.SetColor(shapeDrawParams.m_wireColor.GetAsVector4());
        debugDisplay.DrawWireQuad(scaledWidth, scaledHeight);

        debugDisplay.PopMatrix();

        // Draw line from center indicating facing direction.
        float normalLength = sqrt(scaledWidth * scaledWidth + scaledHeight * scaledHeight) * 0.1f;
        debugDisplay.DrawLine(AZ::Vector3::CreateZero(), AZ::Vector3(0.0f, 0.0f, normalLength));
    }
} // namespace LmbrCentral
