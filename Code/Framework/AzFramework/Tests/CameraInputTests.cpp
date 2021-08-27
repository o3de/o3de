/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

            m_translateCameraInputChannels.m_leftChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_A");
            m_translateCameraInputChannels.m_rightChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_D");
            m_translateCameraInputChannels.m_forwardChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_W");
            m_translateCameraInputChannels.m_backwardChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_S");
            m_translateCameraInputChannels.m_upChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_E");
            m_translateCameraInputChannels.m_downChannelId = AzFramework::InputChannelId("keyboard_key_alphanumeric_Q");
            m_translateCameraInputChannels.m_boostChannelId = AzFramework::InputChannelId("keyboard_key_modifier_shift_l");

            m_firstPersonRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Right);
            m_firstPersonTranslateCamera =
                AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::LookTranslation, m_translateCameraInputChannels);

            auto orbitCamera =
                AZStd::make_shared<AzFramework::OrbitCameraInput>(AzFramework::InputChannelId("keyboard_key_modifier_alt_l"));
            auto orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Left);
            auto orbitTranslateCamera =
                AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::OrbitTranslation, m_translateCameraInputChannels);

            orbitCamera->m_orbitCameras.AddCamera(orbitRotateCamera);
            orbitCamera->m_orbitCameras.AddCamera(orbitTranslateCamera);

            m_cameraSystem->m_cameras.AddCamera(m_firstPersonRotateCamera);
            m_cameraSystem->m_cameras.AddCamera(m_firstPersonTranslateCamera);
            m_cameraSystem->m_cameras.AddCamera(orbitCamera);

            // these tests rely on using motion delta, not cursor positions (default is true)
            AzFramework::ed_cameraSystemUseCursor = false;
        }

        void TearDown() override
        {
            AzFramework::ed_cameraSystemUseCursor = true;

            m_firstPersonRotateCamera.reset();
            m_firstPersonTranslateCamera.reset();

            m_cameraSystem->m_cameras.Clear();
            m_cameraSystem.reset();

            AllocatorsTestFixture::TearDown();
        }

        AzFramework::TranslateCameraInputChannels m_translateCameraInputChannels;
        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_firstPersonRotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_firstPersonTranslateCamera;
    };

    TEST_F(CameraInputFixture, Begin_and_end_OrbitCameraInput_consumes_correct_events)
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

    TEST_F(CameraInputFixture, Begin_CameraInput_notifies_ActivationBeganFn_for_TranslateCameraInput)
    {
        bool activationBegan = false;
        m_firstPersonTranslateCamera->SetActivationBeganFn(
            [&activationBegan]
            {
                activationBegan = true;
            });

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannels.m_forwardChannelId, AzFramework::InputChannel::State::Began });

        EXPECT_TRUE(activationBegan);
    }

    TEST_F(CameraInputFixture, Begin_CameraInput_notifies_ActivationBeganFn_after_delta_for_RotateCameraInput)
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

    TEST_F(CameraInputFixture, Begin_CameraInput_does_not_notify_ActivationBeganFn_with_no_delta_for_RotateCameraInput)
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

    TEST_F(CameraInputFixture, End_CameraInput_notifies_ActivationEndFn_after_delta_for_RotateCameraInput)
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

    TEST_F(CameraInputFixture, End_CameraInput_does_not_notify_ActivationBeganFn_or_ActivationBeganFn_with_no_delta_for_RotateCameraInput)
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

    TEST_F(CameraInputFixture, End_CameraInput_notifies_ActivationBeganFn_or_ActivationEndFn_with_TranslateCamera)
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

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannels.m_forwardChannelId, AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannels.m_forwardChannelId, AzFramework::InputChannel::State::Ended });

        EXPECT_TRUE(activationBegan);
        EXPECT_TRUE(activationEnded);
    }

    TEST_F(CameraInputFixture, End_activation_called_for_CameraInput_if_active_when_cameras_are_cleared)
    {
        bool activationEnded = false;
        m_firstPersonTranslateCamera->SetActivationEndedFn(
            [&activationEnded]
            {
                activationEnded = true;
            });

        HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannels.m_forwardChannelId, AzFramework::InputChannel::State::Began });

        m_cameraSystem->m_cameras.Clear();

        EXPECT_TRUE(activationEnded);
    }
} // namespace UnitTest
