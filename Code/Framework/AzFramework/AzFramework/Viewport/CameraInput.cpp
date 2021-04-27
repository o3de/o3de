/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include "CameraInput.h"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Plane.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Windowing/WindowBus.h>

namespace AzFramework
{
    void CameraSystem::HandleEvents(const InputEvent& event)
    {
        if (const auto& cursor_motion = AZStd::get_if<CursorMotionEvent>(&event))
        {
            m_currentCursorPosition = cursor_motion->m_position;
        }
        else if (const auto& scroll = AZStd::get_if<ScrollEvent>(&event))
        {
            m_scrollDelta = scroll->m_delta;
        }

        m_cameras.HandleEvents(event);
    }

    Camera CameraSystem::StepCamera(const Camera& targetCamera, const float deltaTime)
    {
        const auto cursorDelta = m_currentCursorPosition.has_value() && m_lastCursorPosition.has_value()
            ? m_currentCursorPosition.value() - m_lastCursorPosition.value()
            : ScreenVector(0, 0);

        if (m_currentCursorPosition.has_value())
        {
            m_lastCursorPosition = m_currentCursorPosition;
        }

        const auto nextCamera = m_cameras.StepCamera(targetCamera, cursorDelta, m_scrollDelta, deltaTime);

        m_scrollDelta = 0.0f;

        return nextCamera;
    }

    void Cameras::AddCamera(AZStd::shared_ptr<CameraInput> cameraInput)
    {
        m_idleCameraInputs.push_back(AZStd::move(cameraInput));
    }

    void Cameras::HandleEvents(const InputEvent& event)
    {
        for (auto& cameraInput : m_activeCameraInputs)
        {
            cameraInput->HandleEvents(event);
        }

        for (auto& cameraInput : m_idleCameraInputs)
        {
            cameraInput->HandleEvents(event);
        }
    }

