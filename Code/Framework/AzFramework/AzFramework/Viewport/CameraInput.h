/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Viewport/ClickDetector.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Viewport/ViewportId.h>

namespace AzFramework
{
    //! Update camera key bindings that can be overridden with AZ console vars (invoke from console to update)
    void ReloadCameraKeyBindings();

    //! Return Euler angles (pitch, roll, yaw) for the incoming orientation.
    AZ::Vector3 EulerAngles(const AZ::Matrix3x3& orientation);

    struct Camera
    {
        AZ::Vector3 m_lookAt = AZ::Vector3::CreateZero(); //!< Position of camera when m_lookDist is zero,
                                                          //!< or position of m_lookAt when m_lookDist is greater
                                                          //!< than zero.
        float m_yaw{ 0.0 };
        float m_pitch{ 0.0 };
        float m_lookDist{ 0.0 }; //!< Zero gives first person free look, otherwise orbit about m_lookAt

        //! View camera transform (v in MVP).
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

    void UpdateCameraFromTransform(Camera& camera, const AZ::Transform& transform);

    //! Generic motion type
    template<typename MotionTag>
    struct MotionEvent
    {
        int m_delta;
    };

    using HorizontalMotionEvent = MotionEvent<struct HorizontalMotionTag>;
    using VerticalMotionEvent = MotionEvent<struct VerticalMotionTag>;

    struct ScrollEvent
    {
        float m_delta;
    };

    struct DiscreteInputEvent
    {
        InputChannelId m_channelId; //!< Channel type. (e.g. Keyboard key, mouse button or other device input).
        InputChannel::State m_state; //!< Channel state. (e.g. Begin/update/end event).
    };

    using InputEvent = AZStd::variant<AZStd::monostate, HorizontalMotionEvent, VerticalMotionEvent, ScrollEvent, DiscreteInputEvent>;

    class CameraInput
    {
    public:
        enum class Activation
        {
            Idle,
            Begin,
            Active,
            End
        };

        virtual ~CameraInput() = default;

        bool Beginning() const
        {
            return m_activation == Activation::Begin;
        }

        bool Ending() const
        {
            return m_activation == Activation::End;
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
            m_activation = Activation::Begin;
        }

        void EndActivation()
        {
            m_activation = Activation::End;
        }

        void ContinueActivation()
        {
            m_activation = Activation::Active;
        }

        void ClearActivation()
        {
            m_activation = Activation::Idle;
        }

        void Reset()
        {
            ClearActivation();
            ResetImpl();
        }

        virtual bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) = 0;
        virtual Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) = 0;

        virtual bool Exclusive() const
        {
            return false;
        }

    protected:
        virtual void ResetImpl()
        {
        }

    private:
        Activation m_activation = Activation::Idle;
    };

    Camera SmoothCamera(const Camera& currentCamera, const Camera& targetCamera, float deltaTime);

    class Cameras
    {
    public:
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta);
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime);

        void AddCamera(AZStd::shared_ptr<CameraInput> cameraInput);
        //! Reset the state of all cameras.
        void Reset();
        //! Remove all cameras that were added.
        void Clear();
        //! Is one of the cameras in the active camera inputs marked as 'exclusive'.
        //! @note This implies no other sibling cameras can begin while the exclusive camera is running.
        bool Exclusive() const;

    private:
        AZStd::vector<AZStd::shared_ptr<CameraInput>> m_activeCameraInputs;
        AZStd::vector<AZStd::shared_ptr<CameraInput>> m_idleCameraInputs;
    };

    inline bool Cameras::Exclusive() const
    {
        return AZStd::any_of(
            m_activeCameraInputs.begin(), m_activeCameraInputs.end(),
            [](const auto& cameraInput)
            {
                return cameraInput->Exclusive();
            });
    }

    //! Responsible for updating a series of cameras given various inputs.
    class CameraSystem
    {
    public:
        bool HandleEvents(const InputEvent& event);
        Camera StepCamera(const Camera& targetCamera, float deltaTime);

        Cameras m_cameras;

    private:
        ScreenVector m_motionDelta; //!< The delta used for look/orbit/pan (rotation + translation) - two dimensional.
        float m_scrollDelta = 0.0f; //!< The delta used for dolly/movement (translation) - one dimensional.
    };

    class RotateCameraInput : public CameraInput
    {
    public:
        explicit RotateCameraInput(InputChannelId rotateChannelId);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

    private:
        InputChannelId m_rotateChannelId;
        ClickDetector m_clickDetector;
    };

    struct PanAxes
    {
        AZ::Vector3 m_horizontalAxis;
        AZ::Vector3 m_verticalAxis;
    };

    using PanAxesFn = AZStd::function<PanAxes(const Camera& camera)>;

    inline PanAxes LookPan(const Camera& camera)
    {
        const AZ::Matrix3x3 orientation = camera.Rotation();
        return { orientation.GetBasisX(), orientation.GetBasisZ() };
    }

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

    class PanCameraInput : public CameraInput
    {
    public:
        PanCameraInput(InputChannelId panChannelId, PanAxesFn panAxesFn);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

    private:
        PanAxesFn m_panAxesFn;
        InputChannelId m_panChannelId;
    };

    using TranslationAxesFn = AZStd::function<AZ::Matrix3x3(const Camera& camera)>;

    inline AZ::Matrix3x3 LookTranslation(const Camera& camera)
    {
        const AZ::Matrix3x3 orientation = camera.Rotation();

        const auto basisX = orientation.GetBasisX();
        const auto basisY = orientation.GetBasisY();
        const auto basisZ = AZ::Vector3::CreateAxisZ();

        return AZ::Matrix3x3::CreateFromColumns(basisX, basisY, basisZ);
    }

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

    class TranslateCameraInput : public CameraInput
    {
    public:
        explicit TranslateCameraInput(TranslationAxesFn translationAxesFn);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
        void ResetImpl() override;

    private:
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

        static TranslationType translationFromKey(InputChannelId channelId);

        TranslationType m_translation = TranslationType::Nil;
        TranslationAxesFn m_translationAxesFn;
        bool m_boost = false;
    };

    class OrbitDollyScrollCameraInput : public CameraInput
    {
    public:
        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
    };

    class OrbitDollyCursorMoveCameraInput : public CameraInput
    {
    public:
        explicit OrbitDollyCursorMoveCameraInput(InputChannelId dollyChannelId);

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

    private:
        InputChannelId m_dollyChannelId;
    };

    class ScrollTranslationCameraInput : public CameraInput
    {
    public:
        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
    };

    class OrbitCameraInput : public CameraInput
    {
    public:
        using LookAtFn = AZStd::function<AZStd::optional<AZ::Vector3>(const AZ::Vector3& position, const AZ::Vector3& direction)>;

        // CameraInput overrides ...
        bool HandleEvents(const InputEvent& event, const ScreenVector& cursorDelta, float scrollDelta) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
        bool Exclusive() const override;

        Cameras m_orbitCameras;

        //! Override the default behavior for how a look-at point is calculated.
        void SetLookAtFn(const LookAtFn& lookAtFn);

    private:
        LookAtFn m_lookAtFn;
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
    InputEvent BuildInputEvent(const InputChannel& inputChannel);
} // namespace AzFramework
