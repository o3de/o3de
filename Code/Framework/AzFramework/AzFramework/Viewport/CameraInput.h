/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Viewport/ClickDetector.h>
#include <AzFramework/Viewport/CursorState.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Viewport/ViewportId.h>

namespace AzFramework
{
    AZ_CVAR_EXTERNED(bool, ed_cameraSystemUseCursor);

    struct WindowSize;

    //! Returns Euler angles (pitch, roll, yaw) for the incoming orientation.
    //! @note Order of rotation is Z, Y, X.
    AZ::Vector3 EulerAngles(const AZ::Matrix3x3& orientation);

    //! A simple camera representation using spherical coordinates as input (pitch, yaw and look distance).
    //! The cameras transform and view can be obtained through accessor functions that use the internal
    //! spherical coordinates to calculate the position and orientation.
    struct Camera
    {
        AZ::Vector3 m_lookAt = AZ::Vector3::CreateZero(); //!< Position of camera when m_lookDist is zero,
                                                          //!< or position of m_lookAt when m_lookDist is greater
                                                          //!< than zero.
        float m_yaw{ 0.0 }; //!< Yaw rotation of camera (stored in radians) usually clamped to 0-360 degrees (0-2Pi radians).
        float m_pitch{ 0.0 }; //!< Pitch rotation of the camera (stored in radians) usually clamped to +/-90 degrees (-Pi/2 - Pi/2 radians).
        float m_lookDist{ 0.0 }; //!< Zero gives first person free look, otherwise orbit about m_lookAt

        //! View camera transform (V in model-view-projection matrix (MVP)).
        AZ::Transform View() const;
        //! World camera transform.
        AZ::Transform Transform() const;
        //! World rotation.
        AZ::Matrix3x3 Rotation() const;
        //! World translation.
        AZ::Vector3 Translation() const;
    };

    inline AZ::Transform Camera::View() const
    {
        return Transform().GetInverse();
    }

