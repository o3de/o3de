/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    // sets up a parent widget and render overlay to attach the Viewport UI to
    // as well as a cluster with one button
    class ViewportUiManagerTestFixture : public ::testing::Test
    {
    public:
        ViewportUiManagerTestFixture() = default;

        ViewportManagerWrapper m_viewportManagerWrapper;

        void SetUp() override
        {
            m_viewportManagerWrapper.Create();
        }

        void TearDown() override
        {
            m_viewportManagerWrapper.Destroy();
        }
    };

    TEST_F(ViewportUiManagerTestFixture, CreateClusterAddsNewClusterAndReturnsId)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);

        EXPECT_TRUE(clusterEntry != m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().end());
        EXPECT_TRUE(clusterEntry->second.get() != nullptr);
    }

    TEST_F(ViewportUiManagerTestFixture, CreateClusterButtonAddsNewButtonAndReturnsId)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);

        EXPECT_TRUE(clusterEntry->second->GetButton(buttonId) != nullptr);
    }

    TEST_F(ViewportUiManagerTestFixture, SetClusterActiveButtonSetsButtonStateToActive)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);
        auto button = clusterEntry->second->GetButton(buttonId);

        m_viewportManagerWrapper.GetViewportManager()->SetClusterActiveButton(clusterId, buttonId);

        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Selected);
    }

    TEST_F(ViewportUiManagerTestFixture, ClearClusterActiveButtonSetsButtonStateToDeselected)
    {
        // setup
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);
        auto button = clusterEntry->second->GetButton(buttonId);

        // first set a button to active
        m_viewportManagerWrapper.GetViewportManager()->SetClusterActiveButton(clusterId, buttonId);
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Selected);

        // clear the active button on the cluster
        m_viewportManagerWrapper.GetViewportManager()->ClearClusterActiveButton(clusterId);
        // the button should now be deselected
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Deselected);
    }

    TEST_F(ViewportUiManagerTestFixture, SetClusterDisableButtonOnActiveButton)
    {
        // setup
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);
        auto button = clusterEntry->second->GetButton(buttonId);

        // first set a button to active
        m_viewportManagerWrapper.GetViewportManager()->SetClusterActiveButton(clusterId, buttonId);
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Selected);

        // Disable active button
        m_viewportManagerWrapper.GetViewportManager()->SetClusterDisableButton(clusterId, buttonId, true);
        // try clear active button
        m_viewportManagerWrapper.GetViewportManager()->ClearClusterActiveButton(clusterId);

        // the button should now be disabled
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Disabled);
    }

    TEST_F(ViewportUiManagerTestFixture, SetClusterDisableButton)
    {
        // setup
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");
        auto buttonId2 = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);
        auto button = clusterEntry->second->GetButton(buttonId);
        auto button2 = clusterEntry->second->GetButton(buttonId2);

        // the button should now be disable
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Deselected);
        EXPECT_TRUE(button2->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Deselected);

        m_viewportManagerWrapper.GetViewportManager()->SetClusterDisableButton(clusterId, buttonId, true);
        m_viewportManagerWrapper.GetViewportManager()->SetClusterDisableButton(clusterId, buttonId2, true);

        // the button should now be disabled
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Disabled);
        EXPECT_TRUE(button2->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Disabled);
    }

    TEST_F(ViewportUiManagerTestFixture, RegisterClusterEventHandlerConnectsHandlerToClusterEvent)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        // create a handler which will be triggered by the cluster
        bool handlerTriggered = false;
        auto testButtonId = AzToolsFramework::ViewportUi::ButtonId(buttonId);
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler handler(
            [&handlerTriggered, testButtonId](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                if (buttonId == testButtonId)
                {
                    handlerTriggered = true;
                }
            });

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);

        // trigger the cluster
        m_viewportManagerWrapper.GetViewportManager()->RegisterClusterEventHandler(clusterId, handler);
        clusterEntry->second->PressButton(buttonId);

        EXPECT_TRUE(handlerTriggered);
    }

    TEST_F(ViewportUiManagerTestFixture, RemoveClusterRemovesClusterFromViewportUi)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        m_viewportManagerWrapper.GetViewportManager()->RemoveCluster(clusterId);

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);

        EXPECT_TRUE(clusterEntry == m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().end());
    }

    TEST_F(ViewportUiManagerTestFixture, SetClusterVisibleChangesClusterVisibility)
    {
        m_viewportManagerWrapper.GetMockRenderOverlay()->setVisible(true);

        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");
        m_viewportManagerWrapper.GetViewportManager()->Update();

        m_viewportManagerWrapper.GetViewportManager()->SetClusterVisible(clusterId, false);
        auto cluster = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId)->second;

        bool visible =
            m_viewportManagerWrapper.GetViewportManager()->GetViewportUiDisplay()->IsViewportUiElementVisible(cluster->GetViewportUiElementId());
        EXPECT_FALSE(visible);

        m_viewportManagerWrapper.GetViewportManager()->SetClusterVisible(clusterId, true);
        visible =
            m_viewportManagerWrapper.GetViewportManager()->GetViewportUiDisplay()->IsViewportUiElementVisible(cluster->GetViewportUiElementId());
        EXPECT_TRUE(visible);
    }
} // namespace UnitTest
