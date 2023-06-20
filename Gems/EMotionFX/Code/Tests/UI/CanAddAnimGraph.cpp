/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QPushButton>
#include <QAction>
#include <QtTest>
#include <qtoolbar.h>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanAddAnimGraph)
    {
        RecordProperty("test_case_id", "C953542");

        EMStudio::GetMainWindow()->ApplicationModeChanged("AnimGraph");

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";
        ASSERT_FALSE(animGraphPlugin->GetActiveAnimGraph()) << "No anim graph should be activated.";
        ASSERT_EQ(0, EMotionFX::GetAnimGraphManager().GetNumAnimGraphs()) << "Anim graph manager should contain 0 anim graph.";

        auto toolBar = animGraphPlugin->GetViewWidget()->findChild<QToolBar*>("EMFX.BlendGraphViewWidget.TopToolBar");
        QWidget* addAnimGraphButton = UIFixture::GetWidgetFromToolbar(toolBar, "Create a new anim graph");
        ASSERT_NE(addAnimGraphButton, nullptr) << "Add Animgraph button was not found";
        QTest::mouseClick(addAnimGraphButton, Qt::LeftButton);

        AnimGraph* newGraph = animGraphPlugin->GetActiveAnimGraph();
        // The empty graph should contain one node (the root statemachine).
        ASSERT_TRUE(newGraph && newGraph->GetNumNodes() == 1) << "An empty anim graph should be activated.";
        ASSERT_EQ(1, EMotionFX::GetAnimGraphManager().GetNumAnimGraphs()) << "Anim graph manager should contain 1 anim graph.";

        QTest::mouseClick(addAnimGraphButton, Qt::LeftButton);
        ASSERT_EQ(2, EMotionFX::GetAnimGraphManager().GetNumAnimGraphs()) << "Anim graph manager should contain 2 anim graphs.";
        AnimGraph* newGraph2 = animGraphPlugin->GetActiveAnimGraph();
        ASSERT_NE(newGraph, newGraph2) << "After the second click, the active graph should change.";

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
} // namespace EMotionFX
