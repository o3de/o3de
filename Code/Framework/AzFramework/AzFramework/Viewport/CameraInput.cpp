/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraInput.h"

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/std/numeric.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

namespace AzFramework
{
    AZ_CVAR(
        float,
        ed_cameraSystemDefaultPlaneHeight,
        34.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The default height of the ground plane to do intersection tests against when orbiting");
    AZ_CVAR(float, ed_cameraSystemBoostMultiplier, 3.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemTranslateSpeed, 10.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemOrbitDollyScrollSpeed, 0.02f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemOrbitDollyCursorSpeed, 0.01f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemScrollTranslateSpeed, 0.02f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemMinOrbitDistance, 10.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemMaxOrbitDistance, 50.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemLookSmoothness, 5.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemTranslateSmoothness, 5.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemRotateSpeed, 0.005f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemPanSpeed, 0.01f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(bool, ed_cameraSystemPanInvertX, true, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(bool, ed_cameraSystemPanInvertY, true, nullptr, AZ::ConsoleFunctorFlags::Null, "");

    AZ_CVAR(
        AZ::CVarFixedString, ed_cameraSystemTranslateForwardKey, "keyboard_key_alphanumeric_W", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(
        AZ::CVarFixedString,
        ed_cameraSystemTranslateBackwardKey,
        "keyboard_key_alphanumeric_S",
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "");
    AZ_CVAR(
        AZ::CVarFixedString, ed_cameraSystemTranslateLeftKey, "keyboard_key_alphanumeric_A", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(
        AZ::CVarFixedString, ed_cameraSystemTranslateRightKey, "keyboard_key_alphanumeric_D", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::CVarFixedString, ed_cameraSystemTranslateUpKey, "keyboard_key_alphanumeric_E", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(
        AZ::CVarFixedString, ed_cameraSystemTranslateDownKey, "keyboard_key_alphanumeric_Q", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(
        AZ::CVarFixedString, ed_cameraSystemTranslateBoostKey, "keyboard_key_modifier_shift_l", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::CVarFixedString, ed_cameraSystemOrbitKey, "keyboard_key_modifier_alt_l", nullptr, AZ::ConsoleFunctorFlags::Null, "");

    AZ_CVAR(AZ::CVarFixedString, ed_cameraSystemFreeLookButton, "mouse_button_right", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::CVarFixedString, ed_cameraSystemFreePanButton, "mouse_button_middle", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::CVarFixedString, ed_cameraSystemOrbitLookButton, "mouse_button_left", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::CVarFixedString, ed_cameraSystemOrbitDollyButton, "mouse_button_right", nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::CVarFixedString, ed_cameraSystemOrbitPanButton, "mouse_button_middle", nullptr, AZ::ConsoleFunctorFlags::Null, "");

    static InputChannelId CameraTranslateForwardId;
    static InputChannelId CameraTranslateBackwardId;
    static InputChannelId CameraTranslateLeftId;
    static InputChannelId CameraTranslateRightId;
    static InputChannelId CameraTranslateDownId;
    static InputChannelId CameraTranslateUpId;
    static InputChannelId CameraTranslateBoostId;
    static InputChannelId CameraOrbitId;

    // externed elsewhere
    InputChannelId CameraFreeLookButton;
    InputChannelId CameraFreePanButton;
    InputChannelId CameraOrbitLookButton;
    InputChannelId CameraOrbitDollyButton;
    InputChannelId CameraOrbitPanButton;

    void ReloadCameraKeyBindings()
    {
        const AZ::CVarFixedString& forward = ed_cameraSystemTranslateForwardKey;
        CameraTranslateForwardId = InputChannelId(forward.c_str());
        const AZ::CVarFixedString& backward = ed_cameraSystemTranslateBackwardKey;
        CameraTranslateBackwardId = InputChannelId(backward.c_str());
        const AZ::CVarFixedString& left = ed_cameraSystemTranslateLeftKey;
        CameraTranslateLeftId = InputChannelId(left.c_str());
        const AZ::CVarFixedString& right = ed_cameraSystemTranslateRightKey;
        CameraTranslateRightId = InputChannelId(right.c_str());
        const AZ::CVarFixedString& down = ed_cameraSystemTranslateDownKey;
        CameraTranslateDownId = InputChannelId(down.c_str());
        const AZ::CVarFixedString& up = ed_cameraSystemTranslateUpKey;
        CameraTranslateUpId = InputChannelId(up.c_str());
        const AZ::CVarFixedString& boost = ed_cameraSystemTranslateBoostKey;
        CameraTranslateBoostId = InputChannelId(boost.c_str());
        const AZ::CVarFixedString& orbit = ed_cameraSystemOrbitKey;
        CameraOrbitId = InputChannelId(orbit.c_str());
        const AZ::CVarFixedString& freeLook = ed_cameraSystemFreeLookButton;
        CameraFreeLookButton = InputChannelId(freeLook.c_str());
        const AZ::CVarFixedString& freePan = ed_cameraSystemFreePanButton;
        CameraFreePanButton = InputChannelId(freePan.c_str());
        const AZ::CVarFixedString& orbitLook = ed_cameraSystemOrbitLookButton;
        CameraOrbitLookButton = InputChannelId(orbitLook.c_str());
        const AZ::CVarFixedString& orbitDolly = ed_cameraSystemOrbitDollyButton;
        CameraOrbitDollyButton = InputChannelId(orbitDolly.c_str());
        const AZ::CVarFixedString& orbitPan = ed_cameraSystemOrbitPanButton;
        CameraOrbitPanButton = InputChannelId(orbitPan.c_str());
    }

    static void ReloadCameraKeyBindingsConsole(const AZ::ConsoleCommandContainer&)
    {
        ReloadCameraKeyBindings();
    }

    AZ_CONSOLEFREEFUNC(ReloadCameraKeyBindingsConsole, AZ::ConsoleFunctorFlags::Null, "Reload keybindings for the modern camera system");

    // Based on paper by David Eberly - https://www.geometrictools.com/Documentation/EulerAngles.pdf
    AZ::Vector3 EulerAngles(const AZ::Matrix3x3& orientation)
    {
        float x;
        float y;
        float z;

        // 2.4 Factor as RzRyRx
        if (orientation.GetElement(2, 0) < 1.0f)
        {
            if (orientation.GetElement(2, 0) > -1.0f)
            {
                x = AZStd::atan2(orientation.GetElement(2, 1), orientation.GetElement(2, 2));
                y = AZStd::asin(-orientation.GetElement(2, 0));
                z = AZStd::atan2(orientation.GetElement(1, 0), orientation.GetElement(0, 0));
            }
            else
            {
                x = 0.0f;
                y = AZ::Constants::Pi * 0.5f;
                z = -AZStd::atan2(-orientation.GetElement(2, 1), orientation.GetElement(1, 1));
            }
        }
        else
        {
            x = 0.0f;
            y = -AZ::Constants::Pi * 0.5f;
            z = AZStd::atan2(-orientation.GetElement(1, 2), orientation.GetElement(1, 1));
        }

        return { x, y, z };
    }

    void UpdateCameraFromTransform(Camera& camera, const AZ::Transform& transform)
    {
        const auto eulerAngles = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromTransform(transform));

        camera.m_pitch = eulerAngles.GetX();
        camera.m_yaw = eulerAngles.GetZ();
        // note: m_lookDist is negative so we must invert it here
        camera.m_lookAt = transform.GetTranslation() + (camera.Rotation().GetBasisY() * -camera.m_lookDist);
    }

    bool CameraSystem::HandleEvents(const InputEvent& event)
    {
        if (const auto& horizonalMotion = AZStd::get_if<HorizontalMotionEvent>(&event))
        {
            m_motionDelta.m_x = horizonalMotion->m_delta;
        }
        else if (const auto& verticalMotion = AZStd::get_if<VerticalMotionEvent>(&event))
        {
            m_motionDelta.m_y = verticalMotion->m_delta;
        }
        else if (const auto& scroll = AZStd::get_if<ScrollEvent>(&event))
        {
            m_scrollDelta = scroll->m_delta;
        }

        return m_cameras.HandleEvents(event, m_motionDelta, m_scrollDelta);
    }

    Camera CameraSystem::StepCamera(const Camera& targetCamera, const float deltaTime)
    {
        const auto nextCamera = m_cameras.StepCamera(targetCamera, m_motionDelta, m_scrollDelta, deltaTime);

        m_motionDelta = ScreenVector{ 0, 0 };
        m_scrollDelta = 0.0f;

        return nextCamera;
    }

    void Cameras::AddCamera(AZStd::shared_ptr<CameraInput> cameraInput)
    {
        m_idleCameraInputs.push_back(AZStd::move(cameraInput));
    }

    bool Cameras::HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta)
    {
        bool handling = false;
        for (auto& cameraInput : m_activeCameraInputs)
        {
            handling = cameraInput->HandleEvents(event, cursorDelta, scrollDelta) || handling;
        }

        for (auto& cameraInput : m_idleCameraInputs)
        {
            handling = cameraInput->HandleEvents(event, cursorDelta, scrollDelta) || handling;
        }

        return handling;
    }

    Camera Cameras::StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, const float scrollDelta, const float deltaTime)
    {
        for (int i = 0; i < m_idleCameraInputs.size();)
        {
            auto& cameraInput = m_idleCameraInputs[i];
            const bool canBegin = cameraInput->Beginning() &&
                AZStd::all_of(m_activeCameraInputs.cbegin(), m_activeCameraInputs.cend(),
                              [](const auto& input)
                              {
                                  return !input->Exclusive();
                              }) &&
                (!cameraInput->Exclusive() || (cameraInput->Exclusive() && m_activeCameraInputs.empty()));

            if (canBegin)
            {
                m_activeCameraInputs.push_back(cameraInput);
                using AZStd::swap;
                swap(m_idleCameraInputs[i], m_idleCameraInputs[m_idleCameraInputs.size() - 1]);
                m_idleCameraInputs.pop_back();
            }
            else
            {
                i++;
            }
        }

        const Camera nextCamera = AZStd::accumulate(
            AZStd::begin(m_activeCameraInputs), AZStd::end(m_activeCameraInputs), targetCamera,
            [cursorDelta, scrollDelta, deltaTime](Camera acc, auto& camera)
            {
                acc = camera->StepCamera(acc, cursorDelta, scrollDelta, deltaTime);
                return acc;
            });

        for (int i = 0; i < m_activeCameraInputs.size();)
        {
            auto& cameraInput = m_activeCameraInputs[i];
            if (cameraInput->Ending())
            {
                cameraInput->ClearActivation();
                m_idleCameraInputs.push_back(cameraInput);
                using AZStd::swap;
                swap(m_activeCameraInputs[i], m_activeCameraInputs[m_activeCameraInputs.size() - 1]);
                m_activeCameraInputs.pop_back();
            }
            else
            {
                cameraInput->ContinueActivation();
                i++;
            }
        }

        return nextCamera;
    }

    void Cameras::Reset()
    {
        for (int i = 0; i < m_activeCameraInputs.size();)
        {
            m_activeCameraInputs[i]->Reset();
            m_idleCameraInputs.push_back(m_activeCameraInputs[i]);
            using AZStd::swap;
            swap(m_activeCameraInputs[i], m_activeCameraInputs[m_activeCameraInputs.size() - 1]);
            m_activeCameraInputs.pop_back();
        }
    }

    void Cameras::Clear()
    {
        Reset();
        AZ_Assert(m_activeCameraInputs.empty(), "Active Camera Inputs is not empty");

        m_idleCameraInputs.clear();
    }

    RotateCameraInput::RotateCameraInput(const InputChannelId rotateChannelId)
        : m_rotateChannelId(rotateChannelId)
    {
    }

    bool RotateCameraInput::HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        const ClickDetector::ClickEvent clickEvent = [&event, this]
        {
            if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
            {
                if (input->m_channelId == m_rotateChannelId)
                {
                    if (input->m_state == InputChannel::State::Began)
                    {
                        return ClickDetector::ClickEvent::Down;
                    }
                    else if (input->m_state == InputChannel::State::Ended)
                    {
                        return ClickDetector::ClickEvent::Up;
                    }
                }
            }
            return ClickDetector::ClickEvent::Nil;
        }();

        switch (const auto outcome = m_clickDetector.DetectClick(clickEvent, cursorDelta); outcome)
        {
        case ClickDetector::ClickOutcome::Move:
            BeginActivation();
            break;
        case ClickDetector::ClickOutcome::Release:
            EndActivation();
            break;
        default:
            // noop
            break;
        }

        // note - must also check !ending to ensure the mouse up (release) event
        // is not consumed and can be propagated to other systems.
        // (don't swallow mouse up events)
        return !Idle() && !Ending();
    }

    Camera RotateCameraInput::StepCamera(
        const Camera& targetCamera,
        const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        nextCamera.m_pitch -= float(cursorDelta.m_y) * ed_cameraSystemRotateSpeed;
        nextCamera.m_yaw -= float(cursorDelta.m_x) * ed_cameraSystemRotateSpeed;

        const auto clampRotation = [](const float angle)
        {
            return AZStd::fmod(angle + AZ::Constants::TwoPi, AZ::Constants::TwoPi);
        };

        nextCamera.m_yaw = clampRotation(nextCamera.m_yaw);
        // clamp pitch to be +-90 degrees
        nextCamera.m_pitch = AZ::GetClamp(nextCamera.m_pitch, -AZ::Constants::HalfPi, AZ::Constants::HalfPi);

        return nextCamera;
    }

    PanCameraInput::PanCameraInput(const InputChannelId panChannelId, PanAxesFn panAxesFn)
        : m_panAxesFn(AZStd::move(panAxesFn))
        , m_panChannelId(panChannelId)
    {
    }

    bool PanCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == m_panChannelId)
            {
                if (input->m_state == InputChannel::State::Began)
                {
                    BeginActivation();
                }
                else if (input->m_state == InputChannel::State::Ended)
                {
                    EndActivation();
                }
            }
        }

        return !Idle();
    }

