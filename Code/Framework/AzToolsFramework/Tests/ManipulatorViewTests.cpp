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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace UnitTest
{
    using namespace AzToolsFramework;

    class ManipulatorViewTest
        : public AllocatorsTestFixture
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

        ToolsApplication m_app;
    };

    TEST_F(ManipulatorViewTest, ViewDirectionForCameraAlignedManipulatorFacesCameraInManipulatorSpace)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const AZ::Transform orientation =
            AZ::Transform::CreateFromQuaternion(
                AZ::Quaternion::CreateFromAxisAngle(
                    AZ::Vector3::CreateAxisX(), AZ::DegToRad(-90.0f)));

        const AZ::Transform translation =
            AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 0.0f, 10.0f));

        const AZ::Transform manipulatorSpace = translation * orientation;
        // create a rotation manipulator in an arbitrary space
        RotationManipulators rotationManipulators(manipulatorSpace);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const AZ::Vector3 worldCameraPosition = AZ::Vector3(5.0f, -10.0f, 10.0f);
        // transform the view direction to the space of the manipulator (space + local transform)
        const AZ::Vector3 viewDirection =
            CalculateViewDirection(rotationManipulators, worldCameraPosition);
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

        const float scale =
            AzToolsFramework::CalculateScreenToWorldMultiplier(AZ::Vector3::CreateZero(), cameraState);

        EXPECT_NEAR(scale, 2.0f, std::numeric_limits<float>::epsilon());
    }

    TEST(Manipulator, ScaleBasedOnCameraDistanceToTheSide)
    {
        AzFramework::CameraState cameraState{};
        cameraState.m_position = AZ::Vector3::CreateAxisY(20.0f);
        cameraState.m_forward = -AZ::Vector3::CreateAxisY();

        const float scale =
            AzToolsFramework::CalculateScreenToWorldMultiplier(AZ::Vector3::CreateAxisX(-10.0f), cameraState);

        EXPECT_NEAR(scale, 2.0f, std::numeric_limits<float>::epsilon());
    }
} // namespace UnitTest
