/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Viewport/CameraInput.h>

namespace UnitTest
{
    class CameraInputFixture : public AllocatorsTestFixture
    {
    public:
        AzFramework::Camera m_camera;
        AzFramework::Camera m_targetCamera;
        AZStd::shared_ptr<AzFramework::CameraSystem> m_cameraSystem;

        bool HandleEventAndUpdate(const AzFramework::InputEvent& event)
        {
            constexpr float deltaTime = 0.01666f; // 60fps
            const bool consumed = m_cameraSystem->HandleEvents(event);
            m_targetCamera = m_cameraSystem->StepCamera(m_targetCamera, deltaTime);
            m_camera = m_targetCamera; // no smoothing
            return consumed;
        }

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_cameraSystem = AZStd::make_shared<AzFramework::CameraSystem>();

            m_translateCameraInputChannelIds.m_leftChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_A");
            m_translateCameraInputChannelIds.m_rightChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_D");
            m_translateCameraInputChannelIds.m_forwardChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_W");
            m_translateCameraInputChannelIds.m_backwardChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_S");
            m_translateCameraInputChannelIds.m_upChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_E");
            m_translateCameraInputChannelIds.m_downChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_Q");
            m_translateCameraInputChannelIds.m_boostChannelId = AzFramework::InputChannelId("keyboard_key_modifier_shift_l");

            m_firstPersonRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Right);
            // set rotate speed to be a value that will scale motion delta (pixels moved) by a thousandth.
            m_firstPersonRotateCamera->m_rotateSpeedFn = []()
            {
                return 0.001f;
            };

