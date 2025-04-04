/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/math.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <AzToolsFramework/Input/QtEventToAzInputMapper.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AtomToolsFramework
{
    AZ::Transform TransformFromMatrix4x4(const AZ::Matrix4x4& matrix)
    {
        const auto rotation = AZ::Matrix3x3::CreateFromMatrix4x4(matrix);
        const auto translation = matrix.GetTranslation();
        return AZ::Transform::CreateFromMatrix3x3AndTranslation(rotation, translation);
    }

    AZ::Matrix4x4 Matrix4x4FromTransform(const AZ::Transform& transform)
    {
        return AZ::Matrix4x4::CreateFromQuaternionAndTranslation(transform.GetRotation(), transform.GetTranslation());
    }

    // debug
    void DrawPreviewAxis(AzFramework::DebugDisplayRequests& display, const AZ::Transform& transform, const float axisLength)
    {
        display.SetColor(AZ::Colors::Red);
        display.DrawLine(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisX().GetNormalizedSafe() * axisLength);
        display.SetColor(AZ::Colors::Green);
        display.DrawLine(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisY().GetNormalizedSafe() * axisLength);
        display.SetColor(AZ::Colors::Blue);
        display.DrawLine(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisZ().GetNormalizedSafe() * axisLength);
    }

    // convenience function to access the ViewportContext for the given ViewportId.
    static AZ::RPI::ViewportContextPtr RetrieveViewportContext(const AzFramework::ViewportId viewportId)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (!viewportContextManager)
        {
            return nullptr;
        }

        auto viewportContext = viewportContextManager->GetViewportContextById(viewportId);
        if (!viewportContext)
        {
            return nullptr;
        }

        return viewportContext;
    }

    ModularCameraViewportContextImpl::ModularCameraViewportContextImpl(const AzFramework::ViewportId viewportId)
        : m_viewportId(viewportId)
    {
    }

    AZ::Transform ModularCameraViewportContextImpl::GetCameraTransform() const
    {
        if (auto viewportContext = RetrieveViewportContext(m_viewportId))
        {
            return viewportContext->GetCameraTransform();
        }

        return AZ::Transform::CreateIdentity();
    }

    void ModularCameraViewportContextImpl::SetCameraTransform(const AZ::Transform& transform)
    {
        if (auto viewportContext = RetrieveViewportContext(m_viewportId))
        {
            viewportContext->SetCameraTransform(transform);
        }
    }

    void ModularCameraViewportContextImpl::ConnectViewMatrixChangedHandler(AZ::RPI::MatrixChangedEvent::Handler& handler)
    {
        if (auto viewportContext = RetrieveViewportContext(m_viewportId))
        {
            viewportContext->ConnectViewMatrixChangedHandler(handler);
        }
    }

    void ModularViewportCameraController::SetCameraListBuilderCallback(const CameraListBuilder& builder)
    {
        m_cameraListBuilder = builder;
    }

    void ModularViewportCameraController::SetCameraPropsBuilderCallback(const CameraPropsBuilder& builder)
    {
        m_cameraPropsBuilder = builder;
    }

    void ModularViewportCameraController::SetCameraPriorityBuilderCallback(const CameraPriorityBuilder& builder)
    {
        m_cameraControllerPriorityBuilder = builder;
    }

    void ModularViewportCameraController::SetCameraViewportContextBuilderCallback(const CameraViewportContextBuilder& builder)
    {
        m_cameraViewportContextBuilder = builder;
    }

    void ModularViewportCameraController::SetupCameras(AzFramework::Cameras& cameras)
    {
        if (m_cameraListBuilder)
        {
            m_cameraListBuilder(cameras);
        }
    }

    void ModularViewportCameraController::SetupCameraProperties(AzFramework::CameraProps& cameraProps)
    {
        if (m_cameraPropsBuilder)
        {
            m_cameraPropsBuilder(cameraProps);
        }
    }

    void ModularViewportCameraController::SetupCameraControllerPriority(CameraControllerPriorityFn& cameraPriorityFn)
    {
        if (m_cameraControllerPriorityBuilder)
        {
            m_cameraControllerPriorityBuilder(cameraPriorityFn);
        }
    }

    void ModularViewportCameraController::SetupCameraControllerViewportContext(
        AZStd::unique_ptr<ModularCameraViewportContext>& cameraViewportContext)
    {
        if (m_cameraViewportContextBuilder)
        {
            m_cameraViewportContextBuilder(cameraViewportContext);
        }
    }

    // what priority should the camera system respond to
    AzFramework::ViewportControllerPriority DefaultCameraControllerPriority(const AzFramework::CameraSystem& cameraSystem)
    {
        // ModernViewportCameraControllerInstance receives events at all priorities, when it is in 'exclusive' mode
        // or it is actively handling events (essentially when the camera system is 'active' and responding to inputs)
        // it should only respond to the highest priority
        if (cameraSystem.m_cameras.Exclusive() || cameraSystem.HandlingEvents())
        {
            return AzFramework::ViewportControllerPriority::Highest;
        }

        // otherwise it should only respond to normal priority events
        return AzFramework::ViewportControllerPriority::Normal;
    }

    ModularViewportCameraControllerInstance::ModularViewportCameraControllerInstance(
        const AzFramework::ViewportId viewportId, ModularViewportCameraController* controller)
        : MultiViewportControllerInstanceInterface<ModularViewportCameraController>(viewportId, controller)
    {
        controller->SetupCameras(m_cameraSystem.m_cameras);
        controller->SetupCameraProperties(m_cameraProps);
        controller->SetupCameraControllerPriority(m_priorityFn);
        controller->SetupCameraControllerViewportContext(m_modularCameraViewportContext);

        auto handleCameraChangeFn = [this]([[maybe_unused]] const AZ::Matrix4x4& cameraView)
        {
            // ignore these updates if the camera is being updated internally
            if (!m_updatingTransformInternally)
            {
                const AZ::Transform transform = m_modularCameraViewportContext->GetCameraTransform();
                const AZ::Vector3 eulerAngles = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromTransform(transform));
                UpdateCameraFromTranslationAndRotation(m_targetCamera, transform.GetTranslation(), eulerAngles);
                m_targetRoll = eulerAngles.GetY();

                m_camera = m_targetCamera;
                m_roll = m_targetRoll;
            }
        };

        m_cameraViewMatrixChangeHandler = AZ::RPI::MatrixChangedEvent::Handler(handleCameraChangeFn);
        m_modularCameraViewportContext->ConnectViewMatrixChangedHandler(m_cameraViewMatrixChangeHandler);

        ModularViewportCameraControllerRequestBus::Handler::BusConnect(viewportId);
        AzToolsFramework::ViewportInteraction::ViewportInteractionNotificationBus::Handler::BusConnect(viewportId);
    }

    ModularViewportCameraControllerInstance::~ModularViewportCameraControllerInstance()
    {
        m_cameraViewMatrixChangeHandler.Disconnect();
        AzToolsFramework::ViewportInteraction::ViewportInteractionNotificationBus::Handler::BusDisconnect();
        ModularViewportCameraControllerRequestBus::Handler::BusDisconnect();
    }

    bool ModularViewportCameraControllerInstance::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        const auto findModifierStatesFn = [viewportId = event.m_viewportId]
        {
            const auto* inputDevice =
                AzFramework::InputDeviceRequests::FindInputDevice(AzToolsFramework::GetSyntheticKeyboardDeviceId(viewportId));

            // grab keyboard channel (not important which) and check the modifier state (modifier state is shared for all keyboard channels)
            if (const auto it = inputDevice->GetInputChannelsById().find(AzFramework::InputDeviceKeyboard::Key::Alphanumeric0);
                it != inputDevice->GetInputChannelsById().end())
            {
                if (auto customData = it->second->GetCustomData<AzFramework::ModifierKeyStates>())
                {
                    return *customData;
                }
            }

            return AzFramework::ModifierKeyStates();
        };

        if (event.m_priority == m_priorityFn(m_cameraSystem))
        {
            AzFramework::WindowSize windowSize;
            AzFramework::WindowRequestBus::EventResult(
                windowSize, event.m_windowHandle, &AzFramework::WindowRequestBus::Events::GetRenderResolution);

            return m_cameraSystem.HandleEvents(AzFramework::BuildInputEvent(event.m_inputChannel, findModifierStatesFn(), windowSize));
        }

        return false;
    }

    void ModularViewportCameraControllerInstance::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
    {
        // only update for a single priority (normal is the default)
        if (event.m_priority != AzFramework::ViewportControllerPriority::Normal)
        {
            return;
        }

        m_updatingTransformInternally = true;

        if (m_cameraMode == CameraMode::Control)
        {
            m_targetCamera = m_cameraSystem.StepCamera(m_targetCamera, event.m_deltaTime.count());
            m_camera = AzFramework::SmoothCamera(m_camera, m_targetCamera, m_cameraProps, event.m_deltaTime.count());
            m_roll = AzFramework::SmoothValue(m_targetRoll, m_roll, m_cameraProps.m_rotateSmoothnessFn(), event.m_deltaTime.count());

            m_modularCameraViewportContext->SetCameraTransform(CombinedCameraTransform());
        }
        else if (m_cameraMode == CameraMode::Animation)
        {
            AZ_Assert(m_cameraAnimation.has_value(), "CameraAnimation is not set when in CameraMode::Animation");

            const auto smootherStepFn = [](const float t)
            {
                return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            };

            if (m_cameraAnimation->m_duration == 0.0f)
            {
                m_cameraAnimation->m_time = 1.0f; // set interpolation amount to end
            }
            else
            {
                m_cameraAnimation->m_time =
                    AZ::GetClamp(m_cameraAnimation->m_time + (event.m_deltaTime.count() / m_cameraAnimation->m_duration), 0.0f, 1.0f);
            }

            const auto& [transformStart, transformEnd, animationTime, animationDuration] = m_cameraAnimation.value();

            const float transitionTime = smootherStepFn(animationTime);
            const AZ::Transform current = AZ::Transform::CreateFromQuaternionAndTranslation(
                transformStart.GetRotation().Slerp(transformEnd.GetRotation(), transitionTime),
                transformStart.GetTranslation().Lerp(transformEnd.GetTranslation(), transitionTime));

            const AZ::Vector3 eulerAngles = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromTransform(current));
            m_camera.m_pitch = eulerAngles.GetX();
            m_camera.m_yaw = eulerAngles.GetZ();
            m_camera.m_pivot = current.GetTranslation();
            m_camera.m_offset = AZ::Vector3::CreateZero();
            m_targetRoll = eulerAngles.GetY();
            m_targetCamera = m_camera;

            m_modularCameraViewportContext->SetCameraTransform(current);

            if (animationTime >= 1.0f)
            {
                m_cameraMode = CameraMode::Control;
                m_cameraAnimation.reset();
            }
        }

        m_updatingTransformInternally = false;
    }

    bool ModularViewportCameraControllerInstance::InterpolateToTransform(const AZ::Transform& worldFromLocal, const float duration)
    {
        const auto& currentCameraTransform = CombinedCameraTransform();

        // ensure the transform we're interpolating to isn't the same as our current transform
        // and the transform we're setting isn't the same as one previously set
        if (!currentCameraTransform.IsClose(worldFromLocal) &&
            (!m_cameraAnimation.has_value() || !worldFromLocal.IsClose(m_cameraAnimation->m_transformEnd)))
        {
            m_cameraMode = CameraMode::Animation;
            m_cameraAnimation = CameraAnimation{ currentCameraTransform, worldFromLocal, 0.0f, duration };

            return true;
        }

        return false;
    }

    void ModularViewportCameraControllerInstance::SetCameraPivotAttached(const AZ::Vector3& pivot)
    {
        m_targetCamera.m_pivot = pivot;
    }

    void ModularViewportCameraControllerInstance::SetCameraPivotAttachedImmediate(const AZ::Vector3& pivot)
    {
        m_camera.m_pivot = pivot;
        m_targetCamera.m_pivot = pivot;
    }

    void ModularViewportCameraControllerInstance::SetCameraPivotDetached(const AZ::Vector3& pivot)
    {
        AzFramework::MovePivotDetached(m_targetCamera, pivot);
    }

    void ModularViewportCameraControllerInstance::SetCameraPivotDetachedImmediate(const AZ::Vector3& pivot)
    {
        AzFramework::MovePivotDetached(m_camera, pivot);
        AzFramework::MovePivotDetached(m_targetCamera, pivot);
    }

    void ModularViewportCameraControllerInstance::SetCameraOffset(const AZ::Vector3& offset)
    {
        m_targetCamera.m_offset = offset;
    }

    void ModularViewportCameraControllerInstance::SetCameraOffsetImmediate(const AZ::Vector3& offset)
    {
        m_camera.m_offset = offset;
        m_targetCamera.m_offset = offset;
    }

    void ModularViewportCameraControllerInstance::LookFromOrbit()
    {
        m_targetCamera.m_pivot = m_targetCamera.Translation();
        m_targetCamera.m_offset = AZ::Vector3::CreateZero();
        m_camera = m_targetCamera;
    }

    bool ModularViewportCameraControllerInstance::AddCameras(const AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>& cameraInputs)
    {
        return m_cameraSystem.m_cameras.AddCameras(cameraInputs);
    }

    bool ModularViewportCameraControllerInstance::RemoveCameras(
        const AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>& cameraInputs)
    {
        return m_cameraSystem.m_cameras.RemoveCameras(cameraInputs);
    }

    void ModularViewportCameraControllerInstance::ResetCameras()
    {
        m_cameraSystem.m_cameras.Reset();
    }

    bool ModularViewportCameraControllerInstance::IsInterpolating() const
    {
        return m_cameraMode == CameraMode::Animation;
    }

    void ModularViewportCameraControllerInstance::StartTrackingTransform(const AZ::Transform& worldFromLocal)
    {
        if (!m_storedCamera.has_value())
        {
            m_storedCamera = m_targetCamera;
        }

        const auto angles = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromQuaternion(worldFromLocal.GetRotation()));
        m_targetCamera.m_pitch = angles.GetX();
        m_targetCamera.m_yaw = angles.GetZ();
        m_targetCamera.m_offset = AZ::Vector3::CreateZero();
        m_targetCamera.m_pivot = worldFromLocal.GetTranslation();
        m_targetRoll = angles.GetY();

        m_camera = m_targetCamera;
        m_roll = m_targetRoll;

        ReconnectViewMatrixChangeHandler();
    }

    void ModularViewportCameraControllerInstance::StopTrackingTransform()
    {
        if (m_storedCamera.has_value())
        {
            m_targetCamera = m_storedCamera.value();
            m_targetRoll = 0.0f;

            m_camera = m_targetCamera;
            m_roll = m_targetRoll;
        }

        m_storedCamera.reset();

        ReconnectViewMatrixChangeHandler();
    }

    bool ModularViewportCameraControllerInstance::IsTrackingTransform() const
    {
        return m_storedCamera.has_value();
    }

    void ModularViewportCameraControllerInstance::OnViewportFocusOut()
    {
        ResetCameras();
    }

    void ModularViewportCameraControllerInstance::ReconnectViewMatrixChangeHandler()
    {
        m_cameraViewMatrixChangeHandler.Disconnect();
        m_modularCameraViewportContext->ConnectViewMatrixChangedHandler(m_cameraViewMatrixChangeHandler);
    }

    AZ::Transform PlaceholderModularCameraViewportContextImpl::GetCameraTransform() const
    {
        return m_cameraTransform;
    }

    void PlaceholderModularCameraViewportContextImpl::SetCameraTransform(const AZ::Transform& transform)
    {
        m_cameraTransform = transform;
        m_viewMatrixChangedEvent.Signal(
            AZ::Matrix4x4::CreateFromMatrix3x4(AzFramework::CameraViewFromCameraTransform(AZ::Matrix3x4::CreateFromTransform(transform))));
    }

    void PlaceholderModularCameraViewportContextImpl::ConnectViewMatrixChangedHandler(AZ::RPI::MatrixChangedEvent::Handler& handler)
    {
        handler.Connect(m_viewMatrixChangedEvent);
    }

    AZ::Transform ModularViewportCameraControllerInstance::CombinedCameraTransform() const
    {
        return m_camera.Transform() * AZ::Transform::CreateFromMatrix3x3(AZ::Matrix3x3::CreateRotationY(m_targetRoll));
    }
} // namespace AtomToolsFramework
