/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzTest/GemTestEnvironment.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <EditorModularViewportCameraComposer.h>
#include <EditorViewportCamera.h>
#include <EditorViewportSettings.h>
#include <Mocks/MockViewportRequests.h>
#include <Tests/Utils/Printers.h>

#include <GotoPositionDlg.h>

namespace UnitTest
{
    class EditorCameraFixture : public UnitTest::LeakDetectionFixture
    {
    public:
        AZ::ComponentApplication* m_application = nullptr;
        AtomToolsFramework::ModularCameraViewportContext* m_cameraViewportContextView = nullptr;
        AZStd::unique_ptr<SandboxEditor::EditorModularViewportCameraComposer> m_editorModularViewportCameraComposer;
        AzFramework::ViewportControllerListPtr m_controllerList;
        AZ::Entity* m_entity = nullptr;
        AZ::ComponentDescriptor* m_transformComponent = nullptr;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        ::testing::NiceMock<MockViewportRequests> m_mockViewportRequests;

        static inline constexpr AzFramework::ViewportId TestViewportId = 2345;
        static inline constexpr float InterpolateToTransformDuration = 1.0f;
        static inline constexpr float HalfInterpolateToTransformDuration = InterpolateToTransformDuration * 0.5f;

        void SetUp() override
        {
            m_application = aznew AZ::ComponentApplication;
            AZ::ComponentApplication::Descriptor appDesc;
            m_entity = m_application->Create(appDesc);
            m_transformComponent = AzToolsFramework::Components::TransformComponent::CreateDescriptor();
            m_application->RegisterComponentDescriptor(m_transformComponent);

            m_entity->Init();
            m_entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            m_entity->Activate();

            m_controllerList = AZStd::make_shared<AzFramework::ViewportControllerList>();
            m_controllerList->RegisterViewportContext(TestViewportId);

            m_editorModularViewportCameraComposer = AZStd::make_unique<SandboxEditor::EditorModularViewportCameraComposer>(TestViewportId);

            auto controller = m_editorModularViewportCameraComposer->CreateModularViewportCameraController();
            // set some overrides for the test
            controller->SetCameraViewportContextBuilderCallback(
                [this](AZStd::unique_ptr<AtomToolsFramework::ModularCameraViewportContext>& cameraViewportContext) mutable
                {
                    cameraViewportContext = AZStd::make_unique<AtomToolsFramework::PlaceholderModularCameraViewportContextImpl>();
                    m_cameraViewportContextView = cameraViewportContext.get();
                });

            m_controllerList->Add(controller);

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            ON_CALL(m_mockViewportRequests, GetCameraTransform)
                .WillByDefault(
                    [this]
                    {
                        return m_cameraViewportContextView->GetCameraTransform();
                    });
            ON_CALL(m_mockViewportRequests, SetCameraTransform)
                .WillByDefault(
                    [this](const AZ::Transform& transform)
                    {
                        m_cameraViewportContextView->SetCameraTransform(transform);
                    });

            m_mockViewportRequests.Connect(TestViewportId);
        }

        void TearDown() override
        {
            m_mockViewportRequests.Disconnect();

            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();

            m_editorModularViewportCameraComposer.reset();
            m_cameraViewportContextView = nullptr;

            if (m_application)
            {
                m_application->UnregisterComponentDescriptor(m_transformComponent);
                delete m_transformComponent;
                m_transformComponent = nullptr;

                m_application->Destroy();
                delete m_application;
                m_application = nullptr;
            }
        }
    };

