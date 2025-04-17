/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraInput.h"

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
    static ClickDetector::ClickEvent ClickFromInput(const InputState& state, const AzFramework::InputChannelId& inputChannelId)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&state.m_inputEvent))
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
        const InputState& state,
        const AzFramework::InputChannelId& inputChannelId,
        const ScreenVector& cursorDelta,
        ClickDetector& clickDetector,
        CameraInput& cameraInput)
    {
        const auto clickEvent = ClickFromInput(state, inputChannelId);
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

        // 2.5 Factor as RzRxRy
        if (orientation.GetElement(2, 1) < 1.0f)
        {
            if (orientation.GetElement(2, 1) > -1.0f)
            {
                x = AZStd::asin(orientation.GetElement(2, 1));
                y = AZStd::atan2(-orientation.GetElement(2, 0), orientation.GetElement(2, 2));
                z = AZStd::atan2(-orientation.GetElement(0, 1), orientation.GetElement(1, 1));
            }
            else
            {
                x = -AZ::Constants::Pi * 0.5f;
                y = 0.0f;
                z = -AZStd::atan2(orientation.GetElement(0, 2), orientation.GetElement(0, 0));
            }
        }
        else
        {
            x = AZ::Constants::Pi * 0.5f;
            y = 0.0f;
            z = AZStd::atan2(orientation.GetElement(0, 2), orientation.GetElement(0, 0));
        }

        return { x, y, z };
    }

    void UpdateCameraFromTransform(Camera& camera, const AZ::Transform& transform)
    {
        UpdateCameraFromTranslationAndRotation(
            camera, transform.GetTranslation(), AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromTransform(transform)));
    }

    void UpdateCameraFromTranslationAndRotation(Camera& camera, const AZ::Vector3& translation, const AZ::Vector3& eulerAngles)
    {
        camera.m_pitch = eulerAngles.GetX();
        camera.m_yaw = eulerAngles.GetZ();
        camera.m_pivot = translation;
        camera.m_offset = AZ::Vector3::CreateZero();
    }

    float SmoothValueTime(const float smoothness, const float deltaTime)
    {
        // note: the math for the lerp smoothing implementation for camera rotation and translation was inspired by this excellent
        // article by Scott Lembcke: https://www.gamasutra.com/blogs/ScottLembcke/20180404/316046/Improved_Lerp_Smoothing.php
        const float rate = AZStd::exp2(smoothness);
        return AZStd::exp2(-rate * deltaTime);
    }

    float SmoothValue(const float target, const float current, const float time)
    {
        return AZ::Lerp(target, current, time);
    }

    float SmoothValue(const float target, const float current, const float smoothness, const float deltaTime)
    {
        return SmoothValue(target, current, SmoothValueTime(smoothness, deltaTime));
    }

    bool CameraSystem::HandleEvents(const InputState& state)
    {
        if (const auto& cursor = AZStd::get_if<CursorEvent>(&state.m_inputEvent))
        {
            m_cursorState.SetCurrentPosition(cursor->m_position);
            m_cursorState.SetCaptured(cursor->m_captured);
        }
        else if (const auto& horizontalMotion = AZStd::get_if<HorizontalMotionEvent>(&state.m_inputEvent))
        {
            m_motionDelta.m_x = horizontalMotion->m_delta;
        }
        else if (const auto& verticalMotion = AZStd::get_if<VerticalMotionEvent>(&state.m_inputEvent))
        {
            m_motionDelta.m_y = verticalMotion->m_delta;
        }
        else if (const auto& scroll = AZStd::get_if<ScrollEvent>(&state.m_inputEvent))
        {
            m_scrollDelta = scroll->m_delta;
        }

        m_handlingEvents =
            m_cameras.HandleEvents(state, ed_cameraSystemUseCursor ? m_cursorState.CursorDelta() : m_motionDelta, m_scrollDelta);

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

    bool Cameras::AddCamera(AZStd::shared_ptr<CameraInput> cameraInput)
    {
        const auto idleCameraIt = AZStd::find(m_idleCameraInputs.begin(), m_idleCameraInputs.end(), cameraInput);
        const auto activeCameraIt = AZStd::find(m_activeCameraInputs.begin(), m_activeCameraInputs.end(), cameraInput);

        if (idleCameraIt == m_idleCameraInputs.end() && activeCameraIt == m_activeCameraInputs.end())
        {
            m_idleCameraInputs.push_back(AZStd::move(cameraInput));
            return true;
        }

        return false;
    }

    bool Cameras::AddCameras(const AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>& cameraInputs)
    {
        bool allAdded = true;
        for (auto cameraInput : cameraInputs)
        {
            allAdded = AddCamera(AZStd::move(cameraInput)) && allAdded;
        }
        return allAdded;
    }

    bool Cameras::RemoveCamera(const AZStd::shared_ptr<CameraInput>& cameraInput)
    {
        if (const auto idleCameraIt = AZStd::find(m_idleCameraInputs.begin(), m_idleCameraInputs.end(), cameraInput);
            idleCameraIt != m_idleCameraInputs.end())
        {
            const auto idleIndex = idleCameraIt - m_idleCameraInputs.begin();
            using AZStd::swap;
            swap(m_idleCameraInputs[idleIndex], m_idleCameraInputs[m_idleCameraInputs.size() - 1]);
            m_idleCameraInputs.pop_back();

            return true;
        }

        if (const auto activeCameraIt = AZStd::find(m_activeCameraInputs.begin(), m_activeCameraInputs.end(), cameraInput);
            activeCameraIt != m_activeCameraInputs.end())
        {
            (*activeCameraIt)->Reset();

            const auto activeIndex = activeCameraIt - m_idleCameraInputs.begin();
            using AZStd::swap;
            swap(m_activeCameraInputs[activeIndex], m_activeCameraInputs[m_activeCameraInputs.size() - 1]);
            m_activeCameraInputs.pop_back();

            return true;
        }

        return false;
    }

    bool Cameras::RemoveCameras(const AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>& cameraInputs)
    {
        bool allRemoved = true;
        for (const auto& cameraInput : cameraInputs)
        {
            allRemoved = RemoveCamera(cameraInput) && allRemoved;
        }
        return allRemoved;
    }

    bool Cameras::HandleEvents(const InputState& state, const ScreenVector& cursorDelta, const float scrollDelta)
    {
        bool handling = false;
        for (auto& cameraInput : m_activeCameraInputs)
        {
            handling = cameraInput->HandleEvents(state, cursorDelta, scrollDelta) || handling;
        }

        for (auto& cameraInput : m_idleCameraInputs)
        {
            handling = cameraInput->HandleEvents(state, cursorDelta, scrollDelta) || handling;
        }

        return handling;
    }

    Camera Cameras::StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, const float scrollDelta, const float deltaTime)
    {
        for (int idleIndex = 0; idleIndex < m_idleCameraInputs.size();)
        {
            auto& cameraInput = m_idleCameraInputs[idleIndex];
            const bool canBegin = cameraInput->Beginning() &&
                AZStd::all_of(m_activeCameraInputs.cbegin(),
                              m_activeCameraInputs.cend(),
                              [](const auto& input)
                              {
                                  return !input->Exclusive();
                              }) &&
                (!cameraInput->Exclusive() || m_activeCameraInputs.empty());

            if (canBegin)
            {
                m_activeCameraInputs.push_back(cameraInput);
                using AZStd::swap;
                swap(m_idleCameraInputs[idleIndex], m_idleCameraInputs[m_idleCameraInputs.size() - 1]);
                m_idleCameraInputs.pop_back();
            }
            else
            {
                // if a camera attempted to start but was not allowed to, ensure activation is cancelled
                if (!cameraInput->Idle())
                {
                    cameraInput->CancelActivation();
                }

                idleIndex++;
            }
        }

        const Camera nextCamera = AZStd::accumulate(
            AZStd::begin(m_activeCameraInputs),
            AZStd::end(m_activeCameraInputs),
            targetCamera,
            [cursorDelta, scrollDelta, deltaTime](Camera acc, auto& camera)
            {
                acc = camera->StepCamera(acc, cursorDelta, scrollDelta, deltaTime);
                return acc;
            });

        for (int activeIndex = 0; activeIndex < m_activeCameraInputs.size();)
        {
            auto& cameraInput = m_activeCameraInputs[activeIndex];
            if (cameraInput->Ending())
            {
                cameraInput->ClearActivation();
                m_idleCameraInputs.push_back(cameraInput);
                using AZStd::swap;
                swap(m_activeCameraInputs[activeIndex], m_activeCameraInputs[m_activeCameraInputs.size() - 1]);
                m_activeCameraInputs.pop_back();
            }
            else
            {
                cameraInput->ContinueActivation();
                activeIndex++;
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
            m_activeCameraInputs.begin(),
            m_activeCameraInputs.end(),
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

        m_constrainPitch = []() constexpr
        {
            return true;
        };
    }

    bool RotateCameraInput::HandleEvents(const InputState& state, const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        HandleActivationEvents(state, m_rotateChannelId, cursorDelta, m_clickDetector, *this);
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
        const float deltaPitch = aznumeric_cast<float>(cursorDelta.m_y) * rotateSpeed * Invert(m_invertPitchFn());
        const float deltaYaw = aznumeric_cast<float>(cursorDelta.m_x) * rotateSpeed * Invert(m_invertYawFn());

        nextCamera.m_pitch -= deltaPitch;
        nextCamera.m_yaw -= deltaYaw;
        nextCamera.m_yaw = WrapYawRotation(nextCamera.m_yaw);

        if (m_constrainPitch())
        {
            nextCamera.m_pitch = ClampPitchRotation(nextCamera.m_pitch);
        }

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
        const InputState& state, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        HandleActivationEvents(state, m_panChannelId, cursorDelta, m_clickDetector, *this);
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
        const InputState& state, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        if (const auto& input = AZStd::get_if<DiscreteInputEvent>(&state.m_inputEvent))
        {
            m_boost = state.m_modifiers.IsActive(GetCorrespondingModifierKeyMask(m_translateCameraInputChannelIds.m_boostChannelId));

            if (input->m_state == InputChannel::State::Began)
            {
                if (auto translation = TranslationFromKey(input->m_channelId, m_translateCameraInputChannelIds);
                    translation != TranslationType::Nil)
                {
                    m_translation |= translation;
                    BeginActivation();
                }
            }
            // ensure we don't process end events in the idle state
            else if (input->m_state == InputChannel::State::Ended && !Idle())
            {
                if (auto translation = TranslationFromKey(input->m_channelId, m_translateCameraInputChannelIds);
                    translation != TranslationType::Nil)
                {
                    m_translation &= ~translation;
                    if (m_translation == TranslationType::Nil)
                    {
                        EndActivation();
                    }
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

    bool OrbitCameraInput::HandleEvents(const InputState& state, const ScreenVector& cursorDelta, const float scrollDelta)
    {
        // event action outcome
        enum class Action
        {
            Nothing,
            Begin,
            End
        };

        const Action action = [&state, orbitChannelId = m_orbitChannelId]
        {
            // check for valid event
            if (!AZStd::get_if<CursorEvent>(&state.m_inputEvent) && !AZStd::get_if<DiscreteInputEvent>(&state.m_inputEvent))
            {
                return Action::Nothing;
            }

            // poll modifiers
            if (state.m_modifiers.IsActive(GetCorrespondingModifierKeyMask(orbitChannelId)))
            {
                return Action::Begin;
            }

            return Action::End;
        }();

        if (action == Action::Begin && Idle())
        {
            BeginActivation();
        }
        else if (action == Action::End && Active())
        {
            EndActivation();
        }

        if (Active())
        {
            return m_orbitCameras.HandleEvents(state, cursorDelta, scrollDelta);
        }

        return !Idle();
    }

    Camera OrbitCameraInput::StepCamera(
        const Camera& targetCamera, const ScreenVector& cursorDelta, const float scrollDelta, const float deltaTime)
    {
        Camera nextCamera = targetCamera;

        const auto pivot = m_pivotFn(targetCamera.Translation(), targetCamera.Rotation().GetBasisY());

        if (Beginning())
        {
            nextCamera.m_pivot = pivot;
            nextCamera.m_offset = nextCamera.View().TransformPoint(targetCamera.Translation());
        }

        if (Active())
        {
            MovePivotDetached(nextCamera, pivot);
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

    OrbitScrollDollyCameraInput::OrbitScrollDollyCameraInput()
    {
        m_scrollSpeedFn = []() constexpr
        {
            return 0.03f;
        };
    }

    bool OrbitScrollDollyCameraInput::HandleEvents(
        const InputState& state, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        if (AZStd::get_if<ScrollEvent>(&state.m_inputEvent))
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

    Camera OrbitScrollDollyCameraInput::StepCamera(
        const Camera& targetCamera,
        [[maybe_unused]] const ScreenVector& cursorDelta,
        const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        const auto nextCamera = OrbitDolly(targetCamera, aznumeric_cast<float>(scrollDelta) * m_scrollSpeedFn());
        EndActivation();
        return nextCamera;
    }

    OrbitMotionDollyCameraInput::OrbitMotionDollyCameraInput(const InputChannelId& dollyChannelId)
        : m_dollyChannelId(dollyChannelId)
    {
        m_motionSpeedFn = []() constexpr
        {
            return 0.01f;
        };
    }

    bool OrbitMotionDollyCameraInput::HandleEvents(
        const InputState& state, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        HandleActivationEvents(state, m_dollyChannelId, cursorDelta, m_clickDetector, *this);
        return CameraInputUpdatingAfterMotion(*this);
    }

    Camera OrbitMotionDollyCameraInput::StepCamera(
        const Camera& targetCamera,
        const ScreenVector& cursorDelta,
        [[maybe_unused]] const float scrollDelta,
        [[maybe_unused]] const float deltaTime)
    {
        return OrbitDolly(targetCamera, aznumeric_cast<float>(cursorDelta.m_y) * m_motionSpeedFn());
    }

    void OrbitMotionDollyCameraInput::SetDollyInputChannelId(const InputChannelId& dollyChannelId)
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
        const InputState& state, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] const float scrollDelta)
    {
        if (AZStd::get_if<ScrollEvent>(&state.m_inputEvent))
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
        const auto clampRotation = [](const float angle)
        {
            return AZStd::fmod(angle + AZ::Constants::TwoPi, AZ::Constants::TwoPi);
        };

        // keep yaw in 0 - 360 range
        float targetYaw = clampRotation(targetCamera.m_yaw);
        const float currentYaw = clampRotation(currentCamera.m_yaw);

        // return the sign of the float input (-1, 0, 1)
        const auto sign = [](const float value)
        {
            return aznumeric_cast<float>((0.0f < value) - (value < 0.0f));
        };

        // ensure smooth transition when moving across 0 - 360 boundary
        if (const float yawDelta = targetYaw - currentYaw; AZStd::abs(yawDelta) >= AZ::Constants::Pi)
        {
            targetYaw -= AZ::Constants::TwoPi * sign(yawDelta);
        }

        Camera camera;
        if (cameraProps.m_rotateSmoothingEnabledFn())
        {
            const float lookTime = SmoothValueTime(cameraProps.m_rotateSmoothnessFn(), deltaTime);
            camera.m_pitch = SmoothValue(targetCamera.m_pitch, currentCamera.m_pitch, lookTime);
            camera.m_yaw = SmoothValue(targetYaw, currentYaw, lookTime);
        }
        else
        {
            camera.m_pitch = targetCamera.m_pitch;
            camera.m_yaw = targetYaw;
        }

        if (cameraProps.m_translateSmoothingEnabledFn())
        {
            const float moveTime = SmoothValueTime(cameraProps.m_translateSmoothnessFn(), deltaTime);
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
        const InputState& state, [[maybe_unused]] const ScreenVector& cursorDelta, [[maybe_unused]] float scrollDelta)
    {
        if (const auto* input = AZStd::get_if<DiscreteInputEvent>(&state.m_inputEvent))
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
        const auto pivot = m_pivotFn();

        if (!pivot.has_value())
        {
            EndActivation();
            return targetCamera;
        }

        if (Beginning())
        {
            // as the camera starts, record the camera we would like to end up as
            m_nextCamera.m_offset = m_offsetFn(pivot.value().GetDistance(targetCamera.Translation()));
            const auto angles =
                EulerAngles(AZ::Matrix3x3::CreateFromMatrix3x4(AZ::Matrix3x4::CreateLookAt(targetCamera.Translation(), pivot.value())));
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

    bool CustomCameraInput::HandleEvents(const InputState& state, const ScreenVector& cursorDelta, const float scrollDelta)
    {
        return m_handleEventsFn(*this, state, cursorDelta, scrollDelta);
    }

    Camera CustomCameraInput::StepCamera(
        const Camera& targetCamera, const ScreenVector& cursorDelta, const float scrollDelta, const float deltaTime)
    {
        return m_stepCameraFn(*this, targetCamera, cursorDelta, scrollDelta, deltaTime);
    }

    InputState BuildInputEvent(
        const InputChannel& inputChannel, const AzFramework::ModifierKeyStates& modifiers, const WindowSize& windowSize)
    {
        const auto& inputChannelId = inputChannel.GetInputChannelId();
        const auto& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();

        const bool wasMouseButton = AZStd::any_of(
            InputDeviceMouse::Button::All.begin(),
            InputDeviceMouse::Button::All.end(),
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
                return InputState{ CursorEvent{ ScreenPoint(aznumeric_cast<int>(AZStd::lround(x)), aznumeric_cast<int>(AZStd::lround(y))),
                                                currentCursorState == AzFramework::SystemCursorState::ConstrainedAndHidden },
                                   modifiers };
            }
            else if (inputChannelId == InputDeviceMouse::Movement::X)
            {
                const auto x = inputChannel.GetValue();
                return InputState{ HorizontalMotionEvent{ aznumeric_cast<int>(AZStd::lround(x)) }, modifiers };
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Y)
            {
                const auto y = inputChannel.GetValue();
                return InputState{ VerticalMotionEvent{ aznumeric_cast<int>(AZStd::lround(y)) }, modifiers };
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Z)
            {
                return InputState{ ScrollEvent{ inputChannel.GetValue() }, modifiers };
            }
        }

        if (wasMouseButton || InputDeviceKeyboard::IsKeyboardDevice(inputDeviceId))
        {
            return InputState{ DiscreteInputEvent{ inputChannelId, inputChannel.GetState() }, modifiers };
        }

        return { AZStd::monostate{}, ModifierKeyStates{} };
    }
} // namespace AzFramework
