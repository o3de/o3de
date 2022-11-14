/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Geometry2DUtils.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsWindow.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    AZ_CVAR(
        float,
        ed_paintBrushSizeAdjustAmount,
        0.25f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The amount to increase / decrease the paintbrush size in meters.");

    AZ_CVAR(
        float,
        ed_paintBrushHardnessPercentAdjustAmount,
        5.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The amount to increase / decrease the paintbrush hardness in percent (0 - 100).");

    AZ_CVAR(
        AZ::Color,
        ed_paintBrushManipulatorInnerColor,
        AZ::Color::CreateFromRgba(255, 0, 0, 255),      // Can't use AZ::Colors::Red because there's no guarantee of initialization order.
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The color of the paintbrush manipulator.");

    AZ_CVAR(
        AZ::Color,
        ed_paintBrushManipulatorOuterColor,
        AZ::Color::CreateFromRgba(255, 0, 0, 255),      // Can't use AZ::Colors::Red because there's no guarantee of initialization order.
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The color of the paintbrush manipulator.");

    namespace
    {
        static constexpr AZ::Crc32 PaintbrushIncreaseSize = AZ_CRC_CE("org.o3de.action.paintbrush.increase_size");
        static constexpr AZ::Crc32 PaintbrushDecreaseSize = AZ_CRC_CE("org.o3de.action.paintbrush.decrease_size");
        static constexpr AZ::Crc32 PaintbrushIncreaseHardness = AZ_CRC_CE("org.o3de.action.paintbrush.increase_hardness_percent");
        static constexpr AZ::Crc32 PaintbrushDecreaseHardness = AZ_CRC_CE("org.o3de.action.paintbrush.decrease_hardness_percent");

        static constexpr const char* PaintbrushIncreaseSizeTitle = "Increase Size";
        static constexpr const char* PaintbrushDecreaseSizeTitle = "Decrease Size";
        static constexpr const char* PaintbrushIncreaseHardnessTitle = "Increase Hardness Percent";
        static constexpr const char* PaintbrushDecreaseHardnessTitle = "Decrease Hardness Percent";

        static constexpr const char* PaintbrushIncreaseSizeDesc = "Increases size of paintbrush";
        static constexpr const char* PaintbrushDecreaseSizeDesc = "Decreases size of paintbrush";
        static constexpr const char* PaintbrushIncreaseHardnessDesc = "Increases paintbrush hardness";
        static constexpr const char* PaintbrushDecreaseHardnessDesc = "Decreases paintbrush hardness";
    } // namespace

    AZStd::shared_ptr<PaintBrushManipulator> PaintBrushManipulator::MakeShared(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair, PaintBrushColorMode colorMode)
    {
        return AZStd::shared_ptr<PaintBrushManipulator>(aznew PaintBrushManipulator(worldFromLocal, entityComponentIdPair, colorMode));
    }

    PaintBrushManipulator::PaintBrushManipulator(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair, PaintBrushColorMode colorMode)
        : m_paintBrush(entityComponentIdPair)
    {
        m_ownerEntityComponentId = entityComponentIdPair;

        SetSpace(worldFromLocal);

        // Make sure the Paint Brush Settings window is open
        AzToolsFramework::OpenViewPane(::PaintBrush::s_paintBrushSettingsName);

        // Set the paint brush settings to use the requested color mode.
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetBrushColorMode, colorMode);

        // Get the global Paint Brush Settings so that we can calculate our brush circle sizes.
        PaintBrushSettings brushSettings;
        PaintBrushSettingsRequestBus::BroadcastResult(brushSettings, &PaintBrushSettingsRequestBus::Events::GetSettings);

        const auto [innerRadius, outerRadius] = GetBrushRadii(brushSettings);

        // The PaintBrush manipulator uses two circles projected into world space to represent the brush.
        const AZ::Color innerCircleColor = ed_paintBrushManipulatorInnerColor;
        const AZ::Color outerCircleColor = ed_paintBrushManipulatorOuterColor;
        const float circleWidth = 0.05f;
        SetView(
            AzToolsFramework::CreateManipulatorViewProjectedCircle(*this, innerCircleColor, innerRadius, circleWidth),
            AzToolsFramework::CreateManipulatorViewProjectedCircle(*this, outerCircleColor, outerRadius, circleWidth));

        // Start listening for any changes to the Paint Brush Settings
        PaintBrushSettingsNotificationBus::Handler::BusConnect();

        m_paintBrush.BeginPaintMode();
    }

    PaintBrushManipulator::~PaintBrushManipulator()
    {
        // If we're ending mid-brush-stroke, make sure to notify that the brush stroke should end.
        if (m_paintBrush.IsInBrushStroke())
        {
            m_paintBrush.EndBrushStroke();
        }

        m_paintBrush.EndPaintMode();

        // Stop listening for any changes to the Paint Brush Settings
        PaintBrushSettingsNotificationBus::Handler::BusDisconnect();

        // Make sure the Paint Brush Settings window is closed
        AzToolsFramework::CloseViewPane(::PaintBrush::s_paintBrushSettingsName);
    }

    AZStd::pair<float, float> PaintBrushManipulator::GetBrushRadii(const PaintBrushSettings& settings) const
    {
        const float outerRadius = settings.GetSize() / 2.0f;

        if (settings.GetBrushMode() == PaintBrushMode::Eyedropper)
        {
            // For the eyedropper, we'll set the inner radius to an arbitrarily small percentage of the full brush size to help
            // visualize that we're only picking from the very center of the brush.
            const float eyedropperRadius = outerRadius * 0.02f;
            return { eyedropperRadius, outerRadius };
        }

        // For paint/smooth brushes, the inner circle represents the start of the hardness falloff,
        // and the outer circle is the full paintbrush size.
        const float hardnessRadius = outerRadius * (settings.GetHardnessPercent() / 100.0f);
        return { hardnessRadius, outerRadius };
    }

    void PaintBrushManipulator::Draw(
        const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        // Always set our manipulator state to say that the mouse isn't over the manipulator so that we always use our base
        // manipulator color. The paintbrush isn't a "selectable" manipulator, so it wouldn't make sense for it to change color when the
        // mouse is over it.
        constexpr bool mouseOver = false;

        m_innerCircle->m_color = ed_paintBrushManipulatorInnerColor;
        m_outerCircle->m_color = ed_paintBrushManipulatorOuterColor;

        m_innerCircle->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(),
            { GetSpace(), GetNonUniformScale(), AZ::Vector3::CreateZero(), mouseOver }, debugDisplay, cameraState,
            mouseInteraction);

        m_outerCircle->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(),
            { GetSpace(), GetNonUniformScale(), AZ::Vector3::CreateZero(), mouseOver }, debugDisplay, cameraState,
            mouseInteraction);
    }

    void PaintBrushManipulator::SetView(
        AZStd::shared_ptr<ManipulatorViewProjectedCircle> innerCircle, AZStd::shared_ptr<ManipulatorViewProjectedCircle> outerCircle)
    {
        m_innerCircle = AZStd::move(innerCircle);
        m_outerCircle = AZStd::move(outerCircle);
    }

    void PaintBrushManipulator::OnSettingsChanged(const PaintBrushSettings& newSettings)
    {
        const auto [innerRadius, outerRadius] = GetBrushRadii(newSettings);

        m_innerCircle->SetRadius(innerRadius);
        m_outerCircle->SetRadius(outerRadius);
    }

    bool PaintBrushManipulator::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(Entity);

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
                // Get the current global Paint Brush Settings 
                PaintBrushSettings brushSettings;
                PaintBrushSettingsRequestBus::BroadcastResult(brushSettings, &PaintBrushSettingsRequestBus::Events::GetSettings);

                // Notify that a paint stroke has begun, and provide the paint color including opacity.
                m_paintBrush.BeginBrushStroke(brushSettings);

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
                // Notify that the paint stroke has ended.
                // We need to verify that we're currently painting, because clicks/double-clicks can cause us to end up here
                // without ever getting the mouse Down event.
                if (m_paintBrush.IsInBrushStroke())
                {
                    m_paintBrush.EndBrushStroke();
                }

                return true;
            }
        }

        return false;
    }

    void PaintBrushManipulator::MovePaintBrush(
        int viewportId, const AzFramework::ScreenPoint& screenCoordinates)
    {
        // Ray cast into the screen to find the closest collision point for the current mouse location.
        auto worldSurfacePosition =
            AzToolsFramework::FindClosestPickIntersection(viewportId, screenCoordinates, AzToolsFramework::EditorPickRayLength);

        // If the mouse isn't colliding with anything, don't move the paintbrush, just leave it at its last location
        // and don't perform any brush actions. We'll reset the stroke movement tracking though so that we don't draw unintended lines
        // once the brush makes it back onto a valid surface.
        if (!worldSurfacePosition.has_value())
        {
            m_paintBrush.ResetBrushStrokeTracking();
            return;
        }

        // We found a collision point, so move the paintbrush.
        AZ::Vector3 brushCenter = worldSurfacePosition.value();
        AZ::Transform space = AZ::Transform::CreateTranslation(brushCenter);
        SetSpace(space);

        // If we're currently performing a brush stroke, then trigger the appropriate brush action based on our selected paint mode.
        if (m_paintBrush.IsInBrushStroke())
        {
            // Get our current paint brush settings.
            PaintBrushSettings brushSettings;
            PaintBrushSettingsRequestBus::BroadcastResult(brushSettings, &PaintBrushSettingsRequestBus::Events::GetSettings);

            switch (brushSettings.GetBrushMode())
            {
            case PaintBrushMode::Paintbrush:
                m_paintBrush.PaintToLocation(brushCenter, brushSettings);
                break;
            case PaintBrushMode::Smooth:
                m_paintBrush.SmoothToLocation(brushCenter, brushSettings);
                break;
            case PaintBrushMode::Eyedropper:
                {
                    AZ::Color eyedropperColor = m_paintBrush.UseEyedropper(brushCenter, brushSettings);

                    // Set the color in our paintbrush settings to the color selected by the eyedropper.
                    PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetColor, eyedropperColor);
                }
                break;
            default:
                AZ_Assert(false, "Unsupported brush mode type: %u", brushSettings.GetBrushMode());
                break;
            }
        }
    }

    AZStd::vector<AzToolsFramework::ActionOverride> PaintBrushManipulator::PopulateActionsImpl()
    {
        // Paint brush manipulators should be able to easily adjust the radius of the brush with the [ and ] keys
        return {
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushIncreaseSize)
                .SetKeySequence(QKeySequence{ Qt::Key_BracketRight })
                .SetTitle(PaintbrushIncreaseSizeTitle)
                .SetTip(PaintbrushIncreaseSizeDesc)
                .SetEntityComponentIdPair(m_ownerEntityComponentId)
                .SetCallback(
                    [this]()
                    {
                        AdjustSize(ed_paintBrushSizeAdjustAmount);
                    }),
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushDecreaseSize)
                .SetKeySequence(QKeySequence{ Qt::Key_BracketLeft })
                .SetTitle(PaintbrushDecreaseSizeTitle)
                .SetTip(PaintbrushDecreaseSizeDesc)
                .SetEntityComponentIdPair(m_ownerEntityComponentId)
                .SetCallback(
                    [this]()
                    {
                        AdjustSize(-ed_paintBrushSizeAdjustAmount);
                    }),
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushIncreaseHardness)
                .SetKeySequence(QKeySequence{ Qt::Key_BraceRight })
                .SetTitle(PaintbrushIncreaseHardnessTitle)
                .SetTip(PaintbrushIncreaseHardnessDesc)
                .SetEntityComponentIdPair(m_ownerEntityComponentId)
                .SetCallback(
                    [this]()
                    {
                        AdjustHardnessPercent(ed_paintBrushHardnessPercentAdjustAmount);
                    }),
            AzToolsFramework::ActionOverride()
                .SetUri(PaintbrushDecreaseHardness)
                .SetKeySequence(QKeySequence{ Qt::Key_BraceLeft })
                .SetTitle(PaintbrushDecreaseHardnessTitle)
                .SetTip(PaintbrushDecreaseHardnessDesc)
                .SetEntityComponentIdPair(m_ownerEntityComponentId)
                .SetCallback(
                    [this]()
                    {
                        AdjustHardnessPercent(-ed_paintBrushHardnessPercentAdjustAmount);
                    }),
        };
    }

    void PaintBrushManipulator::AdjustSize(float sizeDelta)
    {
        float diameter = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(diameter, &PaintBrushSettingsRequestBus::Events::GetSize);
        diameter += sizeDelta;
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetSize, diameter);
    }

    void PaintBrushManipulator::AdjustHardnessPercent(float hardnessPercentDelta)
    {
        float hardnessPercent = 0.0f;
        PaintBrushSettingsRequestBus::BroadcastResult(hardnessPercent, &PaintBrushSettingsRequestBus::Events::GetHardnessPercent);
        hardnessPercent += hardnessPercentDelta;
        PaintBrushSettingsRequestBus::Broadcast(&PaintBrushSettingsRequestBus::Events::SetHardnessPercent, hardnessPercent);
    }

} // namespace AzToolsFramework
