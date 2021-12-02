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

namespace UnitTest
{
    class EditorCameraTestEnvironment : public AZ::Test::GemTestEnvironment
    {
        // AZ::Test::GemTestEnvironment overrides ...
        void AddGemsAndComponents() override;
    };

    void EditorCameraTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({ CAMERA_EDITOR_MODULE });
        AddComponentDescriptors({ AzToolsFramework::Components::TransformComponent::CreateDescriptor() });
    }

    class EditorCameraFixture : public ::testing::Test
    {
    public:
        AtomToolsFramework::ModularCameraViewportContext* m_cameraViewportContextView = nullptr;
        AZStd::unique_ptr<SandboxEditor::EditorModularViewportCameraComposer> m_editorModularViewportCameraComposer;
        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_editorLibHandle;
        AzFramework::ViewportControllerListPtr m_controllerList;
        AZStd::unique_ptr<AZ::Entity> m_entity;

        static inline constexpr AzFramework::ViewportId TestViewportId = 2345;
        static inline constexpr float HalfInterpolateToTransformDuration =
            AtomToolsFramework::ModularViewportCameraControllerRequests::InterpolateToTransformDuration * 0.5f;

        void SetUp() override
        {
            m_editorLibHandle = AZ::DynamicModuleHandle::Create("EditorLib");
            [[maybe_unused]] const bool loaded = m_editorLibHandle->Load(true);
            AZ_Assert(loaded, "EditorLib could not be loaded");

            m_controllerList = AZStd::make_shared<AzFramework::ViewportControllerList>();
            m_controllerList->RegisterViewportContext(TestViewportId);

            m_entity = AZStd::make_unique<AZ::Entity>();
            m_entity->Init();
            m_entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            m_entity->Activate();

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
        }

        void TearDown() override
        {
            m_editorModularViewportCameraComposer.reset();
            m_cameraViewportContextView = nullptr;
            m_entity.reset();
            m_editorLibHandle = {};
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
            TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transformToInterpolateTo);

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
            TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transformToInterpolateTo);

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
            interpolationBegan, TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f)));

        // Then
        EXPECT_THAT(interpolationBegan, ::testing::IsTrue());
    }

    TEST_F(EditorCameraFixture, CameraInterpolationDoesNotBeginDuringAnExistingInterpolation)
    {
        // Given/When
        bool initialInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            initialInterpolationBegan, TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f)));

        m_controllerList->UpdateViewport(
            { TestViewportId, AzFramework::FloatSeconds(HalfInterpolateToTransformDuration), AZ::ScriptTimePoint() });

        bool nextInterpolationBegan = true;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            nextInterpolationBegan, TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f)));

        bool interpolating = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            interpolating, TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsInterpolating);

        // Then
        EXPECT_THAT(initialInterpolationBegan, ::testing::IsTrue());
        EXPECT_THAT(nextInterpolationBegan, ::testing::IsFalse());
        EXPECT_THAT(interpolating, ::testing::IsTrue());
    }

    TEST_F(EditorCameraFixture, CameraInterpolationCanBeginAfterAnInterpolationCompletes)
    {
        // Given/When
        bool initialInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            initialInterpolationBegan, TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f)));

        m_controllerList->UpdateViewport(
            { TestViewportId,
              AzFramework::FloatSeconds(AtomToolsFramework::ModularViewportCameraControllerRequests::InterpolateToTransformDuration + 0.5f),
              AZ::ScriptTimePoint() });

        bool interpolating = true;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            interpolating, TestViewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::IsInterpolating);

        bool nextInterpolationBegan = false;
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
            nextInterpolationBegan, TestViewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 10.0f)));

        // Then
        EXPECT_THAT(initialInterpolationBegan, ::testing::IsTrue());
        EXPECT_THAT(interpolating, ::testing::IsFalse());
        EXPECT_THAT(nextInterpolationBegan, ::testing::IsTrue());
    }
} // namespace UnitTest

// required to support running integration tests with the Camera Gem
AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({ new UnitTest::EditorCameraTestEnvironment() });
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN();
