/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX
{
    TEST_F(SystemComponentFixture, SelectionListDanglingActorTest)
    {
        CommandSystem::SelectionList selectionList;

        Actor* actor = aznew Actor("TestActor");
        selectionList.AddActor(actor);
        EXPECT_EQ(selectionList.GetNumSelectedActors(), 1) << "Actor should be in selection list.";

        delete actor;
        EXPECT_EQ(selectionList.GetNumSelectedActors(), 0)
            << "Actor destruction should have automatially removed the actor from the selection list.";
    }

    TEST_F(SystemComponentFixture, SelectionListDanglingJointTest)
    {
        CommandSystem::SelectionList selectionList;

        Actor* actor = aznew Actor("TestActor");
        Node* joint = Node::Create("TestJoint", actor->GetSkeleton());
        actor->AddNode(joint);

        selectionList.AddNode(joint);
        EXPECT_EQ(selectionList.GetNumSelectedNodes(), 1) << "Joint should be in selection list.";

        delete actor;
        EXPECT_EQ(selectionList.GetNumSelectedNodes(), 0)
            << "Actor destruction should have automatially removed all corresponding joint from the selection list.";
    }
} // end namespace EMotionFX
