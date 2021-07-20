/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/SystemComponentFixture.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>


namespace EMotionFX
{
    using MotionEventCommandTests = SystemComponentFixture;

    TEST_F(MotionEventCommandTests, RemoveMotionEventTrackCommandTest)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        Motion* motion = aznew Motion("SkeletalMotion1");

        MotionEventTable* eventTable = motion->GetEventTable();

        // Some of the motion event related commands automatically create sync tracks.
        // This would make data verification harder, so we just manually create it upfront.
        eventTable->AutoCreateSyncTrack(motion);

        const char* eventTrackName = "EventTrack1";
        MotionEventTrack* eventTrack = aznew MotionEventTrack(eventTrackName, motion);
        eventTable->AddTrack(eventTrack);
        EXPECT_EQ(eventTable->GetNumTracks(), 2);

        CommandSystem::CommandRemoveEventTrack(motion, 0);
        EXPECT_EQ(eventTable->GetNumTracks(), 1);

        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_EQ(eventTable->GetNumTracks(), 2);
        EXPECT_EQ(eventTable->GetTrack(1)->GetNameString(), eventTrackName);

        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        EXPECT_EQ(eventTable->GetNumTracks(), 1);

        motion->Destroy();
    }
} // namespace EMotionFX
