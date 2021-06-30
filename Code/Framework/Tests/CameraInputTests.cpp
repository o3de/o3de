/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

            auto firstPersonRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Right);
            auto firstPersonTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::LookTranslation);

            auto orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>();
            auto orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Left);
            auto orbitTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::OrbitTranslation);

            orbitCamera->m_orbitCameras.AddCamera(orbitRotateCamera);
            orbitCamera->m_orbitCameras.AddCamera(orbitTranslateCamera);

            m_cameraSystem->m_cameras.AddCamera(firstPersonRotateCamera);
            m_cameraSystem->m_cameras.AddCamera(firstPersonTranslateCamera);
            m_cameraSystem->m_cameras.AddCamera(orbitCamera);
        }

        void TearDown() override
        {
            m_cameraSystem->m_cameras.Clear();
            m_cameraSystem.reset();

            AllocatorsTestFixture::TearDown();
        }
    };

    TEST_F(CameraInputFixture, BeginEndOrbitCameraConsumesCorrectEvents)
    {
        // begin orbit camera
        const bool consumed1 = HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{AzFramework::InputDeviceKeyboard::Key::ModifierAltL, AzFramework::InputChannel::State::Began});
        // begin listening for orbit rotate (click detector) - event is not consumed
        const bool consumed2 = HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Began});
        // begin orbit rotate (mouse has moved sufficient distance to initiate)
        const bool consumed3 = HandleEventAndUpdate(AzFramework::HorizontalMotionEvent{5});
        // end orbit (mouse up) - event is not consumed
        const bool consumed4 = HandleEventAndUpdate(
            AzFramework::DiscreteInputEvent{AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Ended});

        const auto allConsumed = AZStd::vector<bool>{consumed1, consumed2, consumed3, consumed4};

        using ::testing::ElementsAre;
        EXPECT_THAT(allConsumed, ElementsAre(true, false, true, false));
    }
} // namespace UnitTest