    Camera Cameras::StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, const float deltaTime)
    {
        for (int i = 0; i < m_idleCameraInputs.size();)
        {
            auto& cameraInput = m_idleCameraInputs[i];
            const bool canBegin = cameraInput->Beginning() &&
                std::all_of(m_activeCameraInputs.cbegin(), m_activeCameraInputs.cend(),
                            [](const auto& input) { return !input->Exclusive(); }) &&
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

        // accumulate
        Camera nextCamera = targetCamera;
        for (auto& cameraInput : m_activeCameraInputs)
        {
            nextCamera = cameraInput->StepCamera(nextCamera, cursorDelta, scrollDelta, deltaTime);
        }

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
            m_activeCameraInputs[i] = m_activeCameraInputs[m_activeCameraInputs.size() - 1];
            m_activeCameraInputs.pop_back();
        }
    }

    void RotateCameraInput::HandleEvents(const InputEvent& event)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == m_channelId)
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
    }

    Camera RotateCameraInput::StepCamera(
        const Camera& targetCamera, const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        nextCamera.m_pitch -= float(cursorDelta.m_y) * m_props.m_rotateSpeed;
        nextCamera.m_yaw -= float(cursorDelta.m_x) * m_props.m_rotateSpeed;

        const auto clampRotation = [](const float angle) { return std::fmod(angle + AZ::Constants::TwoPi, AZ::Constants::TwoPi); };

        nextCamera.m_yaw = clampRotation(nextCamera.m_yaw);
        // clamp pitch to be +-90 degrees
        nextCamera.m_pitch = AZ::GetClamp(nextCamera.m_pitch, -AZ::Constants::HalfPi, AZ::Constants::HalfPi);

        return nextCamera;
    }

    void PanCameraInput::HandleEvents(const InputEvent& event)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == InputDeviceMouse::Button::Middle)
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
    }

    Camera PanCameraInput::StepCamera(
        const Camera& targetCamera, const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto panAxes = m_panAxesFn(nextCamera);

        const auto deltaPanX = float(cursorDelta.m_x) * panAxes.m_horizontalAxis * m_props.m_panSpeed;
        const auto deltaPanY = float(cursorDelta.m_y) * panAxes.m_verticalAxis * m_props.m_panSpeed;

        const auto inv = [](const bool invert) {
            constexpr float Dir[] = {1.0f, -1.0f};
            return Dir[static_cast<int>(invert)];
        };

        nextCamera.m_lookAt += deltaPanX * inv(m_props.m_panInvertX);
        nextCamera.m_lookAt += deltaPanY * -inv(m_props.m_panInvertY);

        return nextCamera;
    }

    TranslateCameraInput::TranslationType TranslateCameraInput::translationFromKey(InputChannelId channelId)
    {
        // note: remove hard-coded InputDevice keys
        if (channelId == InputDeviceKeyboard::Key::AlphanumericW)
        {
            return TranslationType::Forward;
        }

        if (channelId == InputDeviceKeyboard::Key::AlphanumericS)
        {
            return TranslationType::Backward;
        }

        if (channelId == InputDeviceKeyboard::Key::AlphanumericA)
        {
            return TranslationType::Left;
        }

        if (channelId == InputDeviceKeyboard::Key::AlphanumericD)
        {
            return TranslationType::Right;
        }

        if (channelId == InputDeviceKeyboard::Key::AlphanumericQ)
        {
            return TranslationType::Down;
        }

        if (channelId == InputDeviceKeyboard::Key::AlphanumericE)
        {
            return TranslationType::Up;
        }

        return TranslationType::Nil;
    }

    void TranslateCameraInput::HandleEvents(const InputEvent& event)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_state == InputChannel::State::Began)
            {
                if (input->m_state == InputChannel::State::Updated)
                {
                    return;
                }

                m_translation |= translationFromKey(input->m_channelId);
                if (m_translation != TranslationType::Nil)
                {
                    BeginActivation();
                }

                if (input->m_channelId == InputDeviceKeyboard::Key::ModifierShiftL)
                {
                    m_boost = true;
                }
            }
            else if (input->m_state == InputChannel::State::Ended)
            {
                m_translation ^= translationFromKey(input->m_channelId);
                if (m_translation == TranslationType::Nil)
                {
                    EndActivation();
                }
                if (input->m_channelId == InputDeviceKeyboard::Key::ModifierShiftL)
                {
                    m_boost = false;
                }
            }
        }
    }

    Camera TranslateCameraInput::StepCamera(
        const Camera& targetCamera, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta,
        const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto translationBasis = m_translationAxesFn(nextCamera);
        const auto axisX = translationBasis.GetBasisX();
        const auto axisY = translationBasis.GetBasisY();
        const auto axisZ = translationBasis.GetBasisZ();

        const float speed = [boost = m_boost, props = m_props]() {
            return props.m_translateSpeed * (boost ? props.m_boostMultiplier : 1.0f);
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

    void OrbitCameraInput::HandleEvents(const InputEvent& event)
    {
        if (const auto* input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == InputDeviceKeyboard::Key::ModifierAltL)
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
            m_orbitCameras.HandleEvents(event);
        }
    }

    Camera OrbitCameraInput::StepCamera(
        const Camera& targetCamera, const ScreenVector& cursorDelta, const float scrollDelta, const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        if (Beginning())
        {
            float hit_distance = 0.0f;
            if (AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateZero())
                    .CastRay(targetCamera.Translation(), targetCamera.Rotation().GetBasisY() * m_props.m_maxOrbitDistance, hit_distance))
            {
                nextCamera.m_lookDist = -hit_distance;
                nextCamera.m_lookAt = targetCamera.Translation() + targetCamera.Rotation().GetBasisY() * hit_distance;
            }
            else
            {
                nextCamera.m_lookDist = -m_props.m_defaultOrbitDistance;
                nextCamera.m_lookAt = targetCamera.Translation() + targetCamera.Rotation().GetBasisY() * m_props.m_defaultOrbitDistance;
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

    void OrbitDollyScrollCameraInput::HandleEvents(const InputEvent& event)
    {
        if (const auto* scroll = AZStd::get_if<ScrollEvent>(&event))
        {
            BeginActivation();
        }
    }

    Camera OrbitDollyScrollCameraInput::StepCamera(
        const Camera& targetCamera, [[maybe_unused]] const ScreenVector& cursorDelta, const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;
        nextCamera.m_lookDist = AZ::GetMin(nextCamera.m_lookDist + scrollDelta * m_props.m_dollySpeed, 0.0f);
        EndActivation();
        return nextCamera;
    }

    void OrbitDollyCursorMoveCameraInput::HandleEvents(const InputEvent& event)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == InputDeviceMouse::Button::Right)
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
    }

    Camera OrbitDollyCursorMoveCameraInput::StepCamera(
        const Camera& targetCamera, const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;
        nextCamera.m_lookDist = AZ::GetMin(nextCamera.m_lookDist + float(cursorDelta.m_y) * m_props.m_dollySpeed, 0.0f);
        return nextCamera;
    }

    void ScrollTranslationCameraInput::HandleEvents(const InputEvent& event)
    {
        if (const auto* scroll = AZStd::get_if<ScrollEvent>(&event))
        {
            BeginActivation();
        }
    }

    Camera ScrollTranslationCameraInput::StepCamera(
        const Camera& targetCamera, [[maybe_unused]] const ScreenVector& cursorDelta, const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto translation_basis = LookTranslation(nextCamera);
        const auto axisY = translation_basis.GetBasisY();

        nextCamera.m_lookAt += axisY * scrollDelta * m_props.m_translateSpeed;

        EndActivation();

        return nextCamera;
    }

    Camera SmoothCamera(const Camera& currentCamera, const Camera& targetCamera, const SmoothProps& props, const float deltaTime)
    {
        const auto clamp_rotation = [](const float angle) { return std::fmod(angle + AZ::Constants::TwoPi, AZ::Constants::TwoPi); };

        // keep yaw in 0 - 360 range
        float targetYaw = clamp_rotation(targetCamera.m_yaw);
        const float currentYaw = clamp_rotation(currentCamera.m_yaw);

        // return the sign of the float input (-1, 0, 1)
        const auto sign = [](const float value) { return aznumeric_cast<float>((0.0f < value) - (value < 0.0f)); };

        // ensure smooth transition when moving across 0 - 360 boundary
        const float yawDelta = targetYaw - currentYaw;
        if (std::abs(yawDelta) >= AZ::Constants::Pi)
        {
            targetYaw -= AZ::Constants::TwoPi * sign(yawDelta);
        }

        Camera camera;
        // note: the math for the lerp smoothing implementation for camera rotation and translation was inspired by this excellent
        // article by Scott Lembcke: https://www.gamasutra.com/blogs/ScottLembcke/20180404/316046/Improved_Lerp_Smoothing.php
        const float lookRate = std::exp2(props.m_lookSmoothness);
        const float lookT = std::exp2(-lookRate * deltaTime);
        camera.m_pitch = AZ::Lerp(targetCamera.m_pitch, currentCamera.m_pitch, lookT);
        camera.m_yaw = AZ::Lerp(targetYaw, currentYaw, lookT);
        const float moveRate = std::exp2(props.m_moveSmoothness);
        const float moveT = std::exp2(-moveRate * deltaTime);
        camera.m_lookDist = AZ::Lerp(targetCamera.m_lookDist, currentCamera.m_lookDist, moveT);
        camera.m_lookAt = targetCamera.m_lookAt.Lerp(currentCamera.m_lookAt, moveT);
        return camera;
    }

    InputEvent BuildInputEvent(const InputChannel& inputChannel, const WindowSize& windowSize)
    {
        const auto& inputChannelId = inputChannel.GetInputChannelId();
        const auto& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();

        if (inputChannelId == InputDeviceMouse::SystemCursorPosition)
        {
            AZ::Vector2 systemCursorPositionNormalized = AZ::Vector2::CreateZero();
            InputSystemCursorRequestBus::EventResult(
                systemCursorPositionNormalized, inputDeviceId, &InputSystemCursorRequestBus::Events::GetSystemCursorPositionNormalized);

            return CursorMotionEvent{ScreenPoint(
                systemCursorPositionNormalized.GetX() * windowSize.m_width, systemCursorPositionNormalized.GetY() * windowSize.m_height)};
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Z)
        {
            return ScrollEvent{inputChannel.GetValue()};
        }
        else if (InputDeviceMouse::IsMouseDevice(inputDeviceId) || InputDeviceKeyboard::IsKeyboardDevice(inputDeviceId))
        {
            return DiscreteInputEvent{inputChannelId, inputChannel.GetState()};
        }

        return AZStd::monostate{};
    }
} // namespace AzFramework
