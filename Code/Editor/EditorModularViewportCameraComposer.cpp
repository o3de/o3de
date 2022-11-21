/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModularViewportCameraComposer.h>

#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraControllerRequestBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Render/IntersectorInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>
#include <EditorViewportSettings.h>

AZ_CVAR(
    bool,
    ed_cameraPinDefaultOrbit,
    true,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Sets whether the default orbit point moves with the camera or not");
AZ_CVAR(
    bool,
    ed_cameraDefaultOrbitAxesOrtho,
    true,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Sets whether to draw the default orbit point as orthographic or not");
AZ_CVAR(
    float,
    ed_cameraDefaultOrbitFadeDuration,
    0.5f,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Sets how long the default orbit point should take to appear and disappear");
AZ_CVAR(
    float,
    ed_cameraPivotFadedOpacity,
    0.5f,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "How faded should the camera pivot appear when it is set but no active rotation is happening");
AZ_CVAR(float, ed_cameraPivotSize, 0.05f, nullptr, AZ::ConsoleFunctorFlags::Null, "Specify the size the camera pivot point should be");
AZ_CVAR(
    AZ::Color,
    ed_cameraPivotColor,
    AZ::Color::CreateFromRgba(255, 0, 0, 255),
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Specify the color the camera pivot point should be");

namespace SandboxEditor
{
    static AzFramework::TranslateCameraInputChannelIds BuildTranslateCameraInputChannelIds()
    {
        AzFramework::TranslateCameraInputChannelIds translateCameraInputChannelIds;
        translateCameraInputChannelIds.m_leftChannelId = SandboxEditor::CameraTranslateLeftChannelId();
        translateCameraInputChannelIds.m_rightChannelId = SandboxEditor::CameraTranslateRightChannelId();
        translateCameraInputChannelIds.m_forwardChannelId = SandboxEditor::CameraTranslateForwardChannelId();
        translateCameraInputChannelIds.m_backwardChannelId = SandboxEditor::CameraTranslateBackwardChannelId();
        translateCameraInputChannelIds.m_upChannelId = SandboxEditor::CameraTranslateUpChannelId();
        translateCameraInputChannelIds.m_downChannelId = SandboxEditor::CameraTranslateDownChannelId();
        translateCameraInputChannelIds.m_boostChannelId = SandboxEditor::CameraTranslateBoostChannelId();

        return translateCameraInputChannelIds;
    }

    EditorModularViewportCameraComposer::EditorModularViewportCameraComposer(const AzFramework::ViewportId viewportId)
        : m_viewportId(viewportId)
    {
        EditorModularViewportCameraComposerNotificationBus::Handler::BusConnect(viewportId);
        Camera::EditorCameraNotificationBus::Handler::BusConnect();
    }

    EditorModularViewportCameraComposer::~EditorModularViewportCameraComposer()
    {
        Camera::EditorCameraNotificationBus::Handler::BusDisconnect();
        EditorModularViewportCameraComposerNotificationBus::Handler::BusDisconnect();
    }

    AZStd::shared_ptr<AtomToolsFramework::ModularViewportCameraController> EditorModularViewportCameraComposer::
        CreateModularViewportCameraController()
    {
        SetupCameras();

        auto controller = AZStd::make_shared<AtomToolsFramework::ModularViewportCameraController>();

        controller->SetCameraViewportContextBuilderCallback(
            [viewportId = m_viewportId](AZStd::unique_ptr<AtomToolsFramework::ModularCameraViewportContext>& cameraViewportContext)
            {
                cameraViewportContext = AZStd::make_unique<AtomToolsFramework::ModularCameraViewportContextImpl>(viewportId);
            });

        controller->SetCameraPriorityBuilderCallback(
            [](AtomToolsFramework::CameraControllerPriorityFn& cameraControllerPriorityFn)
            {
                cameraControllerPriorityFn = AtomToolsFramework::DefaultCameraControllerPriority;
            });

        controller->SetCameraPropsBuilderCallback(
            [](AzFramework::CameraProps& cameraProps)
            {
                cameraProps.m_rotateSmoothnessFn = []
                {
                    return SandboxEditor::CameraRotateSmoothness();
                };

                cameraProps.m_translateSmoothnessFn = []
                {
                    return SandboxEditor::CameraTranslateSmoothness();
                };

                cameraProps.m_rotateSmoothingEnabledFn = []
                {
                    return SandboxEditor::CameraRotateSmoothingEnabled();
                };

                cameraProps.m_translateSmoothingEnabledFn = []
                {
                    return SandboxEditor::CameraTranslateSmoothingEnabled();
                };
            });

        controller->SetCameraListBuilderCallback(
            [this](AzFramework::Cameras& cameras)
            {
                cameras.AddCamera(m_firstPersonRotateCamera);
                cameras.AddCamera(m_firstPersonPanCamera);
                cameras.AddCamera(m_firstPersonTranslateCamera);
                cameras.AddCamera(m_firstPersonScrollCamera);
                cameras.AddCamera(m_firstPersonFocusCamera);
                cameras.AddCamera(m_orbitCamera);
            });

        return controller;
    }

    void EditorModularViewportCameraComposer::SetupCameras()
    {
        const auto hideCursor = [viewportId = m_viewportId]
        {
            if (SandboxEditor::CameraCaptureCursorForLook())
            {
                AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Event(
                    viewportId, &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::BeginCursorCapture);
            }
        };

        const auto showCursor = [viewportId = m_viewportId]
        {
            if (SandboxEditor::CameraCaptureCursorForLook())
            {
                AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Event(
                    viewportId, &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::EndCursorCapture);
            }
        };

        const auto trackingTransform = [viewportId = m_viewportId]
        {
            bool tracking = false;
            AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
                tracking, viewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsTrackingTransform);

            return tracking;
        };

        m_firstPersonRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(SandboxEditor::CameraFreeLookChannelId());

        m_firstPersonRotateCamera->m_rotateSpeedFn = []
        {
            return SandboxEditor::CameraRotateSpeed();
        };

        m_firstPersonRotateCamera->m_constrainPitch = [trackingTransform]
        {
            return !trackingTransform();
        };

        // default behavior is to hide the cursor but this can be disabled (useful for remote desktop)
        // note: See CaptureCursorLook in the Settings Registry
        m_firstPersonRotateCamera->SetActivationBeganFn(hideCursor);
        m_firstPersonRotateCamera->SetActivationEndedFn(showCursor);

        m_firstPersonPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            SandboxEditor::CameraFreePanChannelId(), AzFramework::LookPan, AzFramework::TranslatePivotLook);

        m_firstPersonPanCamera->m_panSpeedFn = []
        {
            return SandboxEditor::CameraPanSpeedScaled();
        };

        m_firstPersonPanCamera->m_invertPanXFn = []
        {
            return SandboxEditor::CameraPanInvertedX();
        };

        m_firstPersonPanCamera->m_invertPanYFn = []
        {
            return SandboxEditor::CameraPanInvertedY();
        };

        const auto translateCameraInputChannelIds = BuildTranslateCameraInputChannelIds();

        m_firstPersonTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslatePivotLook);

        m_firstPersonTranslateCamera->m_translateSpeedFn = []
        {
            return SandboxEditor::CameraTranslateSpeedScaled();
        };

        m_firstPersonTranslateCamera->m_boostMultiplierFn = []
        {
            return SandboxEditor::CameraBoostMultiplier();
        };

        m_firstPersonScrollCamera = AZStd::make_shared<AzFramework::LookScrollTranslationCameraInput>();

        m_firstPersonScrollCamera->m_scrollSpeedFn = []
        {
            return SandboxEditor::CameraScrollSpeedScaled();
        };

        const auto focusPivotFn = []() -> AZStd::optional<AZ::Vector3>
        {
            // use the manipulator transform as the pivot point
            AZStd::optional<AZ::Transform> entityPivot;
            AzToolsFramework::EditorTransformComponentSelectionRequestBus::EventResult(
                entityPivot, AzToolsFramework::GetEntityContextId(),
                &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::GetManipulatorTransform);

            if (entityPivot.has_value())
            {
                return entityPivot->GetTranslation();
            }

            return AZStd::nullopt;
        };

        m_firstPersonFocusCamera =
            AZStd::make_shared<AzFramework::FocusCameraInput>(SandboxEditor::CameraFocusChannelId(), AzFramework::FocusLook);

        m_firstPersonFocusCamera->SetPivotFn(focusPivotFn);

        m_orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>(SandboxEditor::CameraOrbitChannelId());

        m_orbitCamera->SetPivotFn(
            [this]([[maybe_unused]] const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& direction)
            {
                return m_pivot.value_or(AZ::Vector3::CreateZero());
            });

        m_orbitCamera->SetActivationBeganFn(
            [this]
            {
                AZ::TickBus::Handler::BusConnect();
                AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());

                // pivot should be displayed but not be 'active' (full when rotation behavior is happening)
                m_pivotDisplayState = PivotDisplayState::Faded;
            });
        m_orbitCamera->SetActivationEndedFn(
            [this]
            {
                // when the orbit behavior ends the pivot point should fade out and no longer display
                m_pivotDisplayState = PivotDisplayState::Hidden;
            });

        m_orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(SandboxEditor::CameraOrbitLookChannelId());

        m_orbitRotateCamera->m_rotateSpeedFn = []
        {
            return SandboxEditor::CameraRotateSpeed();
        };

        m_orbitRotateCamera->m_invertYawFn = []
        {
            return SandboxEditor::CameraOrbitYawRotationInverted();
        };

        m_orbitRotateCamera->m_constrainPitch = [trackingTransform]
        {
            return !trackingTransform();
        };

        m_orbitRotateCamera->SetInitiateRotateFn(
            [this]
            {
                AZStd::optional<AzFramework::ScreenPoint> screenPoint;
                AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::EventResult(
                    screenPoint, m_viewportId,
                    &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::MousePosition);

                if (screenPoint.has_value())
                {
                    const auto [origin, direction] =
                        AzToolsFramework::ViewportInteraction::ViewportScreenToWorldRay(m_viewportId, screenPoint.value());

                    AzToolsFramework::EntityIdList visibleEntityIds;
                    AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Event(
                        m_viewportId,
                        &AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Events::FindVisibleEntities,
                        visibleEntityIds);

                    bool pickedEntity = false;
                    float closestDistance = AZStd::numeric_limits<float>::max();
                    for (const auto& entityId : visibleEntityIds)
                    {
                        float distance;
                        if (AzToolsFramework::PickEntity(entityId, origin, direction, distance, m_viewportId))
                        {
                            pickedEntity = true;
                            closestDistance = AZStd::min(distance, closestDistance);
                        }
                    }

                    const float distance = pickedEntity ? closestDistance : AzToolsFramework::GetDefaultEntityPlacementDistance();
                    m_pivot = origin + direction * distance;

                    // ensure we immediately set the camera pivot to ensure no interpolation of current to target occurs
                    AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                        m_viewportId,
                        &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::SetCameraPivotDetachedImmediate,
                        m_pivot.value());
                }
            });

        m_orbitRotateCamera->SetActivationBeganFn(
            [this]
            {
                m_pivotDisplayState = PivotDisplayState::Full;
            });
        m_orbitRotateCamera->SetActivationEndedFn(
            [this]
            {
                m_pivotDisplayState = PivotDisplayState::Faded;
            });

        m_orbitTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslateOffsetOrbit);

        m_orbitTranslateCamera->m_translateSpeedFn = []
        {
            return SandboxEditor::CameraTranslateSpeedScaled();
        };

        m_orbitTranslateCamera->m_boostMultiplierFn = []
        {
            return SandboxEditor::CameraBoostMultiplier();
        };

        m_orbitScrollDollyCamera = AZStd::make_shared<AzFramework::OrbitScrollDollyCameraInput>();

        m_orbitScrollDollyCamera->m_scrollSpeedFn = []
        {
            return SandboxEditor::CameraScrollSpeedScaled();
        };

        m_orbitMotionDollyCamera = AZStd::make_shared<AzFramework::OrbitMotionDollyCameraInput>(SandboxEditor::CameraOrbitDollyChannelId());

        m_orbitMotionDollyCamera->m_motionSpeedFn = []
        {
            return SandboxEditor::CameraDollyMotionSpeedScaled();
        };

        m_orbitPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            SandboxEditor::CameraOrbitPanChannelId(), AzFramework::LookPan, AzFramework::TranslateOffsetOrbit);

        m_orbitPanCamera->m_panSpeedFn = []
        {
            return SandboxEditor::CameraPanSpeedScaled();
        };

        m_orbitPanCamera->m_invertPanXFn = []
        {
            return SandboxEditor::CameraPanInvertedX();
        };

        m_orbitPanCamera->m_invertPanYFn = []
        {
            return SandboxEditor::CameraPanInvertedY();
        };

        m_orbitFocusCamera =
            AZStd::make_shared<AzFramework::FocusCameraInput>(SandboxEditor::CameraFocusChannelId(), AzFramework::FocusOrbit);

        m_orbitFocusCamera->SetPivotFn(focusPivotFn);

        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitRotateCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitTranslateCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitScrollDollyCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitMotionDollyCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitPanCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitFocusCamera);
    }

    void EditorModularViewportCameraComposer::OnEditorModularViewportCameraComposerSettingsChanged()
    {
        const auto translateCameraInputChannelIds = BuildTranslateCameraInputChannelIds();
        m_firstPersonTranslateCamera->SetTranslateCameraInputChannelIds(translateCameraInputChannelIds);
        m_firstPersonPanCamera->SetPanInputChannelId(SandboxEditor::CameraFreePanChannelId());
        m_firstPersonRotateCamera->SetRotateInputChannelId(SandboxEditor::CameraFreeLookChannelId());
        m_firstPersonFocusCamera->SetFocusInputChannelId(SandboxEditor::CameraFocusChannelId());

        m_orbitCamera->SetOrbitInputChannelId(SandboxEditor::CameraOrbitChannelId());
        m_orbitTranslateCamera->SetTranslateCameraInputChannelIds(translateCameraInputChannelIds);
        m_orbitPanCamera->SetPanInputChannelId(SandboxEditor::CameraOrbitPanChannelId());
        m_orbitRotateCamera->SetRotateInputChannelId(SandboxEditor::CameraOrbitLookChannelId());
        m_orbitMotionDollyCamera->SetDollyInputChannelId(SandboxEditor::CameraOrbitDollyChannelId());
        m_orbitFocusCamera->SetFocusInputChannelId(SandboxEditor::CameraFocusChannelId());
    }

    void EditorModularViewportCameraComposer::OnViewportViewEntityChanged(const AZ::EntityId& viewEntityId)
    {
        if (viewEntityId.IsValid())
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, viewEntityId, &AZ::TransformBus::Events::GetWorldTM);

            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                m_viewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::StartTrackingTransform,
                worldFromLocal);
        }
        else
        {
            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                m_viewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::StopTrackingTransform);
        }
    }

    void EditorModularViewportCameraComposer::OnTick(const float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        const float delta = [duration = &ed_cameraDefaultOrbitFadeDuration, deltaTime]
        {
            if (*duration == 0.0f)
            {
                return 1.0f;
            }
            return deltaTime / *duration;
        }();

        if (m_pivot.has_value())
        {
            const float opacity = ed_cameraPivotFadedOpacity;
            switch (m_pivotDisplayState)
            {
            case PivotDisplayState::Faded:
                if (m_orbitOpacity <= opacity)
                {
                    m_orbitOpacity = AZStd::min(m_orbitOpacity + delta, opacity);
                }
                else
                {
                    m_orbitOpacity = AZStd::max(m_orbitOpacity - delta, opacity);
                }
                break;
            case PivotDisplayState::Full:
                m_orbitOpacity = AZStd::min(m_orbitOpacity + delta, 1.0f);
                break;
            case PivotDisplayState::Hidden:
                m_orbitOpacity = AZStd::max(m_orbitOpacity - delta, 0.0f);
                if (m_orbitOpacity == 0.0f)
                {
                    AZ::TickBus::Handler::BusDisconnect();
                    AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
                }
                break;
            }
        }
    }

    void EditorModularViewportCameraComposer::DisplayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_pivot.has_value())
        {
            debugDisplay.CullOff();
            const AZ::Color color = ed_cameraPivotColor;
            debugDisplay.SetColor(AZ::Color::CreateFromVector3AndFloat(color.GetAsVector3(), m_orbitOpacity));
            debugDisplay.DrawBall(m_pivot.value(), ed_cameraPivotSize, false);
            debugDisplay.CullOn();
        }
    }
} // namespace SandboxEditor
