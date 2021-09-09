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
            m_camera = m_cameraSystem->StepCamera(m_targetCamera, deltaTime);
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
            m_firstPersonTranslateCamera =
                AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::LookTranslation, m_translateCameraInputChannelIds);

            m_orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>(m_orbitChannelId);
            auto orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Left);
            auto orbitTranslateCamera =
                AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::OrbitTranslation, m_translateCameraInputChannelIds);

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
        m_orbitCamera->SetLookAtFn(
            [](const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& direction)
            {
                return position;
            });

        AzFramework::UpdateCameraFromTransform(
            m_targetCamera,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(0.0f, 0.0f, 90.0f)), AZ::Vector3(10.0f, 10.0f, 10.0f)));

        m_camera = m_targetCamera;

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began });

        // verify the camera yaw has not changed and the look at point
        // does not match that of the camera translation
        using ::testing::Eq;
        using ::testing::Not;
        EXPECT_THAT(m_camera.m_yaw, Eq(AZ::DegToRad(90.0f)));
        EXPECT_THAT(m_camera.m_lookAt, Not(IsClose(m_camera.Translation())));
    }
} // namespace UnitTest
