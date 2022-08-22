/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>
#include <AzToolsFramework/Brushes/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <Editor/EditorImageGradientComponentMode.h>
#include <Editor/EditorImageGradientRequestBus.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientModificationBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>

#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    // Increase / decrease paintbrush radius amount in meters.
    static constexpr float RadiusAdjustAmount = 0.25f;

    // Increase / decrease paintbrush intensity amount.
    static constexpr float IntensityAdjustAmount = 0.025f;

    // Increase / decrease paintbrush opacity amount.
    static constexpr float OpacityAdjustAmount = 0.025f;

    static constexpr AZ::Crc32 PaintbrushIncreaseRadius = AZ_CRC_CE("org.o3de.action.paintbrush.increase_radius");
    static constexpr AZ::Crc32 PaintbrushDecreaseRadius = AZ_CRC_CE("org.o3de.action.paintbrush.decrease_radius");

    const constexpr char* PaintbrushIncreaseRadiusTitle = "Increase Radius";
    const constexpr char* PaintbrushDecreaseRadiusTitle = "Decrease Radius";

    const constexpr char* PaintbrushIncreaseRadiusDesc = "Increases radius of paintbrush";
    const constexpr char* PaintbrushDecreaseRadiusDesc = "Decreases radius of paintbrush";


    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::StartImageModification);

        AzToolsFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_brushManipulator = AzToolsFramework::PaintBrushManipulator::MakeShared(worldFromLocal, entityComponentIdPair);
        Refresh();

        m_brushManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        AzToolsFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
        m_brushManipulator->Unregister();

        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::EndImageModification);
        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::SaveImage);
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorImageGradientComponentMode::PopulateActionsImpl()
    {
        return {
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushIncreaseRadius)
                .SetKeySequence(QKeySequence{ Qt::Key_BracketRight })
                .SetTitle(PaintbrushIncreaseRadiusTitle)
                .SetTip(PaintbrushIncreaseRadiusDesc)
                .SetEntityComponentIdPair(GetEntityComponentIdPair())
                .SetCallback(
                    [this]()
                    {
                        AdjustRadius(RadiusAdjustAmount);
                    }),
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushDecreaseRadius)
                .SetKeySequence(QKeySequence{ Qt::Key_BracketLeft })
                .SetTitle(PaintbrushDecreaseRadiusTitle)
                .SetTip(PaintbrushDecreaseRadiusDesc)
                .SetEntityComponentIdPair(GetEntityComponentIdPair())
                .SetCallback(
                    [this]()
                    {
                        AdjustRadius(-RadiusAdjustAmount);
                    }),
        };
    }

    AZStd::string EditorImageGradientComponentMode::GetComponentModeName() const
    {
        return "Image Gradient Paint Mode";
    }

    bool EditorImageGradientComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        bool result = false;

        AzToolsFramework::PaintBrushRequestBus::EventResult(
            result, GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::HandleMouseInteraction, mouseInteraction);
        return result;
    }

    void EditorImageGradientComponentMode::Refresh()
    {
    }

    void EditorImageGradientComponentMode::OnRadiusChanged(float radius)
    {
        m_brushManipulator->SetRadius(radius);
    }

    void EditorImageGradientComponentMode::OnWorldSpaceChanged(const AZ::Transform& brushTransform)
    {
        m_brushManipulator->SetSpace(brushTransform);
    }

    void EditorImageGradientComponentMode::OnPaint(const AZ::Aabb& dirtyArea)
    {
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        ImageGradientRequestBus::EventResult(imagePixelsPerMeter, GetEntityId(), &ImageGradientRequestBus::Events::GetImagePixelsPerMeter);

        if ((imagePixelsPerMeter.GetX() <= 0.0f) || (imagePixelsPerMeter.GetY() <= 0.0f))
        {
            return;
        }

        const float xStep = 1.0f / imagePixelsPerMeter.GetX();
        const float yStep = 1.0f / imagePixelsPerMeter.GetY();

        const AZ::Vector3 minDistances = dirtyArea.GetMin();
        const AZ::Vector3 maxDistances = dirtyArea.GetMax();

        AZStd::vector<AZ::Vector3> points;

        for (float y = minDistances.GetY(); y <= maxDistances.GetY(); y += yStep)
        {
            for (float x = minDistances.GetX(); x <= maxDistances.GetX(); x += xStep)
            {
                points.emplace_back(x, y, minDistances.GetZ());
            }
        }

        AZStd::vector<float> intensities(points.size());
        AZStd::vector<float> opacities(points.size());
        AZStd::vector<bool> validFlags(points.size());

        AzToolsFramework::PaintBrushRequestBus::Event(
            GetEntityComponentIdPair(),
            &AzToolsFramework::PaintBrushRequestBus::Events::GetValues,
            points,
            intensities,
            opacities,
            validFlags);

        AZStd::vector<float> oldValues(points.size());
        GradientRequestBus::Event(GetEntityId(), &GradientRequestBus::Events::GetValues, points, oldValues);

        for (size_t index = 0; index < points.size(); index++)
        {
            if (validFlags[index])
            {
                float newValue = AZStd::lerp(oldValues[index], intensities[index], opacities[index]);
                ImageGradientModificationBus::Event(
                    GetEntityId(), &ImageGradientModificationBus::Events::SetValue, points[index], newValue);
            }
        }

        LmbrCentral::DependencyNotificationBus::Event(
            GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, dirtyArea);
    }

    void EditorImageGradientComponentMode::AdjustRadius(float radiusDelta)
    {
        float radius = 0.0f;
        AzToolsFramework::PaintBrushRequestBus::EventResult(
            radius, GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::GetRadius);

        radius = AZStd::clamp(radius + radiusDelta, 0.01f, 1024.0f);

        AzToolsFramework::PaintBrushRequestBus::Event(
            GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::SetRadius, radius);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorImageGradientComponentMode::AdjustIntensity(float intensityDelta)
    {
        float intensity = 0.0f;
        AzToolsFramework::PaintBrushRequestBus::EventResult(
            intensity, GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::GetIntensity);

        intensity = AZStd::clamp(intensity + intensityDelta, 0.0f, 1.0f);

        AzToolsFramework::PaintBrushRequestBus::Event(
            GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::SetIntensity, intensity);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorImageGradientComponentMode::AdjustOpacity(float opacityDelta)
    {
        float opacity = 0.0f;
        AzToolsFramework::PaintBrushRequestBus::EventResult(
            opacity, GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::GetOpacity);

        opacity = AZStd::clamp(opacity + opacityDelta, 0.0f, 1.0f);

        AzToolsFramework::PaintBrushRequestBus::Event(
            GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::SetOpacity, opacity);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

} // namespace GradientSignal
