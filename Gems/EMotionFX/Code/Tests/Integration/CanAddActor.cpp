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

#include <QPushButton>
#include <QAction>
#include <QtTest>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AutoRegisteredActor.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanAddActor)
    {
        /*
        Test Case: C14459474
        Can Add Actor
        Imports an Actor to ensure that importing an actor is poosible.
        */

        RecordProperty("test_case_id", "C14459474");

        // Ensure that the number of actors loaded is 0
        ASSERT_EQ(GetActorManager().GetNumActors(), 0);

        // Load an Actor
        const char* actorCmd{ "ImportActor -filename @devroot@/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.actor" };
        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(actorCmd, result)) << result.c_str();
        }

        // Ensure the Actor is correct
        ASSERT_TRUE(GetActorManager().FindActorByName("rinactor"));
        EXPECT_EQ(GetActorManager().GetNumActors(), 1);
    }
} // namespace EMotionFX