            m_firstPersonTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
                m_translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslatePivotLook);

            m_orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>(m_orbitChannelId);
            m_orbitCamera->SetPivotFn(
                [this](const AZ::Vector3&, const AZ::Vector3&)
                {
                    return m_pivot;
                });

            auto orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Left);
            // set rotate speed to be a value that will scale motion delta (pixels moved) by a thousandth.
            orbitRotateCamera->m_rotateSpeedFn = []()
            {
                return 0.001f;
            };

            auto orbitTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
                m_translateCameraInputChannelIds, AzFramework::OrbitTranslation, AzFramework::TranslateOffsetOrbit);

            m_orbitCamera->m_orbitCameras.AddCamera(orbitRotateCamera);
            m_orbitCamera->m_orbitCameras.AddCamera(orbitTranslateCamera);

            m_cameraSystem->m_cameras.AddCamera(m_firstPersonRotateCamera);
            m_cameraSystem->m_cameras.AddCamera(m_firstPersonTranslateCamera);
            m_cameraSystem->m_cameras.AddCamera(m_orbitCamera);

            // these tests rely on using motion delta, not cursor positions (default is true)
            AzFramework::ed_cameraSystemUseCursor = false;
        }

        void TearDown() override
        {
            AzFramework::ed_cameraSystemUseCursor = true;

            m_orbitCamera.reset();
            m_firstPersonRotateCamera.reset();
            m_firstPersonTranslateCamera.reset();

            m_cameraSystem->m_cameras.Clear();
            m_cameraSystem.reset();

            AllocatorsTestFixture::TearDown();
        }

        AzFramework::InputChannelId m_orbitChannelId = AzFramework::InputChannelId("keyboard_key_modifier_alt_l");
        AzFramework::TranslateCameraInputChannelIds m_translateCameraInputChannelIds;
        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_firstPersonRotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_firstPersonTranslateCamera;
        AZStd::shared_ptr<AzFramework::OrbitCameraInput> m_orbitCamera;
        AZ::Vector3 m_pivot = AZ::Vector3::CreateZero();

        // this is approximately Pi/2 * 1000 - this can be used to rotate the camera 90 degrees (pitch or yaw based
        // on vertical or horizontal motion) as the rotate speed function is set to be 1/1000.
        inline static const int PixelMotionDelta90Degrees = 1570;
        inline static const int PixelMotionDelta135Degrees = 2356;
    };

    TEST_F(CameraInputFixture, BeginAndEndOrbitCameraInputConsumesCorrectEvents)
    {
        // begin orbit camera
        const bool consumed1 = HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceKeyboard::Key::ModifierAltL,
                                                                                     AzFramework::InputChannel::State::Began });
        // begin listening for orbit rotate (click detector) - event is not consumed
        const bool consumed2 = HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Began });
        // begin orbit rotate (mouse has moved sufficient distance to initiate)
        const bool consumed3 = HandleEventAndUpdate(AzFramework::HorizontalMotionEvent{ 5 });
        // end orbit (mouse up) - event is not consumed
        const bool consumed4 = HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Ended });

        const auto allConsumed = AZStd::vector<bool>{ consumed1, consumed2, consumed3, consumed4 };

        using ::testing::ElementsAre;
        EXPECT_THAT(allConsumed, ElementsAre(true, false, true, false));
    }

    TEST_F(CameraInputFixture, BeginCameraInputNotifiesActivationBeganFnForTranslateCameraInput)
    {
        bool activationBegan = false;
        m_firstPersonTranslateCamera->SetActivationBeganFn(
            [&activationBegan]
            {
                activationBegan = true;
            });

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId,
                                                              AzFramework::InputChannel::State::Began });

        EXPECT_TRUE(activationBegan);
    }

    TEST_F(CameraInputFixture, BeginCameraInputNotifiesActivationBeganFnAfterDeltaForRotateCameraInput)
    {
        bool activationBegan = false;
        m_firstPersonRotateCamera->SetActivationBeganFn(
            [&activationBegan]
            {
                activationBegan = true;
            });

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(AzFramework::HorizontalMotionEvent{ 20 }); // must move input device

        EXPECT_TRUE(activationBegan);
    }

    TEST_F(CameraInputFixture, BeginCameraInputDoesNotNotifyActivationBeganFnWithNoDeltaForRotateCameraInput)
    {
        bool activationBegan = false;
        m_firstPersonRotateCamera->SetActivationBeganFn(
            [&activationBegan]
            {
                activationBegan = true;
            });

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began });

        EXPECT_FALSE(activationBegan);
    }

    TEST_F(CameraInputFixture, EndCameraInputNotifiesActivationEndFnAfterDeltaForRotateCameraInput)
    {
        bool activationEnded = false;
        m_firstPersonRotateCamera->SetActivationEndedFn(
            [&activationEnded]
            {
                activationEnded = true;
            });

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(AzFramework::HorizontalMotionEvent{ 20 });
        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Ended });

        EXPECT_TRUE(activationEnded);
    }

    TEST_F(CameraInputFixture, EndCameraInputDoesNotNotifyActivationBeganFnOrActivationBeganFnWithNoDeltaForRotateCameraInput)
    {
        bool activationBegan = false;
        m_firstPersonRotateCamera->SetActivationBeganFn(
            [&activationBegan]
            {
                activationBegan = true;
            });

        bool activationEnded = false;
        m_firstPersonRotateCamera->SetActivationEndedFn(
            [&activationEnded]
            {
                activationEnded = true;
            });

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Ended });

        EXPECT_FALSE(activationBegan);
        EXPECT_FALSE(activationEnded);
    }

    TEST_F(CameraInputFixture, End_CameraInputNotifiesActivationBeganFnOrActivationEndFnWithTranslateCamera)
    {
        bool activationBegan = false;
        m_firstPersonTranslateCamera->SetActivationBeganFn(
            [&activationBegan]
            {
                activationBegan = true;
            });

        bool activationEnded = false;
        m_firstPersonTranslateCamera->SetActivationEndedFn(
            [&activationEnded]
            {
                activationEnded = true;
            });

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId,
                                                              AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId,
                                                              AzFramework::InputChannel::State::Ended });

        EXPECT_TRUE(activationBegan);
        EXPECT_TRUE(activationEnded);
    }

    TEST_F(CameraInputFixture, EndActivationCalledForCameraInputIfActiveWhenCamerasAreCleared)
    {
        bool activationEnded = false;
        m_firstPersonTranslateCamera->SetActivationEndedFn(
            [&activationEnded]
            {
                activationEnded = true;
            });

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId,
                                                              AzFramework::InputChannel::State::Began });

        m_cameraSystem->m_cameras.Clear();

        EXPECT_TRUE(activationEnded);
    }

    TEST_F(CameraInputFixture, OrbitCameraInputHandlesLookAtPointAndSelfAtSamePositionWhenOrbiting)
    {
        // create pathological lookAtFn that just returns the same position as the camera
        m_orbitCamera->SetPivotFn(
            [](const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& direction)
            {
                return position;
            });

        const auto expectedCameraPosition = AZ::Vector3(10.0f, 10.0f, 10.0f);
        AzFramework::UpdateCameraFromTransform(
            m_targetCamera,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(0.0f, 0.0f, 90.0f)), expectedCameraPosition));

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began });

        // verify the camera yaw has not changed and pivot point matches the expected camera position
        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_yaw, FloatNear(AZ::DegToRad(90.0f), 0.001f));
        EXPECT_THAT(m_camera.m_pitch, FloatNear(0.0f, 0.001f));
        EXPECT_THAT(m_camera.m_offset, IsClose(AZ::Vector3::CreateZero()));
        EXPECT_THAT(m_camera.m_pivot, IsClose(expectedCameraPosition));
    }

    TEST_F(CameraInputFixture, FirstPersonRotateCameraInputRotatesYawByNinetyDegreesWithRequiredPixelDelta)
    {
        const auto cameraStartingPosition = AZ::Vector3::CreateAxisY(-10.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(AzFramework::HorizontalMotionEvent{ PixelMotionDelta90Degrees });

        const float expectedYaw = AzFramework::WrapYawRotation(-AZ::Constants::HalfPi);

        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_yaw, FloatNear(expectedYaw, 0.001f));
        EXPECT_THAT(m_camera.m_pitch, FloatNear(0.0f, 0.001f));
        EXPECT_THAT(m_camera.m_pivot, IsClose(cameraStartingPosition));
        EXPECT_THAT(m_camera.m_offset, IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(CameraInputFixture, FirstPersonRotateCameraInputRotatesPitchByNinetyDegreesWithRequiredPixelDelta)
    {
        const auto cameraStartingPosition = AZ::Vector3::CreateAxisY(-10.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(AzFramework::VerticalMotionEvent{ PixelMotionDelta90Degrees });

        const float expectedPitch = AzFramework::ClampPitchRotation(-AZ::Constants::HalfPi);

        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_yaw, FloatNear(0.0f, 0.001f));
        EXPECT_THAT(m_camera.m_pitch, FloatNear(expectedPitch, 0.001f));
        EXPECT_THAT(m_camera.m_pivot, IsClose(cameraStartingPosition));
        EXPECT_THAT(m_camera.m_offset, IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(CameraInputFixture, OrbitRotateCameraInputRotatesPitchOffsetByNinetyDegreesWithRequiredPixelDelta)
    {
        const auto cameraStartingPosition = AZ::Vector3::CreateAxisY(-20.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;

        m_pivot = AZ::Vector3::CreateAxisY(-10.0f);

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(AzFramework::VerticalMotionEvent{ PixelMotionDelta90Degrees });

        const auto expectedCameraEndingPosition = AZ::Vector3(0.0f, -10.0f, 10.0f);
        const float expectedPitch = AzFramework::ClampPitchRotation(-AZ::Constants::HalfPi);

        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_yaw, FloatNear(0.0f, 0.001f));
        EXPECT_THAT(m_camera.m_pitch, FloatNear(expectedPitch, 0.001f));
        EXPECT_THAT(m_camera.m_pivot, IsClose(m_pivot));
        EXPECT_THAT(m_camera.m_offset, IsClose(AZ::Vector3::CreateAxisY(-10.0f)));
        EXPECT_THAT(m_camera.Translation(), IsCloseTolerance(expectedCameraEndingPosition, 0.01f));
    }

    TEST_F(CameraInputFixture, OrbitRotateCameraInputRotatesYawOffsetByNinetyDegreesWithRequiredPixelDelta)
    {
        const auto cameraStartingPosition = AZ::Vector3(15.0f, -20.0f, 0.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;

        m_pivot = AZ::Vector3(10.0f, -10.0f, 0.0f);

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(AzFramework::HorizontalMotionEvent{ -PixelMotionDelta90Degrees });

        const auto expectedCameraEndingPosition = AZ::Vector3(20.0f, -5.0f, 0.0f);
        const float expectedYaw = AzFramework::WrapYawRotation(AZ::Constants::HalfPi);

        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_yaw, FloatNear(expectedYaw, 0.001f));
        EXPECT_THAT(m_camera.m_pitch, FloatNear(0.0f, 0.001f));
        EXPECT_THAT(m_camera.m_pivot, IsClose(m_pivot));
        EXPECT_THAT(m_camera.m_offset, IsClose(AZ::Vector3(5.0f, -10.0f, 0.0f)));
        EXPECT_THAT(m_camera.Translation(), IsCloseTolerance(expectedCameraEndingPosition, 0.01f));
    }

    TEST_F(CameraInputFixture, CameraPitchCanNotBeMovedPastNinetyDegreesWhenConstrained)
    {
        const auto cameraStartingPosition = AZ::Vector3(15.0f, -20.0f, 0.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began });
        // pitch by 135.0 degrees
        HandleEventAndUpdate(AzFramework::VerticalMotionEvent{ -PixelMotionDelta135Degrees });

        // clamped to 90.0 degrees
        const float expectedPitch = AZ::DegToRad(90.0f);

        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_pitch, FloatNear(expectedPitch, 0.001f));
    }

    TEST_F(CameraInputFixture, CameraPitchCanBeMovedPastNinetyDegreesWhenUnconstrained)
    {
        m_firstPersonRotateCamera->m_constrainPitch = []
        {
            return false;
        };

        const auto cameraStartingPosition = AZ::Vector3(15.0f, -20.0f, 0.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began });
        // pitch by 135.0 degrees
        HandleEventAndUpdate(AzFramework::VerticalMotionEvent{ -PixelMotionDelta135Degrees });

        const float expectedPitch = AZ::DegToRad(135.0f);

        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_pitch, FloatNear(expectedPitch, 0.001f));
    }
} // namespace UnitTest
