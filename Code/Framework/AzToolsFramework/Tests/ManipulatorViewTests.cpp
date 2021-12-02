/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#pragma optimize("", off)
#pragma inline_depth(0)

namespace UnitTest
{
    using namespace AzToolsFramework;

    class ManipulatorViewTest : public AllocatorsTestFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;

    public:
        void SetUp() override
        {
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_app.Start(AzFramework::Application::Descriptor());
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
            m_serializeContext.reset();
        }

        ToolsTestApplication m_app{ "ManipulatorViewTest" };
    };

    TEST_F(ManipulatorViewTest, ViewDirectionForCameraAlignedManipulatorFacesCameraInManipulatorSpace)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const AZ::Transform orientation =
            AZ::Transform::CreateFromQuaternion(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::DegToRad(-90.0f)));

        const AZ::Transform translation = AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 0.0f, 10.0f));

        const AZ::Transform manipulatorSpace = translation * orientation;
        // create a rotation manipulator in an arbitrary space
        RotationManipulators rotationManipulators(manipulatorSpace);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const AZ::Vector3 worldCameraPosition = AZ::Vector3(5.0f, -10.0f, 10.0f);
        // transform the view direction to the space of the manipulator (space + local transform)
        const AZ::Vector3 viewDirection = CalculateViewDirection(rotationManipulators, worldCameraPosition);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        // check the view direction is in the same space as the manipulator (space + local transform)
        EXPECT_THAT(viewDirection, IsClose(AZ::Vector3::CreateAxisZ()));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST(Manipulator, ScaleBasedOnCameraDistanceInFront)
    {
        AzFramework::CameraState cameraState{};
        cameraState.m_position = AZ::Vector3::CreateAxisY(20.0f);
        cameraState.m_forward = -AZ::Vector3::CreateAxisY();

        const float scale = AzToolsFramework::CalculateScreenToWorldMultiplier(AZ::Vector3::CreateZero(), cameraState);

        EXPECT_NEAR(scale, 2.0f, std::numeric_limits<float>::epsilon());
    }

    TEST(Manipulator, ScaleBasedOnCameraDistanceToTheSide)
    {
        AzFramework::CameraState cameraState{};
        cameraState.m_position = AZ::Vector3::CreateAxisY(20.0f);
        cameraState.m_forward = -AZ::Vector3::CreateAxisY();

        const float scale = AzToolsFramework::CalculateScreenToWorldMultiplier(AZ::Vector3::CreateAxisX(-10.0f), cameraState);

        EXPECT_NEAR(scale, 2.0f, std::numeric_limits<float>::epsilon());
    }

    TEST_F(ManipulatorViewTest, SomeTest)
    {
        auto testDebugDisplayRequests = AZStd::make_shared<TestDebugDisplayRequests>();

        // create the direct call manipulator viewport interaction and an immediate mode dispatcher
        AZStd::unique_ptr<AzManipulatorTestFramework::ManipulatorViewportInteraction> viewportManipulatorInteraction =
            AZStd::make_unique<AzManipulatorTestFramework::DirectCallManipulatorViewportInteraction>(testDebugDisplayRequests);
        AZStd::unique_ptr<AzManipulatorTestFramework::ImmediateModeActionDispatcher> actionDispatcher =
            AZStd::make_unique<AzManipulatorTestFramework::ImmediateModeActionDispatcher>(*viewportManipulatorInteraction);

        auto planarTranslationViewQuad = CreateManipulatorViewQuadForPlanarTranslationManipulator(
            AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Color::CreateZero(), AZ::Color::CreateZero(), 2.0f, 0.28f, 0.6f);

        const AZ::Transform space =
            AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, -3.0f, -4.0f)) * AZ::Transform::CreateUniformScale(2.0f);
        const AZ::Vector3 localPosition = AZ::Vector3(2.0f, -2.0f, 0.0f);
        const AZ::Vector3 nonUniformScale = AZ::Vector3(2.0f, 3.0f, 4.0f);

        const AZ::Transform combinedTransform =
            AzToolsFramework::ApplySpace(AZ::Transform::CreateTranslation(localPosition), space, nonUniformScale);

        AzToolsFramework::ManipulatorState manipulatorState{};
        manipulatorState.m_worldFromLocal = combinedTransform;
        manipulatorState.m_nonUniformScale = nonUniformScale;
        manipulatorState.m_localPosition = AZ::Vector3::CreateZero();

        // camera 2.50, -8.00, 22.00, -90.00, 0.00
        const AzFramework::CameraState cameraState = AzFramework::CreateDefaultCamera(
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-90.0f)), AZ::Vector3(2.5f, -8.0f, 22.0f)),
            AZ::Vector2(1280, 720));

        planarTranslationViewQuad->Draw(
            viewportManipulatorInteraction->GetManipulatorManagerId(), AzToolsFramework::ManipulatorManagerState{ false },
            AzToolsFramework::ManipulatorId(1), manipulatorState, *testDebugDisplayRequests, cameraState,
            AzToolsFramework::ViewportInteraction::MouseInteraction{});

        auto points = testDebugDisplayRequests->GetPoints();

        int i;
        i = 0;
    }
} // namespace UnitTest

#pragma optimize("", on)
#pragma inline_depth()
