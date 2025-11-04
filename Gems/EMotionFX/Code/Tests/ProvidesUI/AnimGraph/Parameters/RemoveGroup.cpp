/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <QTableWidget>
#include <gtest/gtest.h>
#include <Tests/UI/UIFixture.h>
#include <Tests/Matchers.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <QApplication>
#include <QComboBox>
#include <Editor/ReselectingTreeView.h>
#include <QtTest>
#include <QApplication>
#include <QWidget>
#include <QToolBar>


namespace EMotionFX
{
    class RemoveGroupFixture : public UIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();

            // Create empty anim graph and select it.
            const AZ::u32 animGraphId = 1;
            const AZStd::string command = AZStd::string::format("CreateAnimGraph -animGraphID %d", animGraphId);
            AZStd::string commandResult;
            auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
            ASSERT_TRUE(animGraphPlugin);

            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, commandResult)) << commandResult.c_str();
            m_animGraph = GetAnimGraphManager().FindAnimGraphByID(animGraphId);
            ASSERT_NE(m_animGraph, nullptr) << "Cannot find the newly created anim graph.";

            m_parameterWindow = animGraphPlugin->GetParameterWindow();
            ASSERT_TRUE(m_parameterWindow) << "Parameter window was not found";
        }

        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            delete m_animGraph;
            UIFixture::TearDown();
        }

    public:
        AnimGraph* m_animGraph = nullptr;
        EMStudio::ParameterWindow* m_parameterWindow = nullptr;
    };

    TEST_F(RemoveGroupFixture, RemoveNodeGroup)
    {
        RecordProperty("test_case_id", "C5522320");

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";

        // Verify baseline
        ASSERT_EQ(m_animGraph->GetNumParameters(), 0) << "No Parameter Groups should exist initially";
        EXPECT_EQ(m_animGraph->GetNumValueParameters(), 0) << "No parameters should exist initially";

        AZStd::string result;

        // Create Parameter Group
        AZStd::string command = AZStd::string::format("AnimGraphAddGroupParameter -animGraphID %i -name \"%s\"", m_animGraph->GetID(), "Group0");
        ASSERT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, result)) << "Parameter Group could not be created";

        // Verify Parameter Group was created
        // Verify that we have the group inside the anim graph.
        ASSERT_EQ(m_animGraph->GetNumParameters(), 1) << "Group creation failed. We shouldn't have one parameter in the animgraph.";
        EXPECT_EQ(m_animGraph->GetNumValueParameters(), 0) << "Expecting no value parameters as we only created a group.";
        const Parameter* parameter = m_animGraph->FindParameter(0);
        EXPECT_NE(parameter, nullptr) << "The parameter we created should be valid.";
        ASSERT_THAT(parameter->GetName(), StrEq("Group0")) << "Group was made with the wrong name";

        // Select and remove the parameter group.
        m_parameterWindow->SelectParameters({ "Group0" });
        m_parameterWindow->OnRemoveSelected();

        // Verify that the Parameter group was removed
        ASSERT_EQ(m_animGraph->GetNumParameters(), 0) << "No Parameter Groups should exist initially";
    }
} // namespace EMotionFX