    TEST_F(EditorCameraFixture, ModularViewportCameraControllerReferenceFrameUpdatedWhenViewportEntityisChanged)
    {
        // Given
        const auto entityTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)), AZ::Vector3(10.0f, 5.0f, -2.0f));
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, entityTransform);

        // When
        // imitate viewport entity changing
        Camera::EditorCameraNotificationBus::Broadcast(
            &Camera::EditorCameraNotificationBus::Events::OnViewportViewEntityChanged, m_entity->GetId());

        // ensure the viewport updates after the viewport view entity change
        // note: do a large step to ensure smoothing finishes (e.g. not 1.0f/60.0f)
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(2.0f), AZ::ScriptTimePoint() });

        // retrieve updated camera transform
        const AZ::Transform cameraTransform = m_cameraViewportContextView->GetCameraTransform();

        // Then
        // camera transform matches that of the entity
        EXPECT_THAT(cameraTransform, IsClose(entityTransform));
    }

    TEST_F(EditorCameraFixture, TrackingTransformIsTrueAfterTransformIsTracked)
    {
        // Given/When
        const AZ::Transform referenceFrame = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)), AZ::Vector3(1.0f, 2.0f, 3.0f));
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
            TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::StartTrackingTransform, referenceFrame);

        bool trackingTransform = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            trackingTransform, TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsTrackingTransform);

        // Then
        EXPECT_THAT(trackingTransform, ::testing::IsTrue());
    }

    TEST_F(EditorCameraFixture, TrackingTransformIsFalseAfterTransformIsStoppedBeingTracked)
    {
        // Given
        const AZ::Transform referenceFrame = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)), AZ::Vector3(1.0f, 2.0f, 3.0f));
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
            TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::StartTrackingTransform, referenceFrame);

        // When
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
            TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::StopTrackingTransform);

        // Then
        bool trackingTransform = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            trackingTransform, TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsTrackingTransform);

        EXPECT_THAT(trackingTransform, ::testing::IsFalse());
    }

    TEST_F(EditorCameraFixture, InterpolateToTransform)
    {
        // When
        AZ::Transform transformToInterpolateTo = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationZ(AZ::DegToRad(90.0f)), AZ::Vector3(20.0f, 40.0f, 60.0f));
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transformToInterpolateTo,
            InterpolateToTransformDuration);

        // simulate interpolation
        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(HalfInterpolateToTransformDuration), AZ::ScriptTimePoint() });
        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(HalfInterpolateToTransformDuration), AZ::ScriptTimePoint() });

        const auto finalTransform = m_cameraViewportContextView->GetCameraTransform();

        // Then
        EXPECT_THAT(finalTransform, IsClose(transformToInterpolateTo));
    }

    TEST_F(EditorCameraFixture, InterpolateToTransformWithReferenceSpaceSet)
    {
        // Given
        const AZ::Transform referenceFrame = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)), AZ::Vector3(1.0f, 2.0f, 3.0f));
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
            TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::StartTrackingTransform, referenceFrame);

        AZ::Transform transformToInterpolateTo = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationZ(AZ::DegToRad(90.0f)), AZ::Vector3(20.0f, 40.0f, 60.0f));

        // When
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transformToInterpolateTo,
            InterpolateToTransformDuration);

        // simulate interpolation
        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(HalfInterpolateToTransformDuration), AZ::ScriptTimePoint() });
        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(HalfInterpolateToTransformDuration), AZ::ScriptTimePoint() });

        const auto finalTransform = m_cameraViewportContextView->GetCameraTransform();

        // Then
        EXPECT_THAT(finalTransform, IsClose(transformToInterpolateTo));
    }

    TEST_F(EditorCameraFixture, BeginningCameraInterpolationReturnsTrue)
    {
        // Given/When
        bool interpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            interpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f)),
            InterpolateToTransformDuration);

        // Then
        EXPECT_THAT(interpolationBegan, ::testing::IsTrue());
    }

    TEST_F(EditorCameraFixture, CameraInterpolationIsNotInterruptedIfGoingToTheSameTransform)
    {
        const auto& transformToInterpolateTo = AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f));

        // Given/When
        bool initialInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            initialInterpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transformToInterpolateTo,
            InterpolateToTransformDuration);

        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(HalfInterpolateToTransformDuration), AZ::ScriptTimePoint() });

        bool nextInterpolationBegan = true;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            nextInterpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transformToInterpolateTo,
            InterpolateToTransformDuration);

        bool interpolating = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            interpolating, TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsInterpolating);

        // Then
        EXPECT_THAT(initialInterpolationBegan, ::testing::IsTrue());
        EXPECT_THAT(nextInterpolationBegan, ::testing::IsFalse());
        EXPECT_THAT(interpolating, ::testing::IsTrue());
    }

    TEST_F(EditorCameraFixture, CameraInterpolationCanBeInterruptedIfGoingToDifferentTransform)
    {
        const auto& firstTransformToInterpolateTo = AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f));
        const auto& secondTransformToInterpolateTo = AZ::Transform::CreateTranslation(AZ::Vector3(50.0f, 20.0f, 100.0f));

        // Given/When
        bool initialInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            initialInterpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            firstTransformToInterpolateTo,
            InterpolateToTransformDuration);

        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(HalfInterpolateToTransformDuration), AZ::ScriptTimePoint() });

        bool nextInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            nextInterpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            secondTransformToInterpolateTo,
            InterpolateToTransformDuration);

        bool interpolating = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            interpolating, TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsInterpolating);

        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(InterpolateToTransformDuration), AZ::ScriptTimePoint() });

        const auto actualTransformAfterUpdate = SandboxEditor::GetViewportCameraTransform(TestViewportId);

        // Then
        EXPECT_THAT(actualTransformAfterUpdate, IsClose(secondTransformToInterpolateTo));
        EXPECT_THAT(initialInterpolationBegan, ::testing::IsTrue());
        EXPECT_THAT(nextInterpolationBegan, ::testing::IsTrue());
        EXPECT_THAT(interpolating, ::testing::IsTrue());
    }

    TEST_F(EditorCameraFixture, CameraInterpolationCanBeginAfterAnInterpolationCompletes)
    {
        // Given/When
        bool initialInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            initialInterpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f)),
            InterpolateToTransformDuration);

        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(InterpolateToTransformDuration + 0.5f), AZ::ScriptTimePoint() });

        bool interpolating = true;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            interpolating, TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsInterpolating);

        bool nextInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            nextInterpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            AZ::Transform::CreateTranslation(AZ::Vector3(20.0f, 20.0f, 20.0f)),
            InterpolateToTransformDuration);

        // Then
        EXPECT_THAT(initialInterpolationBegan, ::testing::IsTrue());
        EXPECT_THAT(interpolating, ::testing::IsFalse());
        EXPECT_THAT(nextInterpolationBegan, ::testing::IsTrue());
    }

    TEST_F(EditorCameraFixture, CameraCannotInterpolateToSamePositionWithoutMovingFromIt)
    {
        const auto& transformToInterpolateTo = AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f));

        // Given/When
        bool initialInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            initialInterpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transformToInterpolateTo,
            InterpolateToTransformDuration);

        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(InterpolateToTransformDuration), AZ::ScriptTimePoint() });

        bool interpolating = true;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            interpolating, TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsInterpolating);

        bool nextInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            nextInterpolationBegan,
            TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transformToInterpolateTo,
            InterpolateToTransformDuration);

        // Then
        EXPECT_THAT(initialInterpolationBegan, ::testing::IsTrue());
        EXPECT_THAT(interpolating, ::testing::IsFalse());
        EXPECT_THAT(nextInterpolationBegan, ::testing::IsFalse());
    }

    TEST_F(EditorCameraFixture, CameraInterpolatesToTransformWhenGoToPositionInstantlyIsOff)
    {
        // Given
        SandboxEditor::SetCameraGoToPositionInstantlyEnabled(false);
        SandboxEditor::SetCameraGoToPositionDuration(InterpolateToTransformDuration);

        // set initial camera transform
        SandboxEditor::SetViewportCameraTransform(TestViewportId, AZ::Transform::CreateIdentity());

        const auto startingPosition = AZ::Vector3(10.0f, 20.0f, 30.0f);
        const auto pitchRadians = AZ::DegToRad(-45.0f);
        const auto yawRadians = AZ::DegToRad(90.0f);
        const auto expectedTransform = SandboxEditor::TransformFromPositionPitchYaw(startingPosition, pitchRadians, yawRadians);

        // When
        // camera transition (interpolation or instant based on setting)
        SandboxEditor::HandleViewportCameraTransitionFromSetting(TestViewportId, startingPosition, pitchRadians, yawRadians);

        // Then
        // ensure camera transform did not change instantly
        const auto actualTransformBeforeUpdate = SandboxEditor::GetViewportCameraTransform(TestViewportId);
        EXPECT_THAT(actualTransformBeforeUpdate, ::testing::Not(IsClose(expectedTransform)));
        EXPECT_THAT(actualTransformBeforeUpdate, IsClose(AZ::Transform::CreateIdentity()));

        // simulate viewport update
        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(InterpolateToTransformDuration), AZ::ScriptTimePoint() });

        // ensure camera transform is at interpolated position/orientation
        const auto actualTransformAfterUpdate = SandboxEditor::GetViewportCameraTransform(TestViewportId);
        EXPECT_THAT(actualTransformAfterUpdate, IsClose(expectedTransform));
    }

    TEST_F(EditorCameraFixture, CameraChangesImmediatelyWhenGoToPositionInstantlyIsOn)
    {
        // Given
        SandboxEditor::SetCameraGoToPositionInstantlyEnabled(true);
        SandboxEditor::SetCameraGoToPositionDuration(InterpolateToTransformDuration);

        // set initial camera transform
        SandboxEditor::SetViewportCameraTransform(TestViewportId, AZ::Transform::CreateIdentity());

        const auto startingPosition = AZ::Vector3(50.0f, 60.0f, 70.0f);
        const auto pitchRadians = AZ::DegToRad(45.0f);
        const auto yawRadians = AZ::DegToRad(180.0f);
        const auto expectedTransform = SandboxEditor::TransformFromPositionPitchYaw(startingPosition, pitchRadians, yawRadians);

        // When
        // camera transition (interpolation or instant based on setting)
        SandboxEditor::HandleViewportCameraTransitionFromSetting(TestViewportId, expectedTransform);

        // Then
        // ensure camera transform updated immediately
        const auto actualTransformAfterUpdate = SandboxEditor::GetViewportCameraTransform(TestViewportId);
        EXPECT_THAT(actualTransformAfterUpdate, IsClose(expectedTransform));
    }

    TEST_F(EditorCameraFixture, CameraInterpolatesToTransform)
    {
        // Given
        SandboxEditor::SetViewportCameraTransform(TestViewportId, AZ::Transform::CreateIdentity());

        const auto startingPosition = AZ::Vector3(1.0f, 2.0f, 3.0f);
        const auto pitchRadians = AZ::DegToRad(30.0f);
        const auto yawRadians = AZ::DegToRad(20.0f);
        const auto expectedTransform = SandboxEditor::TransformFromPositionPitchYaw(startingPosition, pitchRadians, yawRadians);

        // When
        // interpolate camera to transform
        SandboxEditor::InterpolateViewportCameraToTransform(
            TestViewportId, startingPosition, pitchRadians, yawRadians, InterpolateToTransformDuration);

        // simulate viewport update
        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(InterpolateToTransformDuration), AZ::ScriptTimePoint() });

        // Then
        const auto actualTransform = SandboxEditor::GetViewportCameraTransform(TestViewportId);
        EXPECT_THAT(actualTransform, IsClose(expectedTransform));
    }

    TEST_F(EditorCameraFixture, CameraInterpolatesToTransformWithZeroDurationAfterUpdate)
    {
        // Given
        SandboxEditor::SetViewportCameraTransform(TestViewportId, AZ::Transform::CreateIdentity());

        const auto startingPosition = AZ::Vector3(1.0f, 2.0f, 3.0f);
        const auto pitchRadians = AZ::DegToRad(30.0f);
        const auto yawRadians = AZ::DegToRad(20.0f);
        const auto expectedTransform = SandboxEditor::TransformFromPositionPitchYaw(startingPosition, pitchRadians, yawRadians);

        // When
        // interpolate camera to transform (zero duration)
        SandboxEditor::InterpolateViewportCameraToTransform(TestViewportId, startingPosition, pitchRadians, yawRadians, 0.0f);

        // simulate viewport update
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(0.0166f), AZ::ScriptTimePoint() });

        // Then
        const auto actualTransform = SandboxEditor::GetViewportCameraTransform(TestViewportId);
        EXPECT_THAT(actualTransform, IsClose(expectedTransform));
    }

    TEST_F(EditorCameraFixture, CameraDoesNotInterpolateToTransformWithZeroDurationAndNoUpdate)
    {
        // Given
        SandboxEditor::SetViewportCameraTransform(TestViewportId, AZ::Transform::CreateIdentity());

        const auto startingPosition = AZ::Vector3(1.0f, 2.0f, 3.0f);
        const auto pitchRadians = AZ::DegToRad(30.0f);
        const auto yawRadians = AZ::DegToRad(20.0f);

        // When
        // interpolate camera to transform (zero duration)
        SandboxEditor::InterpolateViewportCameraToTransform(TestViewportId, startingPosition, pitchRadians, yawRadians, 0.0f);

        // no update

        // Then
        const auto actualTransform = SandboxEditor::GetViewportCameraTransform(TestViewportId);
        EXPECT_THAT(actualTransform, IsClose(AZ::Transform::CreateIdentity()));
    }

    using GotoPositionPitchConstraintsTest = UnitTest::LeakDetectionFixture;
    TEST_F(GotoPositionPitchConstraintsTest, GoToPositionPitchIsSetToPlusOrMinusNinetyDegrees)
    {
        float minPitch = 0.0f;
        float maxPitch = 0.0f;

        GoToPositionPitchConstraints m_goToPositionContraints;
        m_goToPositionContraints.DeterminePitchRange(
            [&minPitch, &maxPitch](const float minPitchDegrees, const float maxPitchDegrees)
            {
                minPitch = minPitchDegrees;
                maxPitch = maxPitchDegrees;
            });

        using ::testing::FloatNear;
        EXPECT_THAT(minPitch, FloatNear(-90.0f, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(maxPitch, FloatNear(90.0f, AZ::Constants::FloatEpsilon));
    }

    TEST_F(GotoPositionPitchConstraintsTest, GoToPositionPitchClampsFinalPitchValueWithTolerance)
    {
        const auto [expectedMinPitchRadians, expectedMaxPitchRadians] = AzFramework::CameraPitchMinMaxRadiansWithTolerance();

        GoToPositionPitchConstraints m_goToPositionContraints;
        const float minClampedPitchRadians = m_goToPositionContraints.PitchClampedRadians(-90.0f);
        const float maxClampedPitchRadians = m_goToPositionContraints.PitchClampedRadians(90.0f);

        using ::testing::FloatNear;
        EXPECT_THAT(minClampedPitchRadians, FloatNear(expectedMinPitchRadians, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(maxClampedPitchRadians, FloatNear(expectedMaxPitchRadians, AZ::Constants::FloatEpsilon));
    }

    TEST_F(EditorCameraFixture, CameraSettingsRegistryValuesCanBeModified)
    {
        using ::testing::Eq;
        using ::testing::FloatNear;

        {
            const float existingCameraSpeedScale = SandboxEditor::CameraSpeedScale();
            const float expectedCameraSpeedScale = existingCameraSpeedScale + 1.0f;
            SandboxEditor::SetCameraSpeedScale(expectedCameraSpeedScale);
            const float actualCameraSpeedScale = SandboxEditor::CameraSpeedScale();
            EXPECT_THAT(expectedCameraSpeedScale, FloatNear(actualCameraSpeedScale, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraTranslateSpeed = SandboxEditor::CameraTranslateSpeed();
            const float expectedCameraTranslateSpeed = existingCameraTranslateSpeed + 1.0f;
            SandboxEditor::SetCameraTranslateSpeed(expectedCameraTranslateSpeed);
            const float actualCameraTranslateSpeed = SandboxEditor::CameraTranslateSpeed();
            EXPECT_THAT(expectedCameraTranslateSpeed, FloatNear(actualCameraTranslateSpeed, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraBoostMultiplier = SandboxEditor::CameraBoostMultiplier();
            const float expectedCameraBoostMultiplier = existingCameraBoostMultiplier + 1.0f;
            SandboxEditor::SetCameraBoostMultiplier(expectedCameraBoostMultiplier);
            const float actualCameraBoostMultiplier = SandboxEditor::CameraBoostMultiplier();
            EXPECT_THAT(expectedCameraBoostMultiplier, FloatNear(actualCameraBoostMultiplier, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraRotateSpeed = SandboxEditor::CameraRotateSpeed();
            const float expectedCameraRotateSpeed = existingCameraRotateSpeed + 1.0f;
            SandboxEditor::SetCameraRotateSpeed(expectedCameraRotateSpeed);
            const float actualCameraRotateSpeed = SandboxEditor::CameraRotateSpeed();
            EXPECT_THAT(expectedCameraRotateSpeed, FloatNear(actualCameraRotateSpeed, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraScrollSpeed = SandboxEditor::CameraScrollSpeed();
            const float expectedCameraScrollSpeed = existingCameraScrollSpeed + 1.0f;
            SandboxEditor::SetCameraScrollSpeed(expectedCameraScrollSpeed);
            const float actualCameraScrollSpeed = SandboxEditor::CameraScrollSpeed();
            EXPECT_THAT(expectedCameraScrollSpeed, FloatNear(actualCameraScrollSpeed, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraDollyMotionSpeed = SandboxEditor::CameraDollyMotionSpeed();
            const float expectedCameraDollyMotionSpeed = existingCameraDollyMotionSpeed + 1.0f;
            SandboxEditor::SetCameraDollyMotionSpeed(expectedCameraDollyMotionSpeed);
            const float actualCameraDollyMotionSpeed = SandboxEditor::CameraDollyMotionSpeed();
            EXPECT_THAT(expectedCameraDollyMotionSpeed, FloatNear(actualCameraDollyMotionSpeed, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraPanSpeed = SandboxEditor::CameraPanSpeed();
            const float expectedCameraPanSpeed = existingCameraPanSpeed + 1.0f;
            SandboxEditor::SetCameraPanSpeed(expectedCameraPanSpeed);
            const float actualCameraPanSpeed = SandboxEditor::CameraPanSpeed();
            EXPECT_THAT(expectedCameraPanSpeed, FloatNear(actualCameraPanSpeed, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraRotateSmoothness = SandboxEditor::CameraRotateSmoothness();
            const float expectedCameraRotateSmoothness = existingCameraRotateSmoothness + 1.0f;
            SandboxEditor::SetCameraRotateSmoothness(expectedCameraRotateSmoothness);
            const float actualCameraRotateSmoothness = SandboxEditor::CameraRotateSmoothness();
            EXPECT_THAT(expectedCameraRotateSmoothness, FloatNear(actualCameraRotateSmoothness, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraTranslateSmoothness = SandboxEditor::CameraTranslateSmoothness();
            const float expectedCameraTranslateSmoothness = existingCameraTranslateSmoothness + 1.0f;
            SandboxEditor::SetCameraTranslateSmoothness(expectedCameraTranslateSmoothness);
            const float actualCameraTranslateSmoothness = SandboxEditor::CameraTranslateSmoothness();
            EXPECT_THAT(expectedCameraTranslateSmoothness, FloatNear(actualCameraTranslateSmoothness, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraDefaultOrbitDistance = SandboxEditor::CameraDefaultOrbitDistance();
            const float expectedCameraDefaultOrbitDistance = existingCameraDefaultOrbitDistance + 1.0f;
            SandboxEditor::SetCameraDefaultOrbitDistance(expectedCameraDefaultOrbitDistance);
            const float actualCameraDefaultOrbitDistance = SandboxEditor::CameraDefaultOrbitDistance();
            EXPECT_THAT(expectedCameraDefaultOrbitDistance, FloatNear(actualCameraDefaultOrbitDistance, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraDefaultNearPlaneDistance = SandboxEditor::CameraDefaultNearPlaneDistance();
            const float expectedCameraDefaultNearPlaneDistance = existingCameraDefaultNearPlaneDistance + 1.0f;
            SandboxEditor::SetCameraDefaultNearPlaneDistance(expectedCameraDefaultNearPlaneDistance);
            const float actualCameraDefaultNearPlaneDistance = SandboxEditor::CameraDefaultNearPlaneDistance();
            EXPECT_THAT(
                expectedCameraDefaultNearPlaneDistance, FloatNear(actualCameraDefaultNearPlaneDistance, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraDefaultFarPlaneDistance = SandboxEditor::CameraDefaultFarPlaneDistance();
            const float expectedCameraDefaultFarPlaneDistance = existingCameraDefaultFarPlaneDistance + 1.0f;
            SandboxEditor::SetCameraDefaultFarPlaneDistance(expectedCameraDefaultFarPlaneDistance);
            const float actualCameraDefaultFarPlaneDistance = SandboxEditor::CameraDefaultFarPlaneDistance();
            EXPECT_THAT(expectedCameraDefaultFarPlaneDistance, FloatNear(actualCameraDefaultFarPlaneDistance, AZ::Constants::FloatEpsilon));
        }

        {
            const float existingCameraDefaultFovRadians = SandboxEditor::CameraDefaultFovRadians();
            const float expectedCameraDefaultFovRadians = existingCameraDefaultFovRadians + 1.0f;
            SandboxEditor::SetCameraDefaultFovRadians(expectedCameraDefaultFovRadians);
            const float actualCameraDefaultFovRadians = SandboxEditor::CameraDefaultFovRadians();
            // precision reduced here due to conversion to/from degrees in underlying code
            EXPECT_THAT(expectedCameraDefaultFovRadians, FloatNear(actualCameraDefaultFovRadians, 0.001f));
        }

        {
            const float existingCameraDefaultFovDegrees = SandboxEditor::CameraDefaultFovDegrees();
            const float expectedCameraDefaultFovDegrees = existingCameraDefaultFovDegrees + 1.0f;
            SandboxEditor::SetCameraDefaultFovDegrees(expectedCameraDefaultFovDegrees);
            const float actualCameraDefaultFovDegrees = SandboxEditor::CameraDefaultFovDegrees();
            EXPECT_THAT(expectedCameraDefaultFovDegrees, FloatNear(actualCameraDefaultFovDegrees, AZ::Constants::FloatEpsilon));
        }

        {
            const bool existingCameraOrbitYawRotationInverted = SandboxEditor::CameraOrbitYawRotationInverted();
            const bool expectedCameraOrbitYawRotationInverted = !existingCameraOrbitYawRotationInverted;
            SandboxEditor::SetCameraOrbitYawRotationInverted(expectedCameraOrbitYawRotationInverted);
            const bool actualCameraOrbitYawRotationInverted = SandboxEditor::CameraOrbitYawRotationInverted();
            EXPECT_THAT(expectedCameraOrbitYawRotationInverted, Eq(actualCameraOrbitYawRotationInverted));
        }

        {
            const bool existingCameraPanInvertedX = SandboxEditor::CameraPanInvertedX();
            const bool expectedCameraPanInvertedX = !existingCameraPanInvertedX;
            SandboxEditor::SetCameraPanInvertedX(expectedCameraPanInvertedX);
            const bool actualCameraPanInvertedX = SandboxEditor::CameraPanInvertedX();
            EXPECT_THAT(expectedCameraPanInvertedX, Eq(actualCameraPanInvertedX));
        }

        {
            const bool existingCameraPanInvertedY = SandboxEditor::CameraPanInvertedY();
            const bool expectedCameraPanInvertedY = !existingCameraPanInvertedY;
            SandboxEditor::SetCameraPanInvertedY(expectedCameraPanInvertedY);
            const bool actualCameraPanInvertedY = SandboxEditor::CameraPanInvertedY();
            EXPECT_THAT(expectedCameraPanInvertedY, Eq(actualCameraPanInvertedY));
        }

        {
            const bool existingCameraRotateSmoothingEnabled = SandboxEditor::CameraRotateSmoothingEnabled();
            const bool expectedCameraRotateSmoothingEnabled = !existingCameraRotateSmoothingEnabled;
            SandboxEditor::SetCameraRotateSmoothingEnabled(expectedCameraRotateSmoothingEnabled);
            const bool actualCameraRotateSmoothingEnabled = SandboxEditor::CameraRotateSmoothingEnabled();
            EXPECT_THAT(expectedCameraRotateSmoothingEnabled, Eq(actualCameraRotateSmoothingEnabled));
        }

        {
            const bool existingCameraTranslateSmoothingEnabled = SandboxEditor::CameraTranslateSmoothingEnabled();
            const bool expectedCameraTranslateSmoothingEnabled = !existingCameraTranslateSmoothingEnabled;
            SandboxEditor::SetCameraTranslateSmoothingEnabled(expectedCameraTranslateSmoothingEnabled);
            const bool actualCameraTranslateSmoothingEnabled = SandboxEditor::CameraTranslateSmoothingEnabled();
            EXPECT_THAT(expectedCameraTranslateSmoothingEnabled, Eq(actualCameraTranslateSmoothingEnabled));
        }

        {
            const bool existingCameraCaptureCursorForLook = SandboxEditor::CameraCaptureCursorForLook();
            const bool expectedCameraCaptureCursorForLook = !existingCameraCaptureCursorForLook;
            SandboxEditor::SetCameraCaptureCursorForLook(expectedCameraCaptureCursorForLook);
            const bool actualCameraCaptureCursorForLook = SandboxEditor::CameraCaptureCursorForLook();
            EXPECT_THAT(expectedCameraCaptureCursorForLook, Eq(actualCameraCaptureCursorForLook));
        }

        {
            const bool existingCameraGoToPositionInstantly = SandboxEditor::CameraGoToPositionInstantlyEnabled();
            const bool expectedCameraGoToPositionInstantly = !existingCameraGoToPositionInstantly;
            SandboxEditor::SetCameraGoToPositionInstantlyEnabled(expectedCameraGoToPositionInstantly);
            const bool actualCameraGoToPositionInstantly = SandboxEditor::CameraGoToPositionInstantlyEnabled();
            EXPECT_THAT(expectedCameraGoToPositionInstantly, Eq(actualCameraGoToPositionInstantly));
        }

        {
            const float existingCameraGoToPositionDuration = SandboxEditor::CameraGoToPositionDuration();
            const float expectedCameraGoToPositionDuration = existingCameraGoToPositionDuration + 1.0f;
            SandboxEditor::SetCameraGoToPositionDuration(expectedCameraGoToPositionDuration);
            const float actualCameraGoToPositionDuration = SandboxEditor::CameraGoToPositionDuration();
            EXPECT_THAT(expectedCameraGoToPositionDuration, Eq(actualCameraGoToPositionDuration));
        }
    }

    TEST_F(EditorCameraFixture, CameraSettingsRegistryValuesCanBeReset)
    {
        using ::testing::Eq;
        using ::testing::FloatNear;
        using ::testing::IsTrue;
        using ::testing::Not;

        // reset all relevant settings to defaults
        SandboxEditor::ResetCameraSpeedScale();
        SandboxEditor::ResetCameraTranslateSpeed();
        SandboxEditor::ResetCameraRotateSpeed();
        SandboxEditor::ResetCameraBoostMultiplier();
        SandboxEditor::ResetCameraScrollSpeed();
        SandboxEditor::ResetCameraDollyMotionSpeed();
        SandboxEditor::ResetCameraPanSpeed();
        SandboxEditor::ResetCameraRotateSmoothness();
        SandboxEditor::ResetCameraRotateSmoothingEnabled();
        SandboxEditor::ResetCameraTranslateSmoothness();
        SandboxEditor::ResetCameraTranslateSmoothingEnabled();
        SandboxEditor::ResetCameraCaptureCursorForLook();
        SandboxEditor::ResetCameraOrbitYawRotationInverted();
        SandboxEditor::ResetCameraPanInvertedX();
        SandboxEditor::ResetCameraPanInvertedY();
        SandboxEditor::ResetCameraDefaultEditorPosition();
        SandboxEditor::ResetCameraDefaultOrbitDistance();
        SandboxEditor::ResetCameraDefaultEditorOrientation();
        SandboxEditor::ResetCameraGoToPositionInstantlyEnabled();
        SandboxEditor::ResetCameraGoToPositionDuration();

        // store defaults
        const auto initialCameraSpeedScale = SandboxEditor::CameraSpeedScale();
        const auto initialCameraTranslateSpeed = SandboxEditor::CameraTranslateSpeed();
        const auto initialCameraRotateSpeed = SandboxEditor::CameraRotateSpeed();
        const auto initialCameraBoostMultiplier = SandboxEditor::CameraBoostMultiplier();
        const auto initialCameraScrollSpeed = SandboxEditor::CameraScrollSpeed();
        const auto initialCameraDollyMotionSpeed = SandboxEditor::CameraDollyMotionSpeed();
        const auto initialCameraPanSpeed = SandboxEditor::CameraPanSpeed();
        const auto initialCameraRotateSmoothness = SandboxEditor::CameraRotateSmoothness();
        const auto initialCameraRotateSmoothingEnabled = SandboxEditor::CameraRotateSmoothingEnabled();
        const auto initialCameraTranslateSmoothness = SandboxEditor::CameraTranslateSmoothness();
        const auto initialCameraTranslateSmoothingEnabled = SandboxEditor::CameraTranslateSmoothingEnabled();
        const auto initialCameraCaptureCursorForLook = SandboxEditor::CameraCaptureCursorForLook();
        const auto initialCameraOrbitYawRotationInverted = SandboxEditor::CameraOrbitYawRotationInverted();
        const auto initialCameraPanInvertedX = SandboxEditor::CameraPanInvertedX();
        const auto initialCameraPanInvertedY = SandboxEditor::CameraPanInvertedY();
        const auto initialCameraDefaultEditorPosition = SandboxEditor::CameraDefaultEditorPosition();
        const auto initialCameraDefaultOrbitDistance = SandboxEditor::CameraDefaultOrbitDistance();
        const auto initialCameraDefaultEditorOrientation = SandboxEditor::CameraDefaultEditorOrientation();
        const auto initialCameraGoToPositionInstantly = SandboxEditor::CameraGoToPositionInstantlyEnabled();
        const auto initialCameraGoToPositionDuration = SandboxEditor::CameraGoToPositionDuration();

        // modify all values to be different to default value
        SandboxEditor::SetCameraSpeedScale(SandboxEditor::CameraSpeedScale() + 10.0f);
        SandboxEditor::SetCameraTranslateSpeed(SandboxEditor::CameraTranslateSpeed() + 10.0f);
        SandboxEditor::SetCameraRotateSpeed(SandboxEditor::CameraRotateSpeed() + 10.0f);
        SandboxEditor::SetCameraBoostMultiplier(SandboxEditor::CameraBoostMultiplier() + 10.0f);
        SandboxEditor::SetCameraScrollSpeed(SandboxEditor::CameraScrollSpeed() + 10.0f);
        SandboxEditor::SetCameraDollyMotionSpeed(SandboxEditor::CameraDollyMotionSpeed() + 10.0f);
        SandboxEditor::SetCameraPanSpeed(SandboxEditor::CameraPanSpeed() + 10.0f);
        SandboxEditor::SetCameraRotateSmoothness(SandboxEditor::CameraRotateSmoothness() + 10.0f);
        SandboxEditor::SetCameraRotateSmoothingEnabled(!SandboxEditor::CameraRotateSmoothingEnabled());
        SandboxEditor::SetCameraTranslateSmoothness(SandboxEditor::CameraTranslateSmoothness() + 10.0f);
        SandboxEditor::SetCameraTranslateSmoothingEnabled(!SandboxEditor::CameraTranslateSmoothingEnabled());
        SandboxEditor::SetCameraCaptureCursorForLook(!SandboxEditor::CameraCaptureCursorForLook());
        SandboxEditor::SetCameraOrbitYawRotationInverted(!SandboxEditor::CameraOrbitYawRotationInverted());
        SandboxEditor::SetCameraPanInvertedX(!SandboxEditor::CameraPanInvertedX());
        SandboxEditor::SetCameraPanInvertedY(!SandboxEditor::CameraPanInvertedY());
        SandboxEditor::SetCameraDefaultEditorPosition(SandboxEditor::CameraDefaultEditorPosition() + AZ::Vector3(10.0f));
        SandboxEditor::SetCameraDefaultOrbitDistance(SandboxEditor::CameraDefaultOrbitDistance() + 10.0f);
        SandboxEditor::SetCameraDefaultEditorOrientation(SandboxEditor::CameraDefaultEditorOrientation() + AZ::Vector2(10.0f));
        SandboxEditor::SetCameraGoToPositionInstantlyEnabled(!SandboxEditor::CameraGoToPositionInstantlyEnabled());
        SandboxEditor::SetCameraGoToPositionDuration(SandboxEditor::CameraGoToPositionDuration() + 10.0f);

        // ensure all values have changed vs defaults
        EXPECT_THAT(SandboxEditor::CameraSpeedScale(), Not(FloatNear(initialCameraSpeedScale, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraTranslateSpeed(), Not(FloatNear(initialCameraTranslateSpeed, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraRotateSpeed(), Not(FloatNear(initialCameraRotateSpeed, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraBoostMultiplier(), Not(FloatNear(initialCameraBoostMultiplier, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraScrollSpeed(), Not(FloatNear(initialCameraScrollSpeed, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraDollyMotionSpeed(), Not(FloatNear(initialCameraDollyMotionSpeed, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraPanSpeed(), Not(FloatNear(initialCameraPanSpeed, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraRotateSmoothness(), Not(FloatNear(initialCameraRotateSmoothness, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraRotateSmoothingEnabled(), Not(Eq(initialCameraRotateSmoothingEnabled)));
        EXPECT_THAT(
            SandboxEditor::CameraTranslateSmoothness(), Not(FloatNear(initialCameraTranslateSmoothness, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraTranslateSmoothingEnabled(), Not(Eq(initialCameraTranslateSmoothingEnabled)));
        EXPECT_THAT(SandboxEditor::CameraCaptureCursorForLook(), Not(Eq(initialCameraCaptureCursorForLook)));
        EXPECT_THAT(SandboxEditor::CameraOrbitYawRotationInverted(), Not(Eq(initialCameraOrbitYawRotationInverted)));
        EXPECT_THAT(SandboxEditor::CameraPanInvertedX(), Not(Eq(initialCameraPanInvertedX)));
        EXPECT_THAT(SandboxEditor::CameraPanInvertedY(), Not(Eq(initialCameraPanInvertedY)));
        EXPECT_THAT(SandboxEditor::CameraDefaultEditorPosition(), Not(IsClose(initialCameraDefaultEditorPosition)));
        EXPECT_THAT(
            SandboxEditor::CameraDefaultOrbitDistance(), Not(FloatNear(initialCameraDefaultOrbitDistance, AZ::Constants::FloatEpsilon)));
        EXPECT_THAT(SandboxEditor::CameraDefaultEditorOrientation(), Not(IsClose(initialCameraDefaultEditorOrientation)));
        EXPECT_THAT(SandboxEditor::CameraGoToPositionInstantlyEnabled(), Not(Eq(initialCameraGoToPositionInstantly)));
        EXPECT_THAT(SandboxEditor::CameraGoToPositionDuration(), Not(Eq(initialCameraGoToPositionDuration)));

        // reset all relevant settings to defaults again
        SandboxEditor::ResetCameraSpeedScale();
        SandboxEditor::ResetCameraTranslateSpeed();
        SandboxEditor::ResetCameraRotateSpeed();
        SandboxEditor::ResetCameraBoostMultiplier();
        SandboxEditor::ResetCameraScrollSpeed();
        SandboxEditor::ResetCameraDollyMotionSpeed();
        SandboxEditor::ResetCameraPanSpeed();
        SandboxEditor::ResetCameraRotateSmoothness();
        SandboxEditor::ResetCameraRotateSmoothingEnabled();
        SandboxEditor::ResetCameraTranslateSmoothness();
        SandboxEditor::ResetCameraTranslateSmoothingEnabled();
        SandboxEditor::ResetCameraCaptureCursorForLook();
        SandboxEditor::ResetCameraOrbitYawRotationInverted();
        SandboxEditor::ResetCameraPanInvertedX();
        SandboxEditor::ResetCameraPanInvertedY();
        SandboxEditor::ResetCameraDefaultEditorPosition();
        SandboxEditor::ResetCameraDefaultOrbitDistance();
        SandboxEditor::ResetCameraDefaultEditorOrientation();
        SandboxEditor::ResetCameraGoToPositionInstantlyEnabled();
        SandboxEditor::ResetCameraGoToPositionDuration();

        // ensure values have been reset to defaults
        EXPECT_THAT(SandboxEditor::CameraSpeedScale(), FloatNear(initialCameraSpeedScale, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraTranslateSpeed(), FloatNear(initialCameraTranslateSpeed, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraRotateSpeed(), FloatNear(initialCameraRotateSpeed, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraBoostMultiplier(), FloatNear(initialCameraBoostMultiplier, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraScrollSpeed(), FloatNear(initialCameraScrollSpeed, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraDollyMotionSpeed(), FloatNear(initialCameraDollyMotionSpeed, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraPanSpeed(), FloatNear(initialCameraPanSpeed, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraRotateSmoothness(), FloatNear(initialCameraRotateSmoothness, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraRotateSmoothingEnabled(), Eq(initialCameraRotateSmoothingEnabled));
        EXPECT_THAT(SandboxEditor::CameraTranslateSmoothness(), FloatNear(initialCameraTranslateSmoothness, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraTranslateSmoothingEnabled(), Eq(initialCameraTranslateSmoothingEnabled));
        EXPECT_THAT(SandboxEditor::CameraCaptureCursorForLook(), Eq(initialCameraCaptureCursorForLook));
        EXPECT_THAT(SandboxEditor::CameraOrbitYawRotationInverted(), Eq(initialCameraOrbitYawRotationInverted));
        EXPECT_THAT(SandboxEditor::CameraPanInvertedX(), Eq(initialCameraPanInvertedX));
        EXPECT_THAT(SandboxEditor::CameraPanInvertedY(), Eq(initialCameraPanInvertedY));
        EXPECT_THAT(SandboxEditor::CameraDefaultEditorPosition(), IsClose(initialCameraDefaultEditorPosition));
        EXPECT_THAT(SandboxEditor::CameraDefaultOrbitDistance(), FloatNear(initialCameraDefaultOrbitDistance, AZ::Constants::FloatEpsilon));
        EXPECT_THAT(SandboxEditor::CameraDefaultEditorOrientation(), IsClose(initialCameraDefaultEditorOrientation));
        EXPECT_THAT(SandboxEditor::CameraGoToPositionInstantlyEnabled(), Eq(initialCameraGoToPositionInstantly));
        EXPECT_THAT(SandboxEditor::CameraGoToPositionDuration(), FloatNear(initialCameraGoToPositionDuration, AZ::Constants::FloatEpsilon));
    }

    TEST_F(EditorCameraFixture, CameraSettingsRegistryInputValuesCanBeReset)
    {
        using ::testing::Eq;
        using ::testing::Ne;

        // reset all relevant settings to defaults
        SandboxEditor::ResetCameraTranslateForwardChannelId();
        SandboxEditor::ResetCameraTranslateBackwardChannelId();
        SandboxEditor::ResetCameraTranslateLeftChannelId();
        SandboxEditor::ResetCameraTranslateRightChannelId();
        SandboxEditor::ResetCameraTranslateUpChannelId();
        SandboxEditor::ResetCameraTranslateDownChannelId();
        SandboxEditor::ResetCameraTranslateBoostChannelId();
        SandboxEditor::ResetCameraOrbitChannelId();
        SandboxEditor::ResetCameraFreeLookChannelId();
        SandboxEditor::ResetCameraFreePanChannelId();
        SandboxEditor::ResetCameraOrbitLookChannelId();
        SandboxEditor::ResetCameraOrbitDollyChannelId();
        SandboxEditor::ResetCameraOrbitPanChannelId();
        SandboxEditor::ResetCameraFocusChannelId();

        // store defaults
        auto initialCameraTranslateForwardChannelId = SandboxEditor::CameraTranslateForwardChannelId();
        auto initialCameraTranslateBackwardChannelId = SandboxEditor::CameraTranslateBackwardChannelId();
        auto initialCameraTranslateLeftChannelId = SandboxEditor::CameraTranslateLeftChannelId();
        auto initialCameraTranslateRightChannelId = SandboxEditor::CameraTranslateRightChannelId();
        auto initialCameraTranslateUpChannelId = SandboxEditor::CameraTranslateUpChannelId();
        auto initialCameraTranslateDownChannelId = SandboxEditor::CameraTranslateDownChannelId();
        auto initialCameraTranslateBoostChannelId = SandboxEditor::CameraTranslateBoostChannelId();
        auto initialCameraOrbitChannelId = SandboxEditor::CameraOrbitChannelId();
        auto initialCameraFreeLookChannelId = SandboxEditor::CameraFreeLookChannelId();
        auto initialCameraFreePanChannelId = SandboxEditor::CameraFreePanChannelId();
        auto initialCameraOrbitLookChannelId = SandboxEditor::CameraOrbitLookChannelId();
        auto initialCameraOrbitDollyChannelId = SandboxEditor::CameraOrbitDollyChannelId();
        auto initialCameraOrbitPanChannelId = SandboxEditor::CameraOrbitPanChannelId();
        auto initialCameraFocusChannelId = SandboxEditor::CameraFocusChannelId();

        // modify all values to be different to default value
        SandboxEditor::SetCameraTranslateForwardChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraTranslateBackwardChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraTranslateLeftChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraTranslateRightChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraTranslateUpChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraTranslateDownChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraTranslateBoostChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraOrbitChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraFreeLookChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraFreePanChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraOrbitLookChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraOrbitDollyChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraOrbitPanChannelId(AZStd::string("keyboard_key_alphanumeric_0"));
        SandboxEditor::SetCameraFocusChannelId(AZStd::string("keyboard_key_alphanumeric_0"));

        // ensure all values have changed vs defaults
        EXPECT_THAT(SandboxEditor::CameraTranslateForwardChannelId(), Ne(initialCameraTranslateForwardChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateBackwardChannelId(), Ne(initialCameraTranslateBackwardChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateLeftChannelId(), Ne(initialCameraTranslateLeftChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateRightChannelId(), Ne(initialCameraTranslateRightChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateUpChannelId(), Ne(initialCameraTranslateUpChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateDownChannelId(), Ne(initialCameraTranslateDownChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateBoostChannelId(), Ne(initialCameraTranslateBoostChannelId));
        EXPECT_THAT(SandboxEditor::CameraOrbitChannelId(), Ne(initialCameraOrbitChannelId));
        EXPECT_THAT(SandboxEditor::CameraFreeLookChannelId(), Ne(initialCameraFreeLookChannelId));
        EXPECT_THAT(SandboxEditor::CameraFreePanChannelId(), Ne(initialCameraFreePanChannelId));
        EXPECT_THAT(SandboxEditor::CameraOrbitLookChannelId(), Ne(initialCameraOrbitLookChannelId));
        EXPECT_THAT(SandboxEditor::CameraOrbitDollyChannelId(), Ne(initialCameraOrbitDollyChannelId));
        EXPECT_THAT(SandboxEditor::CameraOrbitPanChannelId(), Ne(initialCameraOrbitPanChannelId));
        EXPECT_THAT(SandboxEditor::CameraFocusChannelId(), Ne(initialCameraFocusChannelId));

        // reset all relevant settings to defaults again
        SandboxEditor::ResetCameraTranslateForwardChannelId();
        SandboxEditor::ResetCameraTranslateBackwardChannelId();
        SandboxEditor::ResetCameraTranslateLeftChannelId();
        SandboxEditor::ResetCameraTranslateRightChannelId();
        SandboxEditor::ResetCameraTranslateUpChannelId();
        SandboxEditor::ResetCameraTranslateDownChannelId();
        SandboxEditor::ResetCameraTranslateBoostChannelId();
        SandboxEditor::ResetCameraOrbitChannelId();
        SandboxEditor::ResetCameraFreeLookChannelId();
        SandboxEditor::ResetCameraFreePanChannelId();
        SandboxEditor::ResetCameraOrbitLookChannelId();
        SandboxEditor::ResetCameraOrbitDollyChannelId();
        SandboxEditor::ResetCameraOrbitPanChannelId();
        SandboxEditor::ResetCameraFocusChannelId();

        // ensure values have been reset to defaults
        EXPECT_THAT(SandboxEditor::CameraTranslateForwardChannelId(), Eq(initialCameraTranslateForwardChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateBackwardChannelId(), Eq(initialCameraTranslateBackwardChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateLeftChannelId(), Eq(initialCameraTranslateLeftChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateRightChannelId(), Eq(initialCameraTranslateRightChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateUpChannelId(), Eq(initialCameraTranslateUpChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateDownChannelId(), Eq(initialCameraTranslateDownChannelId));
        EXPECT_THAT(SandboxEditor::CameraTranslateBoostChannelId(), Eq(initialCameraTranslateBoostChannelId));
        EXPECT_THAT(SandboxEditor::CameraOrbitChannelId(), Eq(initialCameraOrbitChannelId));
        EXPECT_THAT(SandboxEditor::CameraFreeLookChannelId(), Eq(initialCameraFreeLookChannelId));
        EXPECT_THAT(SandboxEditor::CameraFreePanChannelId(), Eq(initialCameraFreePanChannelId));
        EXPECT_THAT(SandboxEditor::CameraOrbitLookChannelId(), Eq(initialCameraOrbitLookChannelId));
        EXPECT_THAT(SandboxEditor::CameraOrbitDollyChannelId(), Eq(initialCameraOrbitDollyChannelId));
        EXPECT_THAT(SandboxEditor::CameraOrbitPanChannelId(), Eq(initialCameraOrbitPanChannelId));
        EXPECT_THAT(SandboxEditor::CameraFocusChannelId(), Eq(initialCameraFocusChannelId));
    }

    static constexpr float CameraTransformTolerance = 0.001f;

    TEST(EditorViewportCamera, CalculateGoToEntityTransformHandlesNormalCaseLookingForward)
    {
        const AZStd::optional<AZ::Transform> nextCameraTransform = SandboxEditor::CalculateGoToEntityTransform(
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, -5.0f, 0.5f)), AZ::DegToRad(60.0f), AZ::Vector3::CreateAxisZ(0.5f), 0.866f);

        EXPECT_THAT(
            *nextCameraTransform,
            IsCloseTolerance(
                AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, -1.87499f, 0.5f)),
                CameraTransformTolerance));
    }

    // camera starts over look down point, looking forwards
    TEST(EditorViewportCamera, CalculateGoToEntityTransformHandlesDegenerateCaseLookingDown)
    {
        const AZStd::optional<AZ::Transform> nextCameraTransform = SandboxEditor::CalculateGoToEntityTransform(
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 3.0f)), AZ::DegToRad(60.0f), AZ::Vector3::CreateAxisZ(0.5f), 0.866f);

        const auto expectedTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(-0.675590217f, 0.0f, 0.0f, 0.737277329f), AZ::Vector3(0.0f, -0.163416f, 2.367864f));

        EXPECT_THAT(nextCameraTransform->GetTranslation(), IsCloseTolerance(expectedTransform.GetTranslation(), CameraTransformTolerance));
        EXPECT_THAT(
            nextCameraTransform->GetUniformScale(), ::testing::FloatNear(expectedTransform.GetUniformScale(), CameraTransformTolerance));
        // it's possible for either rotation to be valid - both represent the same rotation/orientation
        EXPECT_THAT(
            nextCameraTransform->GetRotation(),
            ::testing::AnyOf(
                IsCloseTolerance(expectedTransform.GetRotation(), CameraTransformTolerance),
                IsCloseTolerance(-expectedTransform.GetRotation(), CameraTransformTolerance)));
    }

    // camera starts under look down point, looking up
    TEST(EditorViewportCamera, CalculateGoToEntityTransformHandlesDegenerateCaseLookingUp)
    {
        const AZStd::optional<AZ::Transform> nextCameraTransform = SandboxEditor::CalculateGoToEntityTransform(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationZ(AZ::DegToRad(-90.0f)) *
                    AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f) - AzFramework::CameraPitchTolerance),
                AZ::Vector3(0.0f, 0.0f, 5.0f)),
            AZ::DegToRad(60.0f),
            AZ::Vector3::CreateAxisZ(10.0f),
            0.866f);

        const auto expectedTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(-0.477643281f, 0.477785647f, 0.521411479f, -0.521256149f), AZ::Vector3(-0.163416952f, 0.0f, 8.13213539f));

        EXPECT_THAT(nextCameraTransform->GetTranslation(), IsCloseTolerance(expectedTransform.GetTranslation(), CameraTransformTolerance));
        EXPECT_THAT(
            nextCameraTransform->GetUniformScale(), ::testing::FloatNear(expectedTransform.GetUniformScale(), CameraTransformTolerance));
        // it's possible for either rotation to be valid - both represent the same rotation/orientation
        EXPECT_THAT(
            nextCameraTransform->GetRotation(),
            ::testing::AnyOf(
                IsCloseTolerance(expectedTransform.GetRotation(), CameraTransformTolerance),
                IsCloseTolerance(-expectedTransform.GetRotation(), CameraTransformTolerance)));
    }
} // namespace UnitTest
