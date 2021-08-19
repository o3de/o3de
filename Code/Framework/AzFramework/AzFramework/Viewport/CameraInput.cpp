/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraInput.h"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/std/numeric.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Windowing/WindowBus.h>

namespace AzFramework
{
    AZ_CVAR(
        float,
        ed_cameraSystemDefaultPlaneHeight,
        34.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The default height of the ground plane to do intersection tests against when orbiting");
    AZ_CVAR(float, ed_cameraSystemMinOrbitDistance, 10.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, ed_cameraSystemMaxOrbitDistance, 50.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(
        bool,
        ed_cameraSystemUseCursor,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Should the camera use cursor absolute positions or motion deltas");

    //! return -1.0f if inverted, 1.0f otherwise
    constexpr static float Invert(const bool invert)
    {
        constexpr float Dir[] = { 1.0f, -1.0f };
        return Dir[aznumeric_cast<int>(invert)];
    };

    // maps a discrete motion input to a click detector click event (e.g. button down or up event)
    static ClickDetector::ClickEvent ClickFromInput(const InputEvent& event, const AzFramework::InputChannelId& inputChannelId)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == inputChannelId)
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
    }

    // begins a camera input after a sufficient movement has occurred and ends a
    // camera input once the initiating button is released
    static void HandleActivationEvents(
        const InputEvent& event,
        const AzFramework::InputChannelId& inputChannelId,
        const ScreenVector& cursorDelta,
        ClickDetector& clickDetector,
        CameraInput& cameraInput)
    {
        const auto clickEvent = ClickFromInput(event, inputChannelId);
        switch (const auto outcome = clickDetector.DetectClick(clickEvent, cursorDelta); outcome)
        {
        case ClickDetector::ClickOutcome::Move:
            cameraInput.BeginActivation();
            break;
        case ClickDetector::ClickOutcome::Release:
            cameraInput.EndActivation();
            break;
        default:
            // noop
            break;
        }
    }

    // returns true if a camera input is being updated after having been initiated from a
    // motion input (e.g. mouse move while button held)
    static bool CameraInputUpdatingAfterMotion(const CameraInput& cameraInput)
    {
        // note - must also check !ending to ensure the mouse up (release) event
        // is not consumed and can be propagated to other systems.
        // (don't swallow mouse up events)
        return !cameraInput.Idle() && !cameraInput.Ending();
    }

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
        if (const auto& cursor = AZStd::get_if<CursorEvent>(&event))
        {
            m_cursorState.SetCurrentPosition(cursor->m_position);
        }
        else if (const auto& horizontalMotion = AZStd::get_if<HorizontalMotionEvent>(&event))
        {
            m_motionDelta.m_x = horizontalMotion->m_delta;
        }
        else if (const auto& verticalMotion = AZStd::get_if<VerticalMotionEvent>(&event))
        {
            m_motionDelta.m_y = verticalMotion->m_delta;
        }
        else if (const auto& scroll = AZStd::get_if<ScrollEvent>(&event))
        {
            m_scrollDelta = scroll->m_delta;
        }

        m_handlingEvents =
            m_cameras.HandleEvents(event, ed_cameraSystemUseCursor ? m_cursorState.CursorDelta() : m_motionDelta, m_scrollDelta);

        return m_handlingEvents;
    }

    Camera CameraSystem::StepCamera(const Camera& targetCamera, const float deltaTime)
    {
        const auto nextCamera = m_cameras.StepCamera(
            targetCamera, ed_cameraSystemUseCursor ? m_cursorState.CursorDelta() : m_motionDelta, m_scrollDelta, deltaTime);

        m_cursorState.Update();
        m_motionDelta = ScreenVector{ 0, 0 };
        m_scrollDelta = 0.0f;

        return nextCamera;
    }

    void Cameras::AddCamera(AZStd::shared_ptr<CameraInput> cameraInput)
    {
        m_idleCameraInputs.push_back(AZStd::move(cameraInput));
    }

