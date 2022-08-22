/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Geometry2DUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Brushes/PaintBrush.h>
#include <AzToolsFramework/Brushes/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    void PaintBrush::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PaintBrush>()
                ->Version(1)
                ->Field("Radius", &PaintBrush::m_radius)
                ->Field("Intensity", &PaintBrush::m_intensity)
                ->Field("Opacity", &PaintBrush::m_opacity);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PaintBrush>("Paint Brush", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrush::m_radius, "Radius", "Radius of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrush::OnRadiusChange)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrush::m_intensity, "Intensity", "Intensity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrush::OnIntensityChange)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrush::m_opacity, "Opacity", "Opacity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrush::OnOpacityChange);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PaintBrush>()
                ->Constructor()
                ->Property("radius", BehaviorValueGetter(&PaintBrush::m_radius), nullptr)
                ->Property("intensity", BehaviorValueGetter(&PaintBrush::m_intensity), nullptr)
                ->Property("opacity", BehaviorValueGetter(&PaintBrush::m_opacity), nullptr);
        }
    }

    AZ::u32 PaintBrush::OnIntensityChange()
    {
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnIntensityChanged, m_intensity);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::u32 PaintBrush::OnOpacityChange()
    {
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnOpacityChanged, m_opacity);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::u32 PaintBrush::OnRadiusChange()
    {
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnRadiusChanged, m_radius);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    float PaintBrush::GetRadius() const
    {
        return m_radius;
    }

    float PaintBrush::GetIntensity() const
    {
        return m_intensity;
    }

    float PaintBrush::GetOpacity() const
    {
        return m_opacity;
    }

    void PaintBrush::SetRadius(float radius)
    {
        m_radius = radius;
        OnRadiusChange();
    }

    void PaintBrush::SetIntensity(float intensity)
    {
        m_intensity = intensity;
        OnIntensityChange();
    }

    void PaintBrush::SetOpacity(float opacity)
    {
        m_opacity = opacity;
        OnOpacityChange();
    }

    bool PaintBrush::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Move)
        {
            MovePaintBrush(
                mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);

            // For move events, always return false so that mouse movements with right clicks can still affect the Editor camera.
            return false;
        }
        else if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                // Track when we've started painting so that we can properly handle continuous brush strokes between mouse events.
                m_isFirstPaintedPoint = true;

                m_isPainting = true;
                MovePaintBrush(
                    mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                    mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);
                return true;
            }
        }
        else if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up)
        {
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                m_isPainting = false;
                return true;
            }
        }
        return false;
    }

    void PaintBrush::MovePaintBrush(int viewportId, const AzFramework::ScreenPoint& screenCoordinates)
    {
        // Ray cast into the screen to find the closest collision point for the current mouse location.
        auto worldSurfacePosition = AzToolsFramework::FindClosestPickIntersection(
            viewportId,
            screenCoordinates,
            EditorPickRayLength);

        // If the mouse isn't colliding with anything, don't move the paintbrush, just leave it at its last location.
        if (!worldSurfacePosition.has_value())
        {
            return;
        }

        // We found a collision point, so move the paintbrush.
        m_center = worldSurfacePosition.value();
        AZ::Transform space = AZ::Transform::CreateTranslation(m_center);
        PaintBrushNotificationBus::Event(m_ownerEntityComponentId, &PaintBrushNotificationBus::Events::OnWorldSpaceChanged, space);

        // If we're currently painting, send off a paint notification.
        if (m_isPainting)
        {
            // Keep track of the previous painted point and the current one. We'll use that to create a brush stroke
            // that covers everything in-between.
            if (m_isFirstPaintedPoint)
            {
                m_previousCenter = m_center;
                m_isFirstPaintedPoint = false;
            }

            // Create an AABB that contains both endpoints. By definition, it will contain all of the brush stroke
            // points that fall in-between as well.
            AZ::Aabb strokeRegion = AZ::Aabb::CreateCenterRadius(m_center, m_radius);
            strokeRegion.AddAabb(AZ::Aabb::CreateCenterRadius(m_previousCenter, m_radius));

            PaintBrushNotificationBus::Event(
                m_ownerEntityComponentId,
                &PaintBrushNotificationBus::Events::OnPaint,
                strokeRegion);

            m_previousCenter = m_center;
        }
    }

    void PaintBrush::GetValue(const AZ::Vector3& point, float& intensity, float& opacity, bool& isValid)
    {
        GetValues(
            AZStd::span<const AZ::Vector3>(&point, 1),
            AZStd::span<float>(&intensity, 1),
            AZStd::span<float>(&opacity, 1),
            AZStd::span<bool>(&isValid, 1)
        );
    }

    void PaintBrush::GetValues(
        AZStd::span<const AZ::Vector3> points,
        AZStd::span<float> intensities,
        AZStd::span<float> opacities,
        AZStd::span<bool> validFlags)
    {
        const float manipulatorRadiusSq = m_radius * m_radius;
        const AZ::Vector2 previousCenter2D(m_previousCenter);
        const AZ::Vector2 center2D(m_center);

        for (size_t index = 0; index < points.size(); index++)
        {
            // Any point that falls within our brush stroke is considered valid. Since our brush is a circle, the brush stroke
            // ends up being a capsule with a circle on each end, and effectively a box connecting the two circles.
            // We can easily check to see if the point falls in the capsule by getting the distance between the point
            // and the line segment formed between the center of the start and end circles.
            // If this distance is less than the radius of the circle, then it falls within the brush stroke.
            // This works equally well whether the point is closest to an endpoint of the segment or a point along the segment.
            if (m_isPainting &&
                (AZ::Geometry2DUtils::ShortestDistanceSqPointSegment(
                    AZ::Vector2(points[index]), previousCenter2D, center2D) <= manipulatorRadiusSq))
            {
                intensities[index] = GetIntensity();
                opacities[index] = GetOpacity();
                validFlags[index] = true;
            }
            else
            {
                intensities[index] = 0.0f;
                opacities[index] = 0.0f;
                validFlags[index] = false;
            }
        }
    }

    void PaintBrush::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_ownerEntityComponentId = entityComponentIdPair;
        PaintBrushRequestBus::Handler::BusConnect(entityComponentIdPair);
    }

    void PaintBrush::Deactivate()
    {
        PaintBrushRequestBus::Handler::BusDisconnect();
    }
} // namespace AzToolsFramework
