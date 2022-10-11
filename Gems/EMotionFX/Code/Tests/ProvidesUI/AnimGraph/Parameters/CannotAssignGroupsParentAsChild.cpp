/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <MCore/Source/IDGenerator.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Parameter/GroupParameter.h>
#include <Tests/UI/CommandRunnerFixture.h>

namespace EMotionFX
{
    TEST_F(CommandRunnerFixture, CannotAssignGroupsParentAsChild)
    {
        const AZ::u32 animGraphId = aznumeric_cast<AZ::u32>(MCore::GetIDGenerator().GenerateID()) + 1;

        ExecuteCommands({
            "CreateAnimGraph",
            "Select -animGraphID " + std::to_string(animGraphId),
            "AnimGraphAddGroupParameter -animGraphID " + std::to_string(animGraphId) + " -name OriginalParent",
            "AnimGraphAddGroupParameter -animGraphID " + std::to_string(animGraphId) + " -name OriginalChild -parent OriginalParent",
        });

        const AnimGraph* animGraph = GetAnimGraphManager().FindAnimGraphByID(animGraphId);
        ASSERT_TRUE(animGraph);
        const GroupParameter* originalParent = static_cast<const GroupParameter*>(animGraph->FindParameterByName("OriginalParent"));
        const GroupParameter* originalChild = static_cast<const GroupParameter*>(animGraph->FindParameterByName("OriginalChild"));

        EXPECT_THAT(originalParent->GetChildParameters(), testing::Contains(originalChild));
        EXPECT_THAT(originalChild->GetChildParameters(), testing::Not(testing::Contains(originalParent)));

        {
            AZStd::string result;
            EXPECT_FALSE(CommandSystem::GetCommandManager()->ExecuteCommand({"AnimGraphAdjustGroupParameter -animGraphID " + AZStd::to_string(animGraphId) + " -name OriginalChild -parameterNames OriginalParent -action add"}, result));
        }

        EXPECT_THAT(originalParent->GetChildParameters(), testing::Contains(originalChild));
        EXPECT_THAT(originalChild->GetChildParameters(), testing::Not(testing::Contains(originalParent)));
    }
} // namespace EMotionFX
