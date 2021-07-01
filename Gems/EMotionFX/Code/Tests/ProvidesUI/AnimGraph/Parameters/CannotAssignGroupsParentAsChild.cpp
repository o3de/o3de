/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Parameter/GroupParameter.h>
#include <Tests/UI/CommandRunnerFixture.h>

namespace EMotionFX
{

    TEST_F(CommandRunnerFixture, CannotAssignGroupsParentAsChild)
    {
        ExecuteCommands({
            "CreateAnimGraph",
            "Select -animGraphID 0",
            "AnimGraphAddGroupParameter -animGraphID 0 -name OriginalParent",
            "AnimGraphAddGroupParameter -animGraphID 0 -name OriginalChild -parent OriginalParent",
        });

        const AnimGraph* animGraph = GetAnimGraphManager().FindAnimGraphByID(0);
        ASSERT_TRUE(animGraph);
        const GroupParameter* originalParent = static_cast<const GroupParameter*>(animGraph->FindParameterByName("OriginalParent"));
        const GroupParameter* originalChild = static_cast<const GroupParameter*>(animGraph->FindParameterByName("OriginalChild"));

        EXPECT_THAT(originalParent->GetChildParameters(), testing::Contains(originalChild));
        EXPECT_THAT(originalChild->GetChildParameters(), testing::Not(testing::Contains(originalParent)));

        {
            AZStd::string result;
            EXPECT_FALSE(CommandSystem::GetCommandManager()->ExecuteCommand("AnimGraphAdjustGroupParameter -animGraphID 0 -name OriginalChild -parameterNames OriginalParent -action add", result));
        }

        EXPECT_THAT(originalParent->GetChildParameters(), testing::Contains(originalChild));
        EXPECT_THAT(originalChild->GetChildParameters(), testing::Not(testing::Contains(originalParent)));
    }
} // namespace EMotionFX
