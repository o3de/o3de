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
    static AzFramework::ModifierKeyStates OrbitModifierKeyStates(const AzFramework::InputChannelId orbitChannelId, bool on = true)
    {
        AzFramework::ModifierKeyStates modifierKeyStates;
        modifierKeyStates.SetActive(AzFramework::GetCorrespondingModifierKeyMask(orbitChannelId), on);
        return modifierKeyStates;
    }

    static AzFramework::ModifierKeyStates BoostModifierKeyStates(const AzFramework::InputChannelId boostChannelId, bool on = true)
    {
        AzFramework::ModifierKeyStates modifierKeyStates;
        modifierKeyStates.SetActive(AzFramework::GetCorrespondingModifierKeyMask(boostChannelId), on);
        return modifierKeyStates;
    }

    class CameraInputFixture : public LeakDetectionFixture
    {
    public:
        AzFramework::Camera m_camera;
        AzFramework::Camera m_targetCamera;
        AZStd::shared_ptr<AzFramework::CameraSystem> m_cameraSystem;

        void Update()
        {
            constexpr float deltaTime = 0.01666f; // 60fps
            m_targetCamera = m_cameraSystem->StepCamera(m_targetCamera, deltaTime);
            m_camera = m_targetCamera; // no smoothing
        }

        bool HandleEvent(const AzFramework::InputState& state)
        {
            return m_cameraSystem->HandleEvents(state);
        }

        bool HandleEventAndUpdate(const AzFramework::InputState& state)
        {
            const bool consumed = HandleEvent(state);
            Update();
            return consumed;
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

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

            LeakDetectionFixture::TearDown();
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
        const AzFramework::ModifierKeyStates orbitModifierKeystate = OrbitModifierKeyStates(m_orbitChannelId);

        // begin orbit camera
        const bool consumed1 = HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceKeyboard::Key::ModifierAltL, AzFramework::InputChannel::State::Began },
            orbitModifierKeystate });
        // begin listening for orbit rotate (click detector) - event is not consumed
        const bool consumed2 = HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Began },
            orbitModifierKeystate });
        // begin orbit rotate (mouse has moved sufficient distance to initiate)
        const bool consumed3 =
            HandleEventAndUpdate(AzFramework::InputState{ AzFramework::HorizontalMotionEvent{ 5 }, orbitModifierKeystate });
        // end orbit (mouse up) - event is not consumed
        const bool consumed4 = HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Ended },
            orbitModifierKeystate });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });
        HandleEventAndUpdate(AzFramework::InputState{ AzFramework::HorizontalMotionEvent{ 20 },
                                                      AzFramework::ModifierKeyStates{} }); // must move input device

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });
        HandleEventAndUpdate(AzFramework::InputState{ AzFramework::HorizontalMotionEvent{ 20 }, AzFramework::ModifierKeyStates{} });
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Ended },
            AzFramework::ModifierKeyStates{} });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Ended },
            AzFramework::ModifierKeyStates{} });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Ended },
            AzFramework::ModifierKeyStates{} });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });

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

        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began },
                                     AzFramework::ModifierKeyStates{} });

        // verify the camera yaw has not changed and pivot point matches the expected camera
        // position
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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });
        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::HorizontalMotionEvent{ PixelMotionDelta90Degrees }, AzFramework::ModifierKeyStates{} });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });
        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::VerticalMotionEvent{ PixelMotionDelta90Degrees }, AzFramework::ModifierKeyStates{} });

        const float expectedPitch = AzFramework::ClampPitchRotation(-AZ::Constants::HalfPi);

        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_yaw, FloatNear(0.0f, 0.001f));
        EXPECT_THAT(m_camera.m_pitch, FloatNear(expectedPitch, 0.001f));
        EXPECT_THAT(m_camera.m_pivot, IsClose(cameraStartingPosition));
        EXPECT_THAT(m_camera.m_offset, IsClose(AZ::Vector3::CreateZero()));
    }

    TEST(CameraInput, CameraPitchIsClampedWithExpectedTolerance)
    {
        const auto [expectedMinPitch, expectedMaxPitch] = AzFramework::CameraPitchMinMaxRadiansWithTolerance();
        const float minPitch = AzFramework::ClampPitchRotation(-AZ::Constants::HalfPi);
        const float maxPitch = AzFramework::ClampPitchRotation(AZ::Constants::HalfPi);

        using ::testing::FloatNear;
        EXPECT_THAT(minPitch, FloatNear(expectedMinPitch, AzFramework::CameraPitchTolerance));
        EXPECT_THAT(maxPitch, FloatNear(expectedMaxPitch, AzFramework::CameraPitchTolerance));
    }

    TEST_F(CameraInputFixture, OrbitRotateCameraInputRotatesPitchOffsetByNinetyDegreesWithRequiredPixelDelta)
    {
        const AzFramework::ModifierKeyStates orbitModifierKeystate = OrbitModifierKeyStates(m_orbitChannelId);

        const auto cameraStartingPosition = AZ::Vector3::CreateAxisY(-20.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;

        m_pivot = AZ::Vector3::CreateAxisY(-10.0f);

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began }, orbitModifierKeystate });
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Began },
            orbitModifierKeystate });
        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::VerticalMotionEvent{ PixelMotionDelta90Degrees }, orbitModifierKeystate });

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
        const AzFramework::ModifierKeyStates orbitModifierKeystate = OrbitModifierKeyStates(m_orbitChannelId);

        const auto cameraStartingPosition = AZ::Vector3(15.0f, -20.0f, 0.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;

        m_pivot = AZ::Vector3(10.0f, -10.0f, 0.0f);

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began }, orbitModifierKeystate });
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Left, AzFramework::InputChannel::State::Began },
            orbitModifierKeystate });
        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::HorizontalMotionEvent{ -PixelMotionDelta90Degrees }, orbitModifierKeystate });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });
        // pitch by 135.0 degrees
        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::VerticalMotionEvent{ -PixelMotionDelta135Degrees }, AzFramework::ModifierKeyStates{} });

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

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ AzFramework::InputDeviceMouse::Button::Right, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });
        // pitch by 135.0 degrees
        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::VerticalMotionEvent{ -PixelMotionDelta135Degrees }, AzFramework::ModifierKeyStates{} });

        const float expectedPitch = AZ::DegToRad(135.0f);

        using ::testing::FloatNear;
        EXPECT_THAT(m_camera.m_pitch, FloatNear(expectedPitch, 0.001f));
    }

    TEST_F(CameraInputFixture, InvalidTranslationInputKeyCannotBeginTranslateCameraInputAgain)
    {
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });

        const bool consumed = m_cameraSystem->HandleEvents(
            AzFramework::InputState{ AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began },
                                     AzFramework::ModifierKeyStates{} });

        using ::testing::IsFalse;
        using ::testing::IsTrue;
        EXPECT_THAT(consumed, IsTrue());
        EXPECT_THAT(m_firstPersonTranslateCamera->Beginning(), IsFalse());
        EXPECT_THAT(m_firstPersonTranslateCamera->Active(), IsTrue());
    }

    TEST_F(CameraInputFixture, InvalidTranslationInputKeyDownCannotBeginTranslateCameraInputAgain)
    {
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });

        const bool consumed = m_cameraSystem->HandleEvents(
            AzFramework::InputState{ AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began },
                                     AzFramework::ModifierKeyStates{} });

        using ::testing::IsFalse;
        using ::testing::IsTrue;
        EXPECT_THAT(consumed, IsTrue());
        EXPECT_THAT(m_firstPersonTranslateCamera->Beginning(), IsFalse());
        EXPECT_THAT(m_firstPersonTranslateCamera->Active(), IsTrue());
    }

    TEST_F(CameraInputFixture, InvalidTranslationInputKeyUpDoesNotAffectTranslateCameraInputEnd)
    {
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });

        const bool consumed = m_cameraSystem->HandleEvents(
            AzFramework::InputState{ AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began },
                                     AzFramework::ModifierKeyStates{} });

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Ended },
            AzFramework::ModifierKeyStates{} });

        using ::testing::IsFalse;
        using ::testing::IsTrue;
        EXPECT_THAT(consumed, IsTrue());
        EXPECT_THAT(m_firstPersonTranslateCamera->Idle(), IsTrue());
    }

    TEST_F(CameraInputFixture, OrbitCameraInputCannotBeLeftInInvalidStateIfItCannotFullyBeginAfterInputChannelBegin)
    {
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });

        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began },
                                     AzFramework::ModifierKeyStates{} });

        using ::testing::IsFalse;
        using ::testing::IsTrue;
        EXPECT_THAT(m_orbitCamera->Beginning(), IsFalse());
        EXPECT_THAT(m_orbitCamera->Idle(), IsTrue());
    }

    TEST_F(CameraInputFixture, OrbitCameraInputCannotBeLeftInInvalidStateIfItCannotFullyBeginAfterInputChannelBeginAndEnd)
    {
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_forwardChannelId, AzFramework::InputChannel::State::Began },
            AzFramework::ModifierKeyStates{} });

        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began },
                                     AzFramework::ModifierKeyStates{} });
        HandleEventAndUpdate(
            AzFramework::InputState{ AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Ended },
                                     AzFramework::ModifierKeyStates{} });

        using ::testing::IsFalse;
        using ::testing::IsTrue;
        EXPECT_THAT(m_orbitCamera->Ending(), IsFalse());
        EXPECT_THAT(m_orbitCamera->Idle(), IsTrue());
    }

    TEST_F(CameraInputFixture, NewCameraInputCanBeAddedToCameraSystem)
    {
        auto firstPersonPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            AzFramework::InputDeviceMouse::Button::Middle, AzFramework::LookPan, AzFramework::TranslatePivotLook);
        const bool added =
            m_cameraSystem->m_cameras.AddCameras(AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>{ firstPersonPanCamera });

        EXPECT_THAT(added, ::testing::IsTrue());
    }

    TEST_F(CameraInputFixture, ExistingCameraInputCannotBeAddedToCameraSystem)
    {
        const bool added =
            m_cameraSystem->m_cameras.AddCameras(AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>{ m_firstPersonRotateCamera });

        EXPECT_THAT(added, ::testing::IsFalse());
    }

    TEST_F(CameraInputFixture, ExistingCameraInputCanBeRemovedFromCameraSystem)
    {
        const bool removed = m_cameraSystem->m_cameras.RemoveCameras(
            AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>{ m_firstPersonRotateCamera });

        EXPECT_THAT(removed, ::testing::IsTrue());
    }

    TEST_F(CameraInputFixture, NonExistentCameraInputCannotBeRemovedFromCameraSystem)
    {
        auto firstPersonPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            AzFramework::InputDeviceMouse::Button::Middle, AzFramework::LookPan, AzFramework::TranslatePivotLook);
        const bool removed =
            m_cameraSystem->m_cameras.RemoveCameras(AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>{ firstPersonPanCamera });

        EXPECT_THAT(removed, ::testing::IsFalse());
    }

    // note: this test is attempting to mimic the behavior that happens when a user presses 'ctrl-tab' to change focus from the editor
    // which can cause the event/update order to become irregular - this test verifies the orbit behavior does not change the position
    // of the camera if updates are dropped (due to the editor losing focus)
    TEST_F(CameraInputFixture, OrbitRotateCameraInputDoesNotResetPositionWithInconsitentEventAndUpdate)
    {
        const AzFramework::ModifierKeyStates orbitModifierKeystate = OrbitModifierKeyStates(m_orbitChannelId);

        const auto cameraStartingPosition = AZ::Vector3::CreateAxisY(-20.0f);
        m_targetCamera.m_pivot = cameraStartingPosition;
        m_pivot = AZ::Vector3::CreateAxisY(-10.0f);

        HandleEvent(AzFramework::InputState{ AzFramework::DiscreteInputEvent{ m_orbitChannelId, AzFramework::InputChannel::State::Began },
                                             orbitModifierKeystate });
        Update();
        HandleEvent(
            AzFramework::InputState{ AzFramework::CursorEvent{ AzFramework::ScreenPoint{ 100, 100 } }, AzFramework::ModifierKeyStates{} });
        HandleEvent(AzFramework::InputState{ AzFramework::CursorEvent{ AzFramework::ScreenPoint{ 100, 100 } }, orbitModifierKeystate });
        Update();

        EXPECT_THAT(m_camera.Translation(), IsCloseTolerance(cameraStartingPosition, 0.01f));
    }

    TEST_F(CameraInputFixture, TranslateCameraInputBoostDoesNotGetStuckOn)
    {
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_leftChannelId, AzFramework::InputChannel::State::Began },
            BoostModifierKeyStates(m_translateCameraInputChannelIds.m_boostChannelId, false) });
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_boostChannelId, AzFramework::InputChannel::State::Began },
            BoostModifierKeyStates(m_translateCameraInputChannelIds.m_boostChannelId, true) });

        EXPECT_THAT(m_firstPersonTranslateCamera->Boosting(), ::testing::IsTrue());

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_leftChannelId, AzFramework::InputChannel::State::Ended },
            BoostModifierKeyStates(m_translateCameraInputChannelIds.m_boostChannelId, true) });
        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_boostChannelId, AzFramework::InputChannel::State::Ended },
            BoostModifierKeyStates(m_translateCameraInputChannelIds.m_boostChannelId, false) });

        HandleEventAndUpdate(AzFramework::InputState{
            AzFramework::DiscreteInputEvent{ m_translateCameraInputChannelIds.m_leftChannelId, AzFramework::InputChannel::State::Began },
            BoostModifierKeyStates(m_translateCameraInputChannelIds.m_boostChannelId, false) });

        EXPECT_THAT(m_firstPersonTranslateCamera->Boosting(), ::testing::IsFalse());
    }
} // namespace UnitTest
