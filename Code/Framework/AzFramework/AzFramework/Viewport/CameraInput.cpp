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
        camera.m_pivot = transform.GetTranslation();
        camera.m_offset = AZ::Vector3::CreateZero();
    }

    bool CameraSystem::HandleEvents(const InputEvent& event)
    {
        if (const auto& cursor = AZStd::get_if<CursorEvent>(&event))
        {
            m_cursorState.SetCurrentPosition(cursor->m_position);
            m_cursorState.SetCaptured(cursor->m_captured);
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

        nextCamera.m_yaw = WrapYawRotation(nextCamera.m_yaw);
        nextCamera.m_pitch = ClampPitchRotation(nextCamera.m_pitch);

        return nextCamera;
    }

    void RotateCameraInput::SetRotateInputChannelId(const InputChannelId& rotateChannelId)
    {
        m_rotateChannelId = rotateChannelId;
    }

    PanCameraInput::PanCameraInput(const InputChannelId& panChannelId, PanAxesFn panAxesFn, TranslationDeltaFn translationDeltaFn)
        : m_panAxesFn(AZStd::move(panAxesFn))
        , m_panChannelId(panChannelId)
        , m_translationDeltaFn(translationDeltaFn)
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
        const auto deltaPanX = aznumeric_cast<float>(cursorDelta.m_x) * panAxes.m_horizontalAxis * panSpeed;
        const auto deltaPanY = aznumeric_cast<float>(cursorDelta.m_y) * panAxes.m_verticalAxis * panSpeed;

        m_translationDeltaFn(nextCamera, deltaPanX * Invert(m_invertPanXFn()));
        m_translationDeltaFn(nextCamera, deltaPanY * -Invert(m_invertPanYFn()));

        return nextCamera;
    }

    void PanCameraInput::SetPanInputChannelId(const InputChannelId& panChannelId)
    {
        m_panChannelId = panChannelId;
    }

    TranslateCameraInput::TranslationType TranslateCameraInput::TranslationFromKey(
        const InputChannelId& channelId, const TranslateCameraInputChannelIds& translateCameraInputChannelIds)
    {
        if (channelId == translateCameraInputChannelIds.m_forwardChannelId)
        {
            return TranslationType::Forward;
        }

        if (channelId == translateCameraInputChannelIds.m_backwardChannelId)
        {
            return TranslationType::Backward;
        }

        if (channelId == translateCameraInputChannelIds.m_leftChannelId)
        {
            return TranslationType::Left;
        }

        if (channelId == translateCameraInputChannelIds.m_rightChannelId)
        {
            return TranslationType::Right;
        }

        if (channelId == translateCameraInputChannelIds.m_downChannelId)
        {
            return TranslationType::Down;
        }

        if (channelId == translateCameraInputChannelIds.m_upChannelId)
        {
            return TranslationType::Up;
        }

        return TranslationType::Nil;
    }

    TranslateCameraInput::TranslateCameraInput(
        const TranslateCameraInputChannelIds& translateCameraInputChannelIds,
        TranslationAxesFn translationAxesFn,
        TranslationDeltaFn translateDeltaFn)
        : m_translationAxesFn(AZStd::move(translationAxesFn))
        , m_translateDeltaFn(AZStd::move(translateDeltaFn))
        , m_translateCameraInputChannelIds(translateCameraInputChannelIds)
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
                m_translation |= TranslationFromKey(input->m_channelId, m_translateCameraInputChannelIds);
                if (m_translation != TranslationType::Nil)
                {
                    BeginActivation();
                }

                if (input->m_channelId == m_translateCameraInputChannelIds.m_boostChannelId)
                {
                    m_boost = true;
                }
            }
            // ensure we don't process end events in the idle state
            else if (input->m_state == InputChannel::State::Ended && !Idle())
            {
                m_translation &= ~(TranslationFromKey(input->m_channelId, m_translateCameraInputChannelIds));
                if (m_translation == TranslationType::Nil)
                {
                    EndActivation();
                }
                if (input->m_channelId == m_translateCameraInputChannelIds.m_boostChannelId)
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
            m_translateDeltaFn(nextCamera, axisY * speed * deltaTime);
        }

        if ((m_translation & TranslationType::Backward) == TranslationType::Backward)
        {
            m_translateDeltaFn(nextCamera, -axisY * speed * deltaTime);
        }

        if ((m_translation & TranslationType::Left) == TranslationType::Left)
        {
            m_translateDeltaFn(nextCamera, -axisX * speed * deltaTime);
        }

        if ((m_translation & TranslationType::Right) == TranslationType::Right)
        {
            m_translateDeltaFn(nextCamera, axisX * speed * deltaTime);
        }

        if ((m_translation & TranslationType::Up) == TranslationType::Up)
        {
            m_translateDeltaFn(nextCamera, axisZ * speed * deltaTime);
        }

        if ((m_translation & TranslationType::Down) == TranslationType::Down)
        {
            m_translateDeltaFn(nextCamera, -axisZ * speed * deltaTime);
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

    void TranslateCameraInput::SetTranslateCameraInputChannelIds(const TranslateCameraInputChannelIds& translateCameraInputChannelIds)
    {
        m_translateCameraInputChannelIds = translateCameraInputChannelIds;
    }

    OrbitCameraInput::OrbitCameraInput(const InputChannelId& orbitChannelId)
        : m_orbitChannelId(orbitChannelId)
    {
        m_pivotFn = []([[maybe_unused]] const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& direction)
        {
            return AZ::Vector3::CreateZero();
        };
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
            nextCamera.m_pivot = m_pivotFn(targetCamera.Translation(), targetCamera.Rotation().GetBasisY());
            nextCamera.m_offset = nextCamera.View().TransformPoint(targetCamera.Translation());
        }

        if (Active())
        {
            MovePivotDetached(nextCamera, m_pivotFn(targetCamera.Translation(), targetCamera.Rotation().GetBasisY()));
            nextCamera = m_orbitCameras.StepCamera(nextCamera, cursorDelta, scrollDelta, deltaTime);
        }

        if (Ending())
        {
            m_orbitCameras.Reset();

            nextCamera.m_pivot = nextCamera.Translation();
            nextCamera.m_offset = AZ::Vector3::CreateZero();
        }

        return nextCamera;
    }

    void OrbitCameraInput::SetOrbitInputChannelId(const InputChannelId& orbitChanneId)
    {
        m_orbitChannelId = orbitChanneId;
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

    static Camera OrbitDolly(const Camera& targetCamera, const float delta)
    {
        Camera nextCamera = targetCamera;

        // handle case where pivot and offset may be the same to begin with
        // choose negative y-axis for offset to default to moving the camera backwards from the pivot (standard centered pivot behavior)
        const auto pivotDirection = [&targetCamera]
        {
            if (const auto offsetLength = targetCamera.m_offset.GetLength(); AZ::IsCloseMag(offsetLength, 0.0f))
            {
                return -AZ::Vector3::CreateAxisY();
            }
            else
            {
                return targetCamera.m_offset / offsetLength;
            }
        }();

        nextCamera.m_offset -= pivotDirection * delta;
        if (pivotDirection.Dot(nextCamera.m_offset) < 0.0f)
        {
            nextCamera.m_offset = pivotDirection * 0.001f;
        }

        return nextCamera;
    }

    Camera OrbitDollyScrollCameraInput::StepCamera(
        const Camera& targetCamera,
        [[maybe_unused]] const ScreenVector& cursorDelta,
        const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        const auto nextCamera = OrbitDolly(targetCamera, aznumeric_cast<float>(scrollDelta) * m_scrollSpeedFn());
        EndActivation();
        return nextCamera;
    }

    OrbitDollyMotionCameraInput::OrbitDollyMotionCameraInput(const InputChannelId& dollyChannelId)
        : m_dollyChannelId(dollyChannelId)
    {
        m_motionSpeedFn = []() constexpr
        {
            return 0.01f;
        };
    }

    bool OrbitDollyMotionCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        HandleActivationEvents(event, m_dollyChannelId, cursorDelta, m_clickDetector, *this);
        return CameraInputUpdatingAfterMotion(*this);
    }

    Camera OrbitDollyMotionCameraInput::StepCamera(
        const Camera& targetCamera,
        const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        return OrbitDolly(targetCamera, aznumeric_cast<float>(cursorDelta.m_y) * m_motionSpeedFn());
    }

    void OrbitDollyMotionCameraInput::SetDollyInputChannelId(const InputChannelId& dollyChannelId)
    {
        m_dollyChannelId = dollyChannelId;
    }

    LookScrollTranslationCameraInput::LookScrollTranslationCameraInput()
    {
        m_scrollSpeedFn = []() constexpr
        {
            return 0.02f;
        };
    }

    bool LookScrollTranslationCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        if (const auto* scroll = AZStd::get_if<ScrollEvent>(&event))
        {
            BeginActivation();
        }

        return !Idle();
    }

    Camera LookScrollTranslationCameraInput::StepCamera(
        const Camera& targetCamera,
        [[maybe_unused]] const ScreenVector& cursorDelta,
        const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto translation_basis = LookTranslation(nextCamera);
        const auto axisY = translation_basis.GetBasisY();

        nextCamera.m_pivot += axisY * scrollDelta * m_scrollSpeedFn();

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
            camera.m_pivot = targetCamera.m_pivot.Lerp(currentCamera.m_pivot, moveTime);
            camera.m_offset = targetCamera.m_offset.Lerp(currentCamera.m_offset, moveTime);
        }
        else
        {
            camera.m_pivot = targetCamera.m_pivot;
            camera.m_offset = targetCamera.m_offset;
        }

        return camera;
    }

    FocusCameraInput::FocusCameraInput(const InputChannelId& focusChannelId, FocusOffsetFn offsetFn)
        : m_focusChannelId(focusChannelId)
        , m_offsetFn(offsetFn)
    {
    }

    bool FocusCameraInput::HandleEvents(
        const InputEvent& event, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        if (const auto* input = AZStd::get_if<DiscreteInputEvent>(&event))
        {
            if (input->m_channelId == m_focusChannelId && input->m_state == InputChannel::State::Began)
            {
                BeginActivation();
            }
        }

        return !Idle();
    }

    Camera FocusCameraInput::StepCamera(
        const Camera& targetCamera,
        [[maybe_unused]] const ScreenVector& cursorDelta,
        [[maybe_unused]] float scrollDelta,
        [[maybe_unused]] float deltaTime)
    {
        if (Beginning())
        {
            // as the camera starts, record the camera we would like to end up as
            m_nextCamera.m_offset = m_offsetFn(m_pivotFn().GetDistance(targetCamera.Translation()));
            const auto angles =
                EulerAngles(AZ::Matrix3x3::CreateFromMatrix3x4(AZ::Matrix3x4::CreateLookAt(targetCamera.Translation(), m_pivotFn())));
            m_nextCamera.m_pitch = angles.GetX();
            m_nextCamera.m_yaw = angles.GetZ();
            m_nextCamera.m_pivot = targetCamera.m_pivot;
        }

        // end the behavior when the camera is in alignment
        if (AZ::IsCloseMag(targetCamera.m_pitch, m_nextCamera.m_pitch) && AZ::IsCloseMag(targetCamera.m_yaw, m_nextCamera.m_yaw))
        {
            EndActivation();
        }

        return m_nextCamera;
    }

    void FocusCameraInput::SetPivotFn(PivotFn pivotFn)
    {
        m_pivotFn = AZStd::move(pivotFn);
    }

    void FocusCameraInput::SetFocusInputChannelId(const InputChannelId& focusChannelId)
    {
        m_focusChannelId = focusChannelId;
    }

    bool CustomCameraInput::HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, const float scrollDelta)
    {
        return m_handleEventsFn(*this, event, cursorDelta, scrollDelta);
    }

    Camera CustomCameraInput::StepCamera(
        const Camera& targetCamera, const ScreenVector& cursorDelta, const float scrollDelta, const float deltaTime)
    {
        return m_stepCameraFn(*this, targetCamera, cursorDelta, scrollDelta, deltaTime);
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

                auto currentCursorState = AzFramework::SystemCursorState::Unknown;
                AzFramework::InputSystemCursorRequestBus::EventResult(
                    currentCursorState, inputDeviceId, &AzFramework::InputSystemCursorRequestBus::Events::GetSystemCursorState);

                const auto x = position->m_normalizedPosition.GetX() * aznumeric_cast<float>(windowSize.m_width);
                const auto y = position->m_normalizedPosition.GetY() * aznumeric_cast<float>(windowSize.m_height);
                return CursorEvent{ ScreenPoint(aznumeric_cast<int>(AZStd::lround(x)), aznumeric_cast<int>(AZStd::lround(y))),
                                    currentCursorState == AzFramework::SystemCursorState::ConstrainedAndHidden };
            }
            else if (inputChannelId == InputDeviceMouse::Movement::X)
            {
                const auto x = inputChannel.GetValue();
                return HorizontalMotionEvent{ aznumeric_cast<int>(AZStd::lround(x)) };
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Y)
            {
                const auto y = inputChannel.GetValue();
                return VerticalMotionEvent{ aznumeric_cast<int>(AZStd::lround(y)) };
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