    Camera PanCameraInput::StepCamera(
        const Camera& targetCamera,
        const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto panAxes = m_panAxesFn(nextCamera);

        const auto deltaPanX = float(cursorDelta.m_x) * panAxes.m_horizontalAxis * ed_cameraSystemPanSpeed;
        const auto deltaPanY = float(cursorDelta.m_y) * panAxes.m_verticalAxis * ed_cameraSystemPanSpeed;

        const auto inv = [](const bool invert)
        {
            constexpr float Dir[] = { 1.0f, -1.0f };
            return Dir[aznumeric_cast<int>(invert)];
        };

        nextCamera.m_lookAt += deltaPanX * inv(ed_cameraSystemPanInvertX);
        nextCamera.m_lookAt += deltaPanY * -inv(ed_cameraSystemPanInvertY);

        return nextCamera;
    }

    TranslateCameraInput::TranslationType TranslateCameraInput::translationFromKey(InputChannelId channelId)
    {
        if (channelId == CameraTranslateForwardId)
        {
            return TranslationType::Forward;
        }

        if (channelId == CameraTranslateBackwardId)
        {
            return TranslationType::Backward;
        }

        if (channelId == CameraTranslateLeftId)
        {
            return TranslationType::Left;
        }

        if (channelId == CameraTranslateRightId)
        {
            return TranslationType::Right;
        }

        if (channelId == CameraTranslateDownId)
        {
            return TranslationType::Down;
        }

        if (channelId == CameraTranslateUpId)
        {
            return TranslationType::Up;
        }

        return TranslationType::Nil;
    }

