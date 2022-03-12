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
    class IndirectCallViewportInteractionIntersectionFixture : public ToolsApplicationFixture
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
        m_cameraState.m_viewportSize = AZ::Vector2(1280.0f, 720.0f);
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
        const auto worldIntersectionPoint = AzToolsFramework::FindClosestPickIntersection(
            m_viewportManipulatorInteraction->GetViewportInteraction().GetViewportId(), screenPosition,
            AzToolsFramework::EditorPickRayLength, AzToolsFramework::GetDefaultEntityPlacementDistance());

        EXPECT_THAT(worldIntersectionPoint, IsCloseTolerance(expectedWorldPosition, 0.01f));
    }
} // namespace UnitTest
