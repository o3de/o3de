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

#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Input/Channels/InputChannel.h>
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
        float m_yaw{0.0};
        float m_pitch{0.0};
        float m_lookDist{0.0}; //!< Zero gives first person free look, otherwise orbit about m_lookAt

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

    struct CursorMotionEvent
    {
        ScreenPoint m_position;
    };

    struct ScrollEvent
    {
        float m_delta;
    };

    struct DiscreteInputEvent
    {
        InputChannelId m_channelId; //!< Channel type. (e.g. Keyboard key, mouse button or other device input).
        InputChannel::State m_state; //!< Channel state. (e.g. Begin/update/end event).
    };

    using InputEvent = AZStd::variant<AZStd::monostate, CursorMotionEvent, ScrollEvent, DiscreteInputEvent>;

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

        virtual void HandleEvents(const InputEvent& event) = 0;
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
        void AddCamera(AZStd::shared_ptr<CameraInput> cameraInput);
        bool HandleEvents(const InputEvent& event);
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime);
        void Reset();

    private:
        AZStd::vector<AZStd::shared_ptr<CameraInput>> m_activeCameraInputs;
        AZStd::vector<AZStd::shared_ptr<CameraInput>> m_idleCameraInputs;
    };

    class CameraSystem
    {
    public:
        bool HandleEvents(const InputEvent& event);
        Camera StepCamera(const Camera& targetCamera, float deltaTime);

        Cameras m_cameras;

    private:
        float m_scrollDelta = 0.0f;
        AZStd::optional<ScreenPoint> m_lastCursorPosition;
        AZStd::optional<ScreenPoint> m_currentCursorPosition;
    };

    class RotateCameraInput : public CameraInput
    {
    public:
        explicit RotateCameraInput(const InputChannelId rotateChannelId)
            : m_rotateChannelId(rotateChannelId)
        {
        }

        void HandleEvents(const InputEvent& event) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

    private:
        InputChannelId m_rotateChannelId;
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
        return {orientation.GetBasisX(), orientation.GetBasisZ()};
    }

    inline PanAxes OrbitPan(const Camera& camera)
    {
        const AZ::Matrix3x3 orientation = camera.Rotation();

        const auto basisX = orientation.GetBasisX();
        const auto basisY = [&orientation] {
            const auto forward = orientation.GetBasisY();
            return AZ::Vector3(forward.GetX(), forward.GetY(), 0.0f).GetNormalized();
        }();

        return {basisX, basisY};
    }

    class PanCameraInput : public CameraInput
    {
    public:
        PanCameraInput(const InputChannelId panChannelId, PanAxesFn panAxesFn)
            : m_panAxesFn(AZStd::move(panAxesFn))
            , m_panChannelId(panChannelId)
        {
        }
        void HandleEvents(const InputEvent& event) override;
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
        const auto basisY = [&orientation] {
            const auto forward = orientation.GetBasisY();
            return AZ::Vector3(forward.GetX(), forward.GetY(), 0.0f).GetNormalized();
        }();
        const auto basisZ = AZ::Vector3::CreateAxisZ();

        return AZ::Matrix3x3::CreateFromColumns(basisX, basisY, basisZ);
    }

    class TranslateCameraInput : public CameraInput
    {
    public:
        explicit TranslateCameraInput(TranslationAxesFn translationAxesFn)
            : m_translationAxesFn(AZStd::move(translationAxesFn))
        {
        }
        void HandleEvents(const InputEvent& event) override;
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
        void HandleEvents(const InputEvent& event) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
    };

    class OrbitDollyCursorMoveCameraInput : public CameraInput
    {
    public:
        explicit OrbitDollyCursorMoveCameraInput(const InputChannelId dollyChannelId)
            : m_dollyChannelId(dollyChannelId) {}

        void HandleEvents(const InputEvent& event) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;

    private:
        InputChannelId m_dollyChannelId;
    };

    class ScrollTranslationCameraInput : public CameraInput
    {
    public:
        void HandleEvents(const InputEvent& event) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
    };

    class OrbitCameraInput : public CameraInput
    {
    public:
        void HandleEvents(const InputEvent& event) override;
        Camera StepCamera(const Camera& targetCamera, const ScreenVector& cursorDelta, float scrollDelta, float deltaTime) override;
        bool Exclusive() const override
        {
            return true;
        }

        Cameras m_orbitCameras;
    };

    struct WindowSize;

    //! Map from a generic InputChannel event to a camera specific InputEvent.
    InputEvent BuildInputEvent(const InputChannel& inputChannel, const WindowSize& windowSize);
} // namespace AzFramework
