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

#include <EMotionFX/CommandSystem/Source/AnimGraphNodeGroupCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    TEST_F(AnimGraphFixture, AnimGraphAddMassNodeGroupTests)
    {
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup("Add anim graph node groups");
        AZStd::string result;
        AZStd::string commandString;

        for (AZ::u32 i = 0; i < 100; ++i)
        {
            commandString = AZStd::string::format("AnimGraphAddNodeGroup -animGraphID %i -name \"NodeGroup%s\"", m_animGraph->GetID(), AZStd::to_string(i).c_str());
            commandGroup.AddCommandString(commandString);
        }

        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(m_animGraph->GetNumNodeGroups(), 100);

        EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(result));
        EXPECT_EQ(m_animGraph->GetNumNodeGroups(), 0);

        EXPECT_TRUE(CommandSystem::GetCommandManager()->Redo(result));
        EXPECT_EQ(m_animGraph->GetNumNodeGroups(), 100);
    }


    TEST_F(AnimGraphFixture, AnimGraphRemoveMassNodeGroupTests)
    {
        AZStd::string nodeGroupName;
        for (AZ::u32 i = 0; i < 100; ++i)
        {
            EMotionFX::AnimGraphNodeGroup* nodeGroup = aznew AnimGraphNodeGroup();
            nodeGroupName = AZStd::string::format("NodeGroup%s", AZStd::to_string(i).c_str());
            nodeGroup->SetName(nodeGroupName.c_str());
            m_animGraph->AddNodeGroup(nodeGroup);
        }
        EXPECT_EQ(m_animGraph->GetNumNodeGroups(), 100);

        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup("Remove anim graph node groups");
        AZStd::string result;
        AZStd::string commandString;

        for (AZ::u32 i = 0; i < 100; ++i)
        {
            commandString = AZStd::string::format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"NodeGroup%s\"", m_animGraph->GetID(), AZStd::to_string(i).c_str());
            commandGroup.AddCommandString(commandString);
        }

        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(m_animGraph->GetNumNodeGroups(), 0);

        EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(result));
        EXPECT_EQ(m_animGraph->GetNumNodeGroups(), 100);

        EXPECT_TRUE(CommandSystem::GetCommandManager()->Redo(result));
        EXPECT_EQ(m_animGraph->GetNumNodeGroups(), 0);
    }
} // namespace EMotionFX