    inline AZ::Transform Camera::Transform() const
    {
        return AZ::Transform::CreateTranslation(m_lookAt) * AZ::Transform::CreateRotationZ(m_yaw) *
            AZ::Transform::CreateRotationX(m_pitch) * AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(m_lookDist));
    }

    inline AZ::Matrix3x3 Camera::Rotation() const
    {
        return AZ::Matrix3x3::CreateFromQuaternion(Transform().GetRotation());
    }

    inline AZ::Vector3 Camera::Translation() const
    {
        return Transform().GetTranslation();
    }

    //! Extracts Euler angles (orientation) and translation from the transform and writes the values to the camera.
    void UpdateCameraFromTransform(Camera& camera, const AZ::Transform& transform);

    //! Generic motion type.
    template<typename MotionTag>
    struct MotionEvent
    {
        int m_delta;
    };

    using HorizontalMotionEvent = MotionEvent<struct HorizontalMotionTag>;
    using VerticalMotionEvent = MotionEvent<struct VerticalMotionTag>;

    struct CursorEvent
    {
        ScreenPoint m_position;
        bool m_captured = false;
    };

    struct ScrollEvent
    {
        float m_delta;
    };

    //! Represents an input event which occurs as a discrete change in state (e.g. button down, button up) rather than a continuous stream.
    //! @note Such as a key press with a down/up event as opposed to a continuous delta.
    struct DiscreteInputEvent
    {
        InputChannelId m_channelId; //!< Channel type. (e.g. Keyboard key, mouse button or other device input).
        InputChannel::State m_state; //!< Channel state. (e.g. Begin/update/end event).
    };

    //! Represents a type-safe union of input events that are handled by the camera system.
    using InputEvent =
        AZStd::variant<AZStd::monostate, HorizontalMotionEvent, VerticalMotionEvent, CursorEvent, ScrollEvent, DiscreteInputEvent>;

    //! Base class for all camera behaviors.
    //! The core interface consists of:
    //!    HandleEvents, used to receive and process incoming input events to begin, update and end a behavior.
    //!    StepCamera, to update the current camera transform (position and orientation).
    class CameraInput
    {
    public:
        using ActivateChangeFn = AZStd::function<void()>;

        //! The state of activation the camera input is currently in.
        //! State changes of Activation: Idle -> Beginning -> Active -> Ending -> Idle
        enum class Activation
        {
            Idle, //!< Camera input is not currently active (initial state and transitioned to from Ending).
            Beginning, //!< Camera input is just beginning (transitioned to from Idle).
            Active, //!< Camera input is currently active and running (transitioned to from Beginning).
            Ending //!< Camera input is ending and will return to idle (transitioned to from Active).
        };

        virtual ~CameraInput() = default;

        bool Beginning() const
        {
            return m_activation == Activation::Beginning;
        }

        bool Ending() const
        {
            return m_activation == Activation::Ending;
        }

        bool Idle() const
        {
            return m_activation == Activation::Idle;
        }

        bool Active() const
        {
            return m_activation == Activation::Active;
        }

        void BeginActivation()
        {
            m_activation = Activation::Beginning;
        }

        void EndActivation()
        {
            m_activation = Activation::Ending;
        }

        void ContinueActivation()
        {
            // continue activation is called after the first step of the camera input,
            // activation began is called once, the first time before the state is set to active
            if (m_activation == Activation::Beginning)
            {
                if (m_activationBeganFn)
                {
                    m_activationBeganFn();
                }
            }

            m_activation = Activation::Active;
        }

        void ClearActivation()
        {
            // clear activation happens after an end has been requested, activation ended
            // is then called before the camera input is returned to idle
            if (m_activationEndedFn)
            {
                m_activationEndedFn();
            }

            m_activation = Activation::Idle;
        }

        void Reset()
        {
            ClearActivation();
            ResetImpl();
        }

        //! Respond to input events to transition a camera input to active, handle input while running, and restore to idle when input ends.
        virtual bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) = 0;
        //! Use processed input events to update the state of the camera.
        //! @note targetCamera is the current target camera at the beginning of an update. The returned camera is the targetCamera + some
        //! delta to get to the next camera position and/or orientation.
        virtual Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) = 0;

        //! It is usually possible for one to many camera inputs to be running at the same time (the default), it is however possible to
        //! to make a camera input 'exclusive', so only it can run at a time. This implies that all other behaviors must have stopped before
        //! it can begin, and no other camera input can run while it is active.
        virtual bool Exclusive() const
        {
            return false;
        }

        void SetActivationBeganFn(ActivateChangeFn activationBeganFn)
        {
            m_activationBeganFn = AZStd::move(activationBeganFn);
        }

        void SetActivationEndedFn(ActivateChangeFn activationEndedFn)
        {
            m_activationEndedFn = AZStd::move(activationEndedFn);
        }

    protected:
        //! Handle any state reset that may be required for the camera input (optional).
        virtual void ResetImpl()
        {
        }

    private:
        Activation m_activation = Activation::Idle; //!< Default all camera inputs to the idle state.
        AZStd::function<void()> m_activationBeganFn; //!< Called when the camera input successfully makes it to the active state.
        AZStd::function<void()> m_activationEndedFn; //!< Called when the camera input ends and returns to the idle state.
    };

    //! Properties to use to configure behavior across all types of camera.
    struct CameraProps
    {
        //! Rotate smoothing value (useful approx range 3-6, higher values give sharper feel).
        AZStd::function<float()> m_rotateSmoothnessFn;
        //! Translate smoothing value (useful approx range 3-6, higher values give sharper feel).
        AZStd::function<float()> m_translateSmoothnessFn;
        //! Enable/disable rotation smoothing.
        AZStd::function<bool()> m_rotateSmoothingEnabledFn;
        //! Enable/disable translation smoothing.
        AZStd::function<bool()> m_translateSmoothingEnabledFn;
    };

    //! An interpolation function to smoothly interpolate all camera properties from currentCamera to targetCamera.
    //! The camera returned will be some value between current and target camera.
    //! @note The rate of interpolation can be customized with CameraProps.
    Camera SmoothCamera(const Camera& currentCamera, const Camera& targetCamera, const CameraProps& cameraProps, float deltaTime);

    //! Manages a list of camera inputs.
    //! By default all camera inputs are added to the idle list, when a camera activates it will be added to the active list, when it
    //! deactivates it will be returned to the idle list.
    class Cameras
    {
    public:
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta);
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime);

        //! Add a camera input (behavior) to run in this set of camera inputs.
        //! The camera inputs added here will determine the overall behavior of the camera.
        void AddCamera(AZStd::shared_ptr<CameraInput> cameraInput);
        //! Reset the state of all cameras.
        void Reset();
        //! Remove all cameras that were added.
        void Clear();
        //! Is one of the cameras in the active camera inputs marked as 'exclusive'.
        //! @note This implies no other sibling cameras can begin while the exclusive camera is running.
        bool Exclusive() const;

    private:
        AZStd::vector<AZStd::shared_ptr<CameraInput>> m_activeCameraInputs; //!< Active camera inputs updating the camera (empty initially).
        AZStd::vector<AZStd::shared_ptr<CameraInput>>
            m_idleCameraInputs; //!< Idle camera inputs not contributing to the update (filled initially).
    };

    //! Responsible for updating a series of cameras given various inputs.
    class CameraSystem
    {
    public:
        bool HandleEvents(const InputEvent& event);
        Camera StepCamera(const Camera& targetCamera, float deltaTime);
        bool HandlingEvents() const
        {
            return m_handlingEvents;
        }

        Cameras m_cameras; //!< Represents a collection of camera inputs that together provide a camera controller.

    private:
        ScreenVector m_motionDelta; //!< The delta used for look/orbit/pan (rotation + translation) - two dimensional.
        CursorState m_cursorState; //!< The current and previous position of the cursor (used to calculate movement delta).
        float m_scrollDelta = 0.0f; //!< The delta used for dolly/movement (translation) - one dimensional.
        bool m_handlingEvents = false; //!< Is the camera system currently handling events (events are consumed and not propagated).
    };

    //! A camera input to handle motion deltas that can rotate or orbit the camera.
    class RotateCameraInput : public CameraInput
    {
    public:
        explicit RotateCameraInput(const InputChannelId& rotateChannelId);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

        AZStd::function<float()> m_rotateSpeedFn;
        AZStd::function<bool()> m_invertPitchFn;
        AZStd::function<bool()> m_invertYawFn;

    private:
        InputChannelId m_rotateChannelId; //!< Input channel to begin the rotate camera input.
        ClickDetector m_clickDetector; //!< Used to determine when a sufficient motion delta has occurred after an initial discrete input
                                       //!< event has started (press and move event).
    };

    //! Axes to use while panning the camera.
    struct PanAxes
    {
        AZ::Vector3 m_horizontalAxis;
        AZ::Vector3 m_verticalAxis;
    };

    //! PanAxes build function that will return a pair of pan axes depending on the camera orientation.
    using PanAxesFn = AZStd::function<PanAxes(const Camera& camera)>;

    //! PanAxes to use while in 'look' camera behavior (free look).
    inline PanAxes LookPan(const Camera& camera)
    {
        const AZ::Matrix3x3 orientation = camera.Rotation();
        return { orientation.GetBasisX(), orientation.GetBasisZ() };
    }

    //! PanAxes to use while in 'orbit' camera behavior.
    inline PanAxes OrbitPan(const Camera& camera)
    {
        const AZ::Matrix3x3 orientation = camera.Rotation();

        const auto basisX = orientation.GetBasisX();
        const auto basisY = [&orientation]
        {
            const auto forward = orientation.GetBasisY();
            return AZ::Vector3(forward.GetX(), forward.GetY(), 0.0f).GetNormalized();
        }();

        return { basisX, basisY };
    }

    //! A camera input to handle motion deltas that can pan the camera (translate in two axes).
    class PanCameraInput : public CameraInput
    {
    public:
        PanCameraInput(const InputChannelId& panChannelId, PanAxesFn panAxesFn);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

        AZStd::function<float()> m_panSpeedFn;
        AZStd::function<bool()> m_invertPanXFn;
        AZStd::function<bool()> m_invertPanYFn;

    private:
        PanAxesFn m_panAxesFn; //!< Builder for the particular pan axes (provided in the constructor).
        InputChannelId m_panChannelId; //!< Input channel to begin the pan camera input.
        ClickDetector m_clickDetector; //!< Used to determine when a sufficient motion delta has occurred after an initial discrete input
                                       //!< event has started (press and move event).
    };

    //! Axes to use while translating the camera.
    using TranslationAxesFn = AZStd::function<AZ::Matrix3x3(const Camera& camera)>;

    //! TranslationAxes to use while in 'look' camera behavior (free look).
    inline AZ::Matrix3x3 LookTranslation(const Camera& camera)
    {
        const AZ::Matrix3x3 orientation = camera.Rotation();

        const auto basisX = orientation.GetBasisX();
        const auto basisY = orientation.GetBasisY();
        const auto basisZ = AZ::Vector3::CreateAxisZ();

        return AZ::Matrix3x3::CreateFromColumns(basisX, basisY, basisZ);
    }

    //! TranslationAxes to use while in 'orbit' camera behavior.
    inline AZ::Matrix3x3 OrbitTranslation(const Camera& camera)
    {
        const AZ::Matrix3x3 orientation = camera.Rotation();

        const auto basisX = orientation.GetBasisX();
        const auto basisY = [&orientation]
        {
            const auto forward = orientation.GetBasisY();
            return AZ::Vector3(forward.GetX(), forward.GetY(), 0.0f).GetNormalized();
        }();
        const auto basisZ = AZ::Vector3::CreateAxisZ();

        return AZ::Matrix3x3::CreateFromColumns(basisX, basisY, basisZ);
    }

    //! Groups all camera translation inputs.
    struct TranslateCameraInputChannels
    {
        InputChannelId m_forwardChannelId;
        InputChannelId m_backwardChannelId;
        InputChannelId m_leftChannelId;
        InputChannelId m_rightChannelId;
        InputChannelId m_downChannelId;
        InputChannelId m_upChannelId;
        InputChannelId m_boostChannelId;
    };

    //! A camera input to handle discrete events that can translate the camera (translate in three axes).
    class TranslateCameraInput : public CameraInput
    {
    public:
        explicit TranslateCameraInput(
            TranslationAxesFn translationAxesFn, const TranslateCameraInputChannels& translateCameraInputChannels);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
        void ResetImpl() override;

        AZStd::function<float()> m_translateSpeedFn;
        AZStd::function<float()> m_boostMultiplierFn;

    private:
        //! The type of translation the camera input is performing (multiple may be active at once).
        enum class TranslationType
        {
            // clang-format off
            Nil      = 0,
            Forward  = 1 << 0,
            Backward = 1 << 1,
            Left     = 1 << 2,
            Right    = 1 << 3,
            Up       = 1 << 4,
            Down     = 1 << 5,
            // clang-format on
        };

        friend TranslationType operator|(const TranslationType lhs, const TranslationType rhs)
        {
            return static_cast<TranslationType>(
                static_cast<std::underlying_type_t<TranslationType>>(lhs) | static_cast<std::underlying_type_t<TranslationType>>(rhs));
        }

        friend TranslationType& operator|=(TranslationType& lhs, const TranslationType rhs)
        {
            lhs = lhs | rhs;
            return lhs;
        }

        friend TranslationType operator^(const TranslationType lhs, const TranslationType rhs)
        {
            return static_cast<TranslationType>(
                static_cast<std::underlying_type_t<TranslationType>>(lhs) ^ static_cast<std::underlying_type_t<TranslationType>>(rhs));
        }

        friend TranslationType& operator^=(TranslationType& lhs, const TranslationType rhs)
        {
            lhs = lhs ^ rhs;
            return lhs;
        }

        friend TranslationType operator&(const TranslationType lhs, const TranslationType rhs)
        {
            return static_cast<TranslationType>(
                static_cast<std::underlying_type_t<TranslationType>>(lhs) & static_cast<std::underlying_type_t<TranslationType>>(rhs));
        }

        friend TranslationType& operator&=(TranslationType& lhs, const TranslationType rhs)
        {
            lhs = lhs & rhs;
            return lhs;
        }

        friend TranslationType operator~(const TranslationType lhs)
        {
            return static_cast<TranslationType>(~static_cast<std::underlying_type_t<TranslationType>>(lhs));
        }

        //! Converts from a generic input channel id to a concrete translation type (based on the user's key mappings).
        TranslationType TranslationFromKey(
            const InputChannelId& channelId, const TranslateCameraInputChannels& translateCameraInputChannels);

        TranslationType m_translation = TranslationType::Nil; //!< Types of translation the camera input is under.
        TranslationAxesFn m_translationAxesFn; //!< Builder for translation axes.
        TranslateCameraInputChannels m_translateCameraInputChannels; //!< Input channel ids that map to internal translation types.
        bool m_boost = false; //!< Is the translation speed currently being multiplied/scaled upwards.
    };

    //! A camera input to handle discrete scroll events that can modify the camera look distance.
    class OrbitDollyScrollCameraInput : public CameraInput
    {
    public:
        OrbitDollyScrollCameraInput();

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

        AZStd::function<float()> m_scrollSpeedFn;
    };

    //! A camera input to handle motion deltas that can modify the camera look distance.
    class OrbitDollyCursorMoveCameraInput : public CameraInput
    {
    public:
        explicit OrbitDollyCursorMoveCameraInput(const InputChannelId& dollyChannelId);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

        AZStd::function<float()> m_cursorSpeedFn;

    private:
        InputChannelId m_dollyChannelId; //!< Input channel to begin the dolly cursor camera input.
        ClickDetector m_clickDetector; //!< Used to determine when a sufficient motion delta has occurred after an initial discrete input
                                       //!< event has started (press and move event).
    };

    //! A camera input to handle discrete scroll events that can scroll (translate) the camera along its forward axis.
    class ScrollTranslationCameraInput : public CameraInput
    {
    public:
        ScrollTranslationCameraInput();

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

        AZStd::function<float()> m_scrollSpeedFn;
    };

    //! A camera input that doubles as its own set of camera inputs.
    //! It is 'exclusive', so does not overlap with other sibling camera inputs - it runs its own set of camera inputs as 'children'.
    class OrbitCameraInput : public CameraInput
    {
    public:
        using LookAtFn = AZStd::function<AZStd::optional<AZ::Vector3>(const AZ::Vector3& position, const AZ::Vector3& direction)>;

        explicit OrbitCameraInput(const InputChannelId& orbitChannelId);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
        bool Exclusive() const override;

        Cameras m_orbitCameras; //!< The camera inputs to run when this camera input is active (only these will run as it is exclusive).

        //! Override the default behavior for how a look-at point is calculated.
        void SetLookAtFn(const LookAtFn& lookAtFn);

    private:
        InputChannelId m_orbitChannelId; //!< Input channel to begin the orbit camera input.
        LookAtFn m_lookAtFn; //!< The look-at behavior to use for this orbit camera (how is the look-at point calculated/retrieved).
    };

    inline void OrbitCameraInput::SetLookAtFn(const LookAtFn& lookAtFn)
    {
        m_lookAtFn = lookAtFn;
    }

    inline bool OrbitCameraInput::Exclusive() const
    {
        return true;
    }

    //! Map from a generic InputChannel event to a camera specific InputEvent.
    InputEvent BuildInputEvent(const InputChannel& inputChannel, const WindowSize& windowSize);
} // namespace AzFramework
