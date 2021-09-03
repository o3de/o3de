/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    using ButtonGroup = AzToolsFramework::ViewportUi::Internal::ButtonGroup;

    // sets up a parent widget and render overlay to attach the Viewport UI to
    // as well as a button group with one button
    class ViewportUiDisplayTestFixture : public ::testing::Test
    {
    public:
        ViewportUiDisplayTestFixture() = default;

        void SetUp() override
        {
            m_buttonGroup = AZStd::make_shared<ButtonGroup>();
            m_buttonGroup->AddButton("");
            m_parentWidget = new QWidget();
            m_mockRenderOverlay = new QWidget();
        }

        void TearDown() override
        {
            m_buttonGroup.reset();
            delete m_parentWidget;
            delete m_mockRenderOverlay;
        }

        QWidget* m_parentWidget = nullptr;
        QWidget* m_mockRenderOverlay = nullptr;
        AZStd::shared_ptr<ButtonGroup> m_buttonGroup = nullptr;
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
        viewportUi.AddCluster(m_buttonGroup, AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto widget = viewportUi.GetViewportUiElement(m_buttonGroup->GetViewportUiElementId());

        EXPECT_TRUE(widget.get() != nullptr);

        viewportUi.RemoveViewportUiElement(m_buttonGroup->GetViewportUiElementId());
        widget = viewportUi.GetViewportUiElement(m_buttonGroup->GetViewportUiElementId());

        EXPECT_TRUE(widget.get() == nullptr);
    }

    TEST_F(ViewportUiDisplayTestFixture, ShowViewportUiElementSetsWidgetVisibilityToTrue)
    {
        m_mockRenderOverlay->setVisible(true);

        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();
        viewportUi.AddCluster(m_buttonGroup, AzToolsFramework::ViewportUi::Alignment::TopLeft);
        viewportUi.Update();
        viewportUi.ShowViewportUiElement(m_buttonGroup->GetViewportUiElementId());

        EXPECT_TRUE(viewportUi.IsViewportUiElementVisible(m_buttonGroup->GetViewportUiElementId()));
    }

    TEST_F(ViewportUiDisplayTestFixture, HideViewportUiElementSetsWidgetVisibilityToFalse)
    {
        m_mockRenderOverlay->setVisible(true);

        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();
        viewportUi.AddCluster(m_buttonGroup, AzToolsFramework::ViewportUi::Alignment::TopLeft);
        viewportUi.HideViewportUiElement(m_buttonGroup->GetViewportUiElementId());

        EXPECT_FALSE(viewportUi.IsViewportUiElementVisible(m_buttonGroup->GetViewportUiElementId()));
    }

    TEST_F(ViewportUiDisplayTestFixture, UpdateUiOverlayGeometryChangesGeometryToMatchViewportUiElements)
    {
        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();
        viewportUi.AddCluster(m_buttonGroup, AzToolsFramework::ViewportUi::Alignment::TopLeft);

        viewportUi.Update();
        auto widget = viewportUi.GetViewportUiElement(m_buttonGroup->GetViewportUiElementId());

        EXPECT_EQ(viewportUi.GetUiMainWindow()->mask(), widget->geometry());
    }

    TEST_F(ViewportUiDisplayTestFixture, UpdateSetsViewportUiInvisibleIfNoChildGeometry)
    {
        m_mockRenderOverlay->setVisible(true);

        ViewportUiDisplay viewportUi(m_parentWidget, m_mockRenderOverlay);
        viewportUi.InitializeUiOverlay();

        auto buttonGroup = AZStd::make_shared<ButtonGroup>();
        buttonGroup->AddButton("");
        viewportUi.AddCluster(buttonGroup, AzToolsFramework::ViewportUi::Alignment::TopLeft);
        viewportUi.Update();

        EXPECT_TRUE(viewportUi.GetUiMainWindow()->isVisible());

        viewportUi.RemoveViewportUiElement(buttonGroup->GetViewportUiElementId());
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