    bool Cameras::HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, const float scrollDelta)
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

    bool Cameras::Exclusive() const
    {
        return AZStd::any_of(
            m_activeCameraInputs.begin(), m_activeCameraInputs.end(),
            [](const auto& cameraInput)
            {
                return cameraInput->Exclusive();
            });
    }

    RotateCameraInput::RotateCameraInput(const InputChannelId& rotateChannelId)
        : m_rotateChannelId(rotateChannelId)
    {
        m_rotateSpeedFn = []() constexpr
        {
            return 0.005f;
        };

        m_invertPitchFn = []() constexpr
        {
            return false;
        };

        m_invertYawFn = []() constexpr
        {
            return false;
        };
    }

    bool RotateCameraInput::HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        HandleActivationEvents(event, m_rotateChannelId, cursorDelta, m_clickDetector, *this);
        return CameraInputUpdatingAfterMotion(*this);
    }

    Camera RotateCameraInput::StepCamera(
        const Camera& targetCamera,
        const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const float rotateSpeed = m_rotateSpeedFn();
        nextCamera.m_pitch -= float(cursorDelta.m_y) * rotateSpeed * Invert(m_invertPitchFn());
        nextCamera.m_yaw -= float(cursorDelta.m_x) * rotateSpeed * Invert(m_invertYawFn());

        const auto clampRotation = [](const float angle)
        {
            return AZStd::fmod(angle + AZ::Constants::TwoPi, AZ::Constants::TwoPi);
        };

        nextCamera.m_yaw = clampRotation(nextCamera.m_yaw);
        // clamp pitch to be +/-90 degrees
        nextCamera.m_pitch = AZ::GetClamp(nextCamera.m_pitch, -AZ::Constants::HalfPi, AZ::Constants::HalfPi);

        return nextCamera;
    }

    PanCameraInput::PanCameraInput(const InputChannelId& panChannelId, PanAxesFn panAxesFn)
        : m_panAxesFn(AZStd::move(panAxesFn))
        , m_panChannelId(panChannelId)
    {
        m_panSpeedFn = []() constexpr
        {
            return 0.01f;
        };

        m_invertPanXFn = []() constexpr
        {
            return true;
        };

        m_invertPanYFn = []() constexpr
        {
            return true;
        };
    }

    bool PanCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        HandleActivationEvents(event, m_panChannelId, cursorDelta, m_clickDetector, *this);
        return CameraInputUpdatingAfterMotion(*this);
    }

    Camera PanCameraInput::StepCamera(
        const Camera& targetCamera,
        const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto panAxes = m_panAxesFn(nextCamera);

        const float panSpeed = m_panSpeedFn();
        const auto deltaPanX = float(cursorDelta.m_x) * panAxes.m_horizontalAxis * panSpeed;
        const auto deltaPanY = float(cursorDelta.m_y) * panAxes.m_verticalAxis * panSpeed;

        nextCamera.m_lookAt += deltaPanX * Invert(m_invertPanXFn());
        nextCamera.m_lookAt += deltaPanY * -Invert(m_invertPanYFn());

        return nextCamera;
    }

    TranslateCameraInput::TranslationType TranslateCameraInput::TranslationFromKey(
        const InputChannelId& channelId, const TranslateCameraInputChannels& translateCameraInputChannels)
    {
        if (channelId == translateCameraInputChannels.m_forwardChannelId)
        {
            return TranslationType::Forward;
        }

        if (channelId == translateCameraInputChannels.m_backwardChannelId)
        {
            return TranslationType::Backward;
        }

        if (channelId == translateCameraInputChannels.m_leftChannelId)
        {
            return TranslationType::Left;
        }

        if (channelId == translateCameraInputChannels.m_rightChannelId)
        {
            return TranslationType::Right;
        }

        if (channelId == translateCameraInputChannels.m_downChannelId)
        {
            return TranslationType::Down;
        }

        if (channelId == translateCameraInputChannels.m_upChannelId)
        {
            return TranslationType::Up;
        }

        return TranslationType::Nil;
    }

    TranslateCameraInput::TranslateCameraInput(
        TranslationAxesFn translationAxesFn, const TranslateCameraInputChannels& translateCameraInputChannels)
        : m_translationAxesFn(AZStd::move(translationAxesFn))
        , m_translateCameraInputChannels(translateCameraInputChannels)
    {
        m_translateSpeedFn = []() constexpr
        {
            return 10.0f;
        };

        m_boostMultiplierFn = []() constexpr
        {
            return 3.0f;
        };
    }

    bool TranslateCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_state == InputChannel::State::Began)
            {
                m_translation |= TranslationFromKey(input->m_channelId, m_translateCameraInputChannels);
                if (m_translation != TranslationType::Nil)
                {
                    BeginActivation();
                }

                if (input->m_channelId == m_translateCameraInputChannels.m_boostChannelId)
                {
                    m_boost = true;
                }
            }
            // ensure we don't process end events in the idle state
            else if (input->m_state == InputChannel::State::Ended && !Idle())
            {
                m_translation &= ~(TranslationFromKey(input->m_channelId, m_translateCameraInputChannels));
                if (m_translation == TranslationType::Nil)
                {
                    EndActivation();
                }
                if (input->m_channelId == m_translateCameraInputChannels.m_boostChannelId)
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

        const float speed = [boost = m_boost, &translateSpeedFn = m_translateSpeedFn, &boostMultiplierFn = m_boostMultiplierFn]()
        {
            return translateSpeedFn() * (boost ? boostMultiplierFn() : 1.0f);
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

    OrbitCameraInput::OrbitCameraInput(const InputChannelId& orbitChannelId)
        : m_orbitChannelId(orbitChannelId)
    {
    }

    bool OrbitCameraInput::HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, const float scrollDelta)
    {
        if (const auto* input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == m_orbitChannelId)
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

    OrbitDollyScrollCameraInput::OrbitDollyScrollCameraInput()
    {
        m_scrollSpeedFn = []() constexpr
        {
            return 0.03f;
        };
    }

    bool OrbitDollyScrollCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
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
        nextCamera.m_lookDist = AZ::GetMin(nextCamera.m_lookDist + scrollDelta * m_scrollSpeedFn(), 0.0f);
        EndActivation();
        return nextCamera;
    }

    OrbitDollyCursorMoveCameraInput::OrbitDollyCursorMoveCameraInput(const InputChannelId& dollyChannelId)
        : m_dollyChannelId(dollyChannelId)
    {
        m_cursorSpeedFn = []() constexpr
        {
            return 0.01f;
        };
    }

    bool OrbitDollyCursorMoveCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        HandleActivationEvents(event, m_dollyChannelId, cursorDelta, m_clickDetector, *this);
        return CameraInputUpdatingAfterMotion(*this);
    }

    Camera OrbitDollyCursorMoveCameraInput::StepCamera(
        const Camera& targetCamera,
        const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;
        nextCamera.m_lookDist = AZ::GetMin(nextCamera.m_lookDist + float(cursorDelta.m_y) * m_cursorSpeedFn(), 0.0f);
        return nextCamera;
    }

    ScrollTranslationCameraInput::ScrollTranslationCameraInput()
    {
        m_scrollSpeedFn = []() constexpr
        {
            return 0.02f;
        };
    }

    bool ScrollTranslationCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
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

        nextCamera.m_lookAt += axisY * scrollDelta * m_scrollSpeedFn();

        EndActivation();

        return nextCamera;
    }

    Camera SmoothCamera(const Camera& currentCamera, const Camera& targetCamera, const CameraProps& cameraProps, const float deltaTime)
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
        if (cameraProps.m_rotateSmoothingEnabledFn())
        {
            const float lookRate = AZStd::exp2(cameraProps.m_rotateSmoothnessFn());
            const float lookTime = AZStd::exp2(-lookRate * deltaTime);
            camera.m_pitch = AZ::Lerp(targetCamera.m_pitch, currentCamera.m_pitch, lookTime);
            camera.m_yaw = AZ::Lerp(targetYaw, currentYaw, lookTime);
        }
        else
        {
            camera.m_pitch = targetCamera.m_pitch;
            camera.m_yaw = targetYaw;
        }

        if (cameraProps.m_translateSmoothingEnabledFn())
        {
            const float moveRate = AZStd::exp2(cameraProps.m_translateSmoothnessFn());
            const float moveTime = AZStd::exp2(-moveRate * deltaTime);
            camera.m_lookDist = AZ::Lerp(targetCamera.m_lookDist, currentCamera.m_lookDist, moveTime);
            camera.m_lookAt = targetCamera.m_lookAt.Lerp(currentCamera.m_lookAt, moveTime);
        }
        else
        {
            camera.m_lookDist = targetCamera.m_lookDist;
            camera.m_lookAt = targetCamera.m_lookAt;
        }

        return camera;
    }

    InputEvent BuildInputEvent(const InputChannel& inputChannel, const WindowSize& windowSize)
    {
        const auto& inputChannelId = inputChannel.GetInputChannelId();
        const auto& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();

        const bool wasMouseButton = AZStd::any_of(
            InputDeviceMouse::Button::All.begin(), InputDeviceMouse::Button::All.end(),
            [inputChannelId](const auto& button)
            {
                return button == inputChannelId;
            });

        // accept active mouse channel updates, inactive movement channels will just have a 0 delta
        if (inputChannel.IsActive())
        {
            if (inputChannelId == InputDeviceMouse::SystemCursorPosition)
            {
                const auto* position = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
                AZ_Assert(position, "Expected PositionData2D but found nullptr");

                return CursorEvent{ ScreenPoint(
                    static_cast<int>(position->m_normalizedPosition.GetX() * windowSize.m_width),
                    static_cast<int>(position->m_normalizedPosition.GetY() * windowSize.m_height)) };
            }
            else if (inputChannelId == InputDeviceMouse::Movement::X)
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
        }

        if (wasMouseButton || InputDeviceKeyboard::IsKeyboardDevice(inputDeviceId))
        {
            return DiscreteInputEvent{ inputChannelId, inputChannel.GetState() };
        }

        return AZStd::monostate{};
    }
} // namespace AzFramework
