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

#include <gtest/gtest.h>

#include <Tests/UI/UIFixture.h>
#include <Tests/Matchers.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <QApplication>
#include <QComboBox>


namespace EMotionFX
{
    class AddGroupFixture
        : public UIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();

            // Create empty anim graph and select it.
            const AZ::u32 animGraphId = 1;
            const AZStd::string command = AZStd::string::format("CreateAnimGraph -animGraphID %d", animGraphId);
            AZStd::string commandResult;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, commandResult)) << commandResult.c_str();
            m_animGraph = GetAnimGraphManager().FindAnimGraphByID(animGraphId);
            EXPECT_NE(m_animGraph, nullptr) << "Cannot find the newly created anim graph.";
        }

        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            delete m_animGraph;
            UIFixture::TearDown();
        }

    public:
        AnimGraph* m_animGraph = nullptr;
    };

    TEST_F(AddGroupFixture, AddGroupParameter)
    {
        RecordProperty("test_case_id", "C5506441");

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";

        // Trigger the window that will let us set a name for the group to add.
        EMStudio::ParameterWindow* parameterWindow = animGraphPlugin->GetParameterWindow();
        ASSERT_TRUE(parameterWindow) << "Anim graph parameter window is invalid.";
        parameterWindow->OnAddGroup();

        // Grab a pointer to that dialog that pops up and accept it (basically click the Ok button).
        QWidget* groupCreateWidget = FindTopLevelWidget("EMFX.ParameterCreateRenameDialog");
        ASSERT_NE(groupCreateWidget, nullptr) << "Cannot find anim graph group create/rename dialog. Is the anim graph selected?";
        auto groupCreateWindow = qobject_cast<EMStudio::ParameterCreateRenameWindow*>(groupCreateWidget);
        groupCreateWindow->accept();
        
        // Verify that we have the group inside the anim graph.
        ASSERT_EQ(m_animGraph->GetNumParameters(), 1) << "Group creation failed. We shouldn't have one parameter in the animgraph.";
        EXPECT_EQ(m_animGraph->GetNumValueParameters(), 0) << "Expecting no value parameters as we only created a group.";
        const Parameter* parameter = m_animGraph->FindParameter(0);
        EXPECT_NE(parameter, nullptr) << "The parameter we created should be valid.";
        EXPECT_THAT(parameter->GetName(), StrEq("Group0"));
        EXPECT_EQ(parameter->RTTI_GetType(), azrtti_typeid<GroupParameter>()) << "The type of the created parameter isn't a group.";
    }
} // namespace EMotionFX
