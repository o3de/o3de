/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushRequestBus.h>
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
        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::StartImageModification);
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

        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::SaveImage);

        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::EndImageModification);
        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::EndImageModification);
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
        return m_brushManipulator->HandleMouseInteraction(mouseInteraction);
    }

    void EditorImageGradientComponentMode::Refresh()
    {
    }

    void EditorImageGradientComponentMode::OnPaint(const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn)
    {
        // The OnPaint notification means that we should paint new values into our image gradient.
        // To do this, we need to calculate the set of world space positions that map to individual pixels in the image,
        // then ask the paint brush for each position what value we should set that pixel to. Finally, we use those modified
        // values to change the image gradient.

        // Get the spacing to map individual pixels to world space positions.
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

        // Calculate the minimum set of world space points that map to those pixels.
        AZStd::vector<AZ::Vector3> points;
        for (float y = minDistances.GetY(); y <= maxDistances.GetY(); y += yStep)
        {
            for (float x = minDistances.GetX(); x <= maxDistances.GetX(); x += xStep)
            {
                points.emplace_back(x, y, minDistances.GetZ());
            }
        }

        // Get the painted value settings for each of those world space points.
        AZStd::vector<float> intensities(points.size());
        AZStd::vector<float> opacities(points.size());
        AZStd::vector<bool> validFlags(points.size());

        valueLookupFn(points, intensities, opacities, validFlags);

        // Get the previous gradient image values
        AZStd::vector<float> oldValues(points.size());
        GradientRequestBus::Event(GetEntityId(), &GradientRequestBus::Events::GetValues, points, oldValues);

        // For each value, blend it with the painted value and set the gradient image to the new value.
        for (size_t index = 0; index < points.size(); index++)
        {
            if (validFlags[index])
            {
                float newValue = AZStd::lerp(oldValues[index], intensities[index], opacities[index]);
                ImageGradientModificationBus::Event(
                    GetEntityId(), &ImageGradientModificationBus::Events::SetValue, points[index], newValue);
            }
        }

        // Notify anything listening to the image gradient that the modified region has changed.
        LmbrCentral::DependencyNotificationBus::Event(
            GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, dirtyArea);
    }

    void EditorImageGradientComponentMode::AdjustRadius(float radiusDelta)
    {
        float radius = m_brushManipulator->GetRadius();
        radius = AZStd::clamp(radius + radiusDelta, 0.01f, 1024.0f);
        m_brushManipulator->SetRadius(radius);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorImageGradientComponentMode::AdjustIntensity(float intensityDelta)
    {
        float intensity = m_brushManipulator->GetIntensity();
        intensity = AZStd::clamp(intensity + intensityDelta, 0.0f, 1.0f);
        m_brushManipulator->SetIntensity(intensity);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorImageGradientComponentMode::AdjustOpacity(float opacityDelta)
    {
        float opacity = m_brushManipulator->GetOpacity();
        opacity = AZStd::clamp(opacity + opacityDelta, 0.0f, 1.0f);
        m_brushManipulator->SetOpacity(opacity);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

} // namespace GradientSignal
