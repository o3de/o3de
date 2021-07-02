/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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

            AzFramework::ReloadCameraKeyBindings();

            m_cameraSystem = AZStd::make_shared<AzFramework::CameraSystem>();

            m_firstPersonRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Right);
            m_firstPersonTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::LookTranslation);

            auto orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>();
            auto orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Left);
            auto orbitTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::OrbitTranslation);

            orbitCamera->m_orbitCameras.AddCamera(orbitRotateCamera);
            orbitCamera->m_orbitCameras.AddCamera(orbitTranslateCamera);

            m_cameraSystem->m_cameras.AddCamera(m_firstPersonRotateCamera);
            m_cameraSystem->m_cameras.AddCamera(m_firstPersonTranslateCamera);
            m_cameraSystem->m_cameras.AddCamera(orbitCamera);
        }

        void TearDown() override
        {
            m_firstPersonRotateCamera.reset();
            m_firstPersonTranslateCamera.reset();

            m_cameraSystem->m_cameras.Clear();
            m_cameraSystem.reset();

            AllocatorsTestFixture::TearDown();
        }

        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_firstPersonRotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_firstPersonTranslateCamera;
    };

    TEST_F(CameraInputFixture, BeginEndOrbitCameraConsumesCorrectEvents)
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

    TEST_F(CameraInputFixture, BeginCameraInputNotifiesActivationBeganCallbackForTranslateCamera)
    {
        bool activationBegan = false;
        m_firstPersonTranslateCamera->SetActivationBeganFn(
            [&activationBegan]
            {
                activationBegan = true;
            });

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceKeyboard::Key::AlphanumericW,
                                                              AzFramework::InputChannel::State::Began });

        EXPECT_TRUE(activationBegan);
    }

    TEST_F(CameraInputFixture, BeginCameraInputNotifiesActivationBeganCallbackAfterDeltaForRotateCamera)
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

    TEST_F(CameraInputFixture, BeginCameraInputDoesNotNotifyActivationBeganCallbackWithNoDeltaForRotateCamera)
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

    TEST_F(CameraInputFixture, EndCameraInputNotifiesActivationEndCallbackAfterDeltaForRotateCamera)
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

    TEST_F(CameraInputFixture, EndCameraInputDoesNotNotifyActivationBeganOrEndCallbackWithNoDeltaForRotateCamera)
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

    TEST_F(CameraInputFixture, EndCameraInputNotifiesActivationBeganOrEndCallbackWithTranslateCamera)
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

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceKeyboard::Key::AlphanumericW,
                                                              AzFramework::InputChannel::State::Began });
        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceKeyboard::Key::AlphanumericW,
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

        HandleEventAndUpdate(AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceKeyboard::Key::AlphanumericW,
                                                              AzFramework::InputChannel::State::Began });

        m_cameraSystem->m_cameras.Clear();

        EXPECT_TRUE(activationEnded);
    }
} // namespace UnitTest
