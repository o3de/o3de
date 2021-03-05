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
