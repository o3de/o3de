/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Tests/BoundsTestComponent.h>

namespace UnitTest
{
    class IndirectCallViewportInteractionIntersectionFixture : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            auto* app = GetApplication();
            // register a simple component implementing BoundsRequestBus and EditorComponentSelectionRequestsBus
            app->RegisterComponentDescriptor(BoundsTestComponent::CreateDescriptor());
            // register a component implementing RenderGeometry::IntersectionRequestBus
            app->RegisterComponentDescriptor(RenderGeometryIntersectionTestComponent::CreateDescriptor());

            AZ::Entity* entityGround = nullptr;
            m_entityIdGround = CreateDefaultEditorEntity("EntityGround", &entityGround);

            entityGround->Deactivate();
            auto ground = entityGround->CreateComponent<RenderGeometryIntersectionTestComponent>();
            entityGround->Activate();

            ground->m_localBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-10.0f, -10.0f, -0.5f), AZ::Vector3(10.0f, 10.0f, 0.5f));
        }

        AZ::EntityId m_entityIdGround;
    };

    using IndirectCallManipulatorViewportInteractionIntersectionFixture =
        IndirectCallManipulatorViewportInteractionFixtureMixin<IndirectCallViewportInteractionIntersectionFixture>;

    TEST_F(IndirectCallManipulatorViewportInteractionIntersectionFixture, FindClosestPickIntersectionReturnsExpectedSurfacePoint)
    {
        // camera - 21.00, 8.00, 11.00, -22.00, 150.00
        m_cameraState.m_viewportSize = AzFramework::ScreenSize(1280, 720);
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(150.0f)) * AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-22.0f)),
                AZ::Vector3(21.0f, 8.0f, 11.0f)));

        m_actionDispatcher->CameraState(m_cameraState);

        AzToolsFramework::SetWorldTransform(
            m_entityIdGround,
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationY(AZ::DegToRad(40.0f)) * AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(60.0f)),
                AZ::Vector3(14.0f, -6.0f, 5.0f)));

        // expected world position (value taken from editor scenario)
        const auto expectedWorldPosition = AZ::Vector3(13.606657f, -2.6753534f, 5.9827675f);
        const auto screenPosition = AzFramework::WorldToScreen(expectedWorldPosition, m_cameraState);

        // perform ray intersection against mesh
        constexpr float defaultPickDistance = 10.0f;
        const auto worldIntersectionPoint = AzToolsFramework::FindClosestPickIntersection(
            m_viewportManipulatorInteraction->GetViewportInteraction().GetViewportId(), screenPosition,
            AzToolsFramework::EditorPickRayLength, defaultPickDistance);

        EXPECT_THAT(worldIntersectionPoint, IsCloseTolerance(expectedWorldPosition, 0.01f));

        // Verify that the overloaded version of the API that returns an optional Vector3 also detects the hit correctly.
        const auto optionalWorldIntersectionPoint = AzToolsFramework::FindClosestPickIntersection(
            m_viewportManipulatorInteraction->GetViewportInteraction().GetViewportId(),
            screenPosition,
            AzToolsFramework::EditorPickRayLength);

        EXPECT_TRUE(optionalWorldIntersectionPoint.has_value());
        EXPECT_THAT(*optionalWorldIntersectionPoint, IsCloseTolerance(expectedWorldPosition, 0.01f));
    }

    TEST_F(IndirectCallManipulatorViewportInteractionIntersectionFixture, FindClosestPickIntersectionWithNoHitReturnsExpectedResult)
    {
        const int screenSize = 1000;
        const AZ::Vector3 cameraLocation(100.0f);

        // Create a simple default camera located at (100,100,100) pointing straight up.
        m_cameraState.m_viewportSize = AzFramework::ScreenSize(screenSize, screenSize);
        AzFramework::SetCameraTransform(m_cameraState, AZ::Transform::CreateTranslation(cameraLocation));
        m_actionDispatcher->CameraState(m_cameraState);

        // Query from the center of the screen.
        const AzFramework::ScreenPoint screenPosition(screenSize / 2, screenSize / 2);

        // perform ray intersection. With no collision, it should pick a point directly pointing into the camera
        // at the default distance, starting from the camera's position + near clip plane distance.
        constexpr float defaultPickDistance = 25.0f;
        const auto expectedWorldPosition = cameraLocation + AZ::Vector3(0.0f, defaultPickDistance + m_cameraState.m_nearClip, 0.0f);

        const auto worldIntersectionPoint = AzToolsFramework::FindClosestPickIntersection(
            m_viewportManipulatorInteraction->GetViewportInteraction().GetViewportId(),
            screenPosition,
            AzToolsFramework::EditorPickRayLength,
            defaultPickDistance);

        EXPECT_THAT(worldIntersectionPoint, IsCloseTolerance(expectedWorldPosition, 0.01f));

        // Verify that the overloaded version of the API that returns an optional Vector3 doesn't find a hit.
        const auto optionalWorldIntersectionPoint = AzToolsFramework::FindClosestPickIntersection(
            m_viewportManipulatorInteraction->GetViewportInteraction().GetViewportId(),
            screenPosition,
            AzToolsFramework::EditorPickRayLength);

        EXPECT_FALSE(optionalWorldIntersectionPoint.has_value());
    }
} // namespace UnitTest
