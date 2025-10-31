/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzManipulatorTestFrameworkTestFixtures.h"
#include <AzManipulatorTestFramework/ViewportInteraction.h>

namespace UnitTest
{
    class AValidViewportInteraction : public ToolsApplicationFixture<>
    {
    public:
        AValidViewportInteraction()
            : m_viewportInteraction(
                  AZStd::make_unique<AzManipulatorTestFramework::ViewportInteraction>(AZStd::make_shared<NullDebugDisplayRequests>()))
        {
        }

    protected:
        void SetUpEditorFixtureImpl() override
        {
            m_cameraState = AzFramework::CreateIdentityDefaultCamera(AZ::Vector3::CreateZero(), AzFramework::ScreenSize(800, 600));
        }

    public:
        AZStd::unique_ptr<AzManipulatorTestFramework::ViewportInteraction> m_viewportInteraction;
        AzFramework::CameraState m_cameraState;
    };

    TEST_F(AValidViewportInteraction, HasViewportId1234)
    {
        EXPECT_EQ(m_viewportInteraction->GetViewportId(), 1234);
    }

    TEST_F(AValidViewportInteraction, CanSetAndGetCameraState)
    {
        m_viewportInteraction->SetCameraState(m_cameraState);
        const auto cameraState = m_viewportInteraction->GetCameraState();

        EXPECT_EQ(cameraState.m_position, m_cameraState.m_position);
        EXPECT_EQ(cameraState.m_forward, m_cameraState.m_forward);
    }

    TEST_F(AValidViewportInteraction, CanEnableGridSnapping)
    {
        bool snapping = false;

        m_viewportInteraction->SetGridSnapping(true);
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            snapping, m_viewportInteraction->GetViewportId(),
            &AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Events::GridSnappingEnabled);

        EXPECT_TRUE(snapping);
    }

    TEST_F(AValidViewportInteraction, CanDisableGridSnapping)
    {
        bool snapping = true;

        m_viewportInteraction->SetGridSnapping(false);
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            snapping, m_viewportInteraction->GetViewportId(),
            &AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Events::GridSnappingEnabled);

        EXPECT_FALSE(snapping);
    }

    TEST_F(AValidViewportInteraction, CanGetAndSetGridSize)
    {
        float gridSize = 0.0f;
        const float expectedGridSize = 50.0f;

        m_viewportInteraction->SetGridSize(expectedGridSize);

        m_viewportInteraction->SetGridSnapping(false);
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            gridSize, m_viewportInteraction->GetViewportId(),
            &AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Events::GridSize);

        EXPECT_EQ(gridSize, expectedGridSize);
    }

    TEST_F(AValidViewportInteraction, CanEnableAngularSnapping)
    {
        bool snapping = false;

        m_viewportInteraction->SetAngularSnapping(true);
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            snapping, m_viewportInteraction->GetViewportId(),
            &AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Events::AngleSnappingEnabled);

        EXPECT_TRUE(snapping);
    }

    TEST_F(AValidViewportInteraction, CanDisableAngularSnapping)
    {
        bool snapping = true;

        m_viewportInteraction->SetAngularSnapping(false);
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            snapping, m_viewportInteraction->GetViewportId(),
            &AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Events::AngleSnappingEnabled);

        EXPECT_FALSE(snapping);
    }

    TEST_F(AValidViewportInteraction, CanGetAndSetAngleStep)
    {
        float angularStep = 0.0f;
        const float expectedAngularStep = 50.0f;

        m_viewportInteraction->SetAngularStep(expectedAngularStep);

        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            angularStep, m_viewportInteraction->GetViewportId(),
            &AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Events::AngleStep);

        EXPECT_EQ(angularStep, expectedAngularStep);
    }

    TEST_F(AValidViewportInteraction, HasAValidGetDebugDisplay)
    {
        AzFramework::DebugDisplayRequests& debugDisplay = m_viewportInteraction->GetDebugDisplay();
        EXPECT_NE(nullptr, &debugDisplay);
    }
} // namespace UnitTest
