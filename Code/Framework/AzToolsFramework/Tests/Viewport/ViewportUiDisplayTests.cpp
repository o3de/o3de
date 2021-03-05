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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace UnitTest
{
    using ViewportUiDisplay = AzToolsFramework::ViewportUi::Internal::ViewportUiDisplay;
    using ViewportUiElementId = AzToolsFramework::ViewportUi::ViewportUiElementId;
    using Cluster = AzToolsFramework::ViewportUi::Internal::Cluster;

    // sets up a parent widget and render overlay to attach the Viewport UI to
    // as well as a cluster with one button
    class ViewportUiDisplayTestFixture : public ::testing::Test
    {
    public:
        ViewportUiDisplayTestFixture() = default;

        void SetUp()
        {
            m_cluster = AZStd::make_shared<Cluster>();
            m_cluster->AddButton("");
            m_parentWidget = new QWidget();
            m_mockRenderOverlay = new QWidget();
        }

        void TearDown()
        {
            m_cluster.reset();
            delete m_parentWidget;
            delete m_mockRenderOverlay;
        }

        QWidget* m_parentWidget = nullptr;
        QWidget* m_mockRenderOverlay = nullptr;
        AZStd::shared_ptr<Cluster> m_cluster = nullptr;
    };

    TEST_F(ViewportUiDisplayTestFixture, ViewportUiInitializationReturnsProperlyParentedWidgets)
    {
        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);

        EXPECT_TRUE(viewportUi.GetUiMainWindow()->parent() == m_parentWidget);
        EXPECT_TRUE(viewportUi.GetUiOverlay()->parent() == m_parentWidget);
    }

    TEST_F(ViewportUiDisplayTestFixture, InitializeUiOverlaySetsViewportUiVisibilityToFalse)
    {
        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();

        EXPECT_FALSE(viewportUi.GetUiMainWindow()->isVisible());
        EXPECT_FALSE(viewportUi.GetUiOverlay()->isVisible());
    }

    TEST_F(ViewportUiDisplayTestFixture, RemoveViewportUiElementRemovesElementFromViewportUi)
    {
        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.AddCluster(m_cluster);
        auto widget = viewportUi.GetViewportUiElement(m_cluster->GetViewportUiElementId());

        EXPECT_TRUE(widget.get() != nullptr);

        viewportUi.RemoveViewportUiElement(m_cluster->GetViewportUiElementId());
        widget = viewportUi.GetViewportUiElement(m_cluster->GetViewportUiElementId());

        EXPECT_TRUE(widget.get() == nullptr);
    }

    TEST_F(ViewportUiDisplayTestFixture, ShowViewportUiElementSetsWidgetVisibilityToTrue)
    {
        m_mockRenderOverlay->setVisible(true);

        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();
        viewportUi.AddCluster(m_cluster);
        viewportUi.Update();
        viewportUi.ShowViewportUiElement(m_cluster->GetViewportUiElementId());

        EXPECT_TRUE(viewportUi.IsViewportUiElementVisible(m_cluster->GetViewportUiElementId()));
    }

    TEST_F(ViewportUiDisplayTestFixture, HideViewportUiElementSetsWidgetVisibilityToFalse)
    {
        m_mockRenderOverlay->setVisible(true);

        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();
        viewportUi.AddCluster(m_cluster);
        viewportUi.HideViewportUiElement(m_cluster->GetViewportUiElementId());

        EXPECT_FALSE(viewportUi.IsViewportUiElementVisible(m_cluster->GetViewportUiElementId()));
    }

    TEST_F(ViewportUiDisplayTestFixture, UpdateUiOverlayGeometryChangesGeometryToMatchViewportUiElements)
    {
        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();
        viewportUi.AddCluster(m_cluster);

        viewportUi.Update();
        auto widget = viewportUi.GetViewportUiElement(m_cluster->GetViewportUiElementId());

        EXPECT_EQ(viewportUi.GetUiMainWindow()->mask(), widget->geometry());
    }

    TEST_F(ViewportUiDisplayTestFixture, UpdateSetsViewportUiInvisibleIfNoChildGeometry)
    {
        m_mockRenderOverlay->setVisible(true);

        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();

        auto cluster = AZStd::make_shared<Cluster>();
        cluster->AddButton("");
        viewportUi.AddCluster(cluster);
        viewportUi.Update();

        EXPECT_TRUE(viewportUi.GetUiMainWindow()->isVisible());

        viewportUi.RemoveViewportUiElement(cluster->GetViewportUiElementId());
        viewportUi.Update();
        EXPECT_FALSE(viewportUi.GetUiMainWindow()->isVisible());
    }

    TEST_F(ViewportUiDisplayTestFixture, UpdateSetsUiDimensionsToMatchRenderViewport)
    {
        auto geometry = QRect(25, 50, 200, 100);
        m_mockRenderOverlay->setGeometry(geometry);
        m_mockRenderOverlay->setVisible(true);

        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();

        viewportUi.Update();

        EXPECT_EQ(viewportUi.GetUiOverlay()->height(), m_mockRenderOverlay->height());
        EXPECT_EQ(viewportUi.GetUiOverlay()->width(), m_mockRenderOverlay->width());
    }

} // namespace UnitTest