    TranslateCameraInput::TranslateCameraInput(TranslationAxesFn translationAxesFn)
        : m_translationAxesFn(AZStd::move(translationAxesFn))
    {
    }

    bool TranslateCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_state == InputChannel::State::Began)
            {
                m_translation |= translationFromKey(input->m_channelId);
                if (m_translation != TranslationType::Nil)
                {
                    BeginActivation();
                }

                if (input->m_channelId == CameraTranslateBoostId)
                {
                    m_boost = true;
                }
            }
            // ensure we don't process end events in the idle state
            else if (input->m_state == InputChannel::State::Ended && !Idle())
            {
                m_translation &= ~(translationFromKey(input->m_channelId));
                if (m_translation == TranslationType::Nil)
                {
                    EndActivation();
                }
                if (input->m_channelId == CameraTranslateBoostId)
                {
                    m_boost = false;
                }
            }
        }

        return !Idle();
    }

    Camera TranslateCameraInput::StepCamera(
        const Camera& targetCamera,
        [[maybe_unused]] const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto translationBasis = m_translationAxesFn(nextCamera);
        const auto axisX = translationBasis.GetBasisX();
        const auto axisY = translationBasis.GetBasisY();
        const auto axisZ = translationBasis.GetBasisZ();

        const float speed = [boost = m_boost]()
        {
            return ed_cameraSystemTranslateSpeed * (boost ? ed_cameraSystemBoostMultiplier : 1.0f);
        }();

        if ((m_translation & TranslationType::Forward) == TranslationType::Forward)
        {
            nextCamera.m_lookAt += axisY * speed * deltaTime;
        }

        if ((m_translation & TranslationType::Backward) == TranslationType::Backward)
        {
            nextCamera.m_lookAt -= axisY * speed * deltaTime;
        }

        if ((m_translation & TranslationType::Left) == TranslationType::Left)
        {
            nextCamera.m_lookAt -= axisX * speed * deltaTime;
        }

        if ((m_translation & TranslationType::Right) == TranslationType::Right)
        {
            nextCamera.m_lookAt += axisX * speed * deltaTime;
        }

        if ((m_translation & TranslationType::Up) == TranslationType::Up)
        {
            nextCamera.m_lookAt += axisZ * speed * deltaTime;
        }

        if ((m_translation & TranslationType::Down) == TranslationType::Down)
        {
            nextCamera.m_lookAt -= axisZ * speed * deltaTime;
        }

        if (Ending())
        {
            m_translation = TranslationType::Nil;
        }

        return nextCamera;
    }

    void TranslateCameraInput::ResetImpl()
    {
        m_translation = TranslationType::Nil;
        m_boost = false;
    }

    bool OrbitCameraInput::HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta)
    {
        if (const auto* input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == CameraOrbitId)
            {
                if (input->m_state == InputChannel::State::Began)
                {
                    BeginActivation();
                }
                else if (input->m_state == InputChannel::State::Ended)
                {
                    EndActivation();
                }
            }
        }

        if (Active())
        {
            return m_orbitCameras.HandleEvents(event, cursorDelta, scrollDelta);
        }

        return !Idle();
    }

    Camera OrbitCameraInput::StepCamera(
        const Camera& targetCamera, const ScreenVector& cursorDelta, const float scrollDelta, const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        if (Beginning())
        {
            const auto hasLookAt = [&nextCamera, &targetCamera, &lookAtFn = m_lookAtFn]
            {
                if (lookAtFn)
                {
                    // pass through the camera's position and look vector for use in the lookAt function
                    if (const auto lookAt = lookAtFn(targetCamera.Translation(), targetCamera.Rotation().GetBasisY()))
                    {
                        auto transform = AZ::Transform::CreateLookAt(targetCamera.m_lookAt, *lookAt);
                        nextCamera.m_lookDist = -lookAt->GetDistance(targetCamera.m_lookAt);
                        UpdateCameraFromTransform(nextCamera, transform);

                        return true;
                    }
                }
                return false;
            }();

            if (!hasLookAt)
            {
                float hit_distance = 0.0f;
                AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisZ(ed_cameraSystemDefaultPlaneHeight))
                    .CastRay(targetCamera.Translation(), targetCamera.Rotation().GetBasisY(), hit_distance);

                if (hit_distance > 0.0f)
                {
                    hit_distance = AZStd::min<float>(hit_distance, ed_cameraSystemMaxOrbitDistance);
                    nextCamera.m_lookDist = -hit_distance;
                    nextCamera.m_lookAt = targetCamera.Translation() + targetCamera.Rotation().GetBasisY() * hit_distance;
                }
                else
                {
                    nextCamera.m_lookDist = -ed_cameraSystemMinOrbitDistance;
                    nextCamera.m_lookAt =
                        targetCamera.Translation() + targetCamera.Rotation().GetBasisY() * ed_cameraSystemMinOrbitDistance;
                }
            }
        }

        if (Active())
        {
            nextCamera = m_orbitCameras.StepCamera(nextCamera, cursorDelta, scrollDelta, deltaTime);
        }

        if (Ending())
        {
            m_orbitCameras.Reset();

            nextCamera.m_lookAt = nextCamera.Translation();
            nextCamera.m_lookDist = 0.0f;
        }

        return nextCamera;
    }

    bool OrbitDollyScrollCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        if (const auto* scroll = AZStd::get_if<ScrollEvent>(&event))
        {
            BeginActivation();
        }

        return !Idle();
    }

    Camera OrbitDollyScrollCameraInput::StepCamera(
        const Camera& targetCamera,
        [[maybe_unused]] const ScreenVector& cursorDelta,
        const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;
        nextCamera.m_lookDist = AZ::GetMin(nextCamera.m_lookDist + scrollDelta * ed_cameraSystemOrbitDollyScrollSpeed, 0.0f);
        EndActivation();
        return nextCamera;
    }

    OrbitDollyCursorMoveCameraInput::OrbitDollyCursorMoveCameraInput(const InputChannelId dollyChannelId)
        : m_dollyChannelId(dollyChannelId)
    {
    }

    bool OrbitDollyCursorMoveCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == m_dollyChannelId)
            {
                if (input->m_state == InputChannel::State::Began)
                {
                    BeginActivation();
                }
                else if (input->m_state == InputChannel::State::Ended)
                {
                    EndActivation();
                }
            }
        }

        return !Idle();
    }

    Camera OrbitDollyCursorMoveCameraInput::StepCamera(
        const Camera& targetCamera,
        const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;
        nextCamera.m_lookDist = AZ::GetMin(nextCamera.m_lookDist + float(cursorDelta.m_y) * ed_cameraSystemOrbitDollyCursorSpeed, 0.0f);
        return nextCamera;
    }

    bool ScrollTranslationCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        if (const auto* scroll = AZStd::get_if<ScrollEvent>(&event))
        {
            BeginActivation();
        }

        return !Idle();
    }

    Camera ScrollTranslationCameraInput::StepCamera(
        const Camera& targetCamera,
        [[maybe_unused]] const ScreenVector& cursorDelta,
        const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto translation_basis = LookTranslation(nextCamera);
        const auto axisY = translation_basis.GetBasisY();

        nextCamera.m_lookAt += axisY * scrollDelta * ed_cameraSystemScrollTranslateSpeed;

        EndActivation();

        return nextCamera;
    }

    Camera SmoothCamera(const Camera& currentCamera, const Camera& targetCamera, const float deltaTime)
    {
        const auto clamp_rotation = [](const float angle)
        {
            return AZStd::fmod(angle + AZ::Constants::TwoPi, AZ::Constants::TwoPi);
        };

        // keep yaw in 0 - 360 range
        float targetYaw = clamp_rotation(targetCamera.m_yaw);
        const float currentYaw = clamp_rotation(currentCamera.m_yaw);

        // return the sign of the float input (-1, 0, 1)
        const auto sign = [](const float value)
        {
            return aznumeric_cast<float>((0.0f < value) - (value < 0.0f));
        };

        // ensure smooth transition when moving across 0 - 360 boundary
        const float yawDelta = targetYaw - currentYaw;
        if (AZStd::abs(yawDelta) >= AZ::Constants::Pi)
        {
            targetYaw -= AZ::Constants::TwoPi * sign(yawDelta);
        }

        Camera camera;
        // note: the math for the lerp smoothing implementation for camera rotation and translation was inspired by this excellent
        // article by Scott Lembcke: https://www.gamasutra.com/blogs/ScottLembcke/20180404/316046/Improved_Lerp_Smoothing.php
        const float lookRate = AZStd::exp2(ed_cameraSystemLookSmoothness);
        const float lookT = AZStd::exp2(-lookRate * deltaTime);
        camera.m_pitch = AZ::Lerp(targetCamera.m_pitch, currentCamera.m_pitch, lookT);
        camera.m_yaw = AZ::Lerp(targetYaw, currentYaw, lookT);
        const float moveRate = AZStd::exp2(ed_cameraSystemTranslateSmoothness);
        const float moveT = AZStd::exp2(-moveRate * deltaTime);
        camera.m_lookDist = AZ::Lerp(targetCamera.m_lookDist, currentCamera.m_lookDist, moveT);
        camera.m_lookAt = targetCamera.m_lookAt.Lerp(currentCamera.m_lookAt, moveT);
        return camera;
    }

    InputEvent BuildInputEvent(const InputChannel& inputChannel)
    {
        const auto& inputChannelId = inputChannel.GetInputChannelId();
        const auto& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();

        const bool wasMouseButton = AZStd::any_of(
            InputDeviceMouse::Button::All.begin(), InputDeviceMouse::Button::All.end(),
            [inputChannelId](const auto& button)
            {
                return button == inputChannelId;
            });

        if (inputChannelId == InputDeviceMouse::Movement::X)
        {
            return HorizontalMotionEvent{ aznumeric_cast<int>(inputChannel.GetValue()) };
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Y)
        {
            return VerticalMotionEvent{ aznumeric_cast<int>(inputChannel.GetValue()) };
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Z)
        {
            return ScrollEvent{ inputChannel.GetValue() };
        }
        else if (wasMouseButton || InputDeviceKeyboard::IsKeyboardDevice(inputDeviceId))
        {
            return DiscreteInputEvent{ inputChannelId, inputChannel.GetState() };
        }

        return AZStd::monostate{};
    }
} // namespace AzFramework
