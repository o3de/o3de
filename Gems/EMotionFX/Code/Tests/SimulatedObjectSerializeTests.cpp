/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ActorFixture.h"
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    using SimulatedObjectSerializeTests = ActorFixture;
    
    TEST_F(SimulatedObjectSerializeTests, SerializeTest)
    {
        SimulatedObjectSetup* setup = GetActor()->GetSimulatedObjectSetup().get();        

        // Build some setup.
        SimulatedObject* object = setup->AddSimulatedObject();
        object->SetName("Left Arm");
        object->SetDampingFactor(2.0f);
        object->SetGravityFactor(3.0f);
        object->SetStiffnessFactor(4.0f);
        const AZStd::vector<AZStd::string> jointNames = { "l_upArm", "l_loArm", "l_hand" };
        Skeleton* skeleton = GetActor()->GetSkeleton();
        for (const AZStd::string& name : jointNames)
        {
            size_t skeletonJointIndex;
            const Node* skeletonJoint = skeleton->FindNodeAndIndexByName(name, skeletonJointIndex);
            ASSERT_NE(skeletonJoint, nullptr);
            ASSERT_NE(skeletonJointIndex, InvalidIndex);

            SimulatedJoint* simulatedJoint = object->AddSimulatedJoint(skeletonJointIndex);
            simulatedJoint->SetDamping(0.1f);
            simulatedJoint->SetMass(2.0f);
            simulatedJoint->SetStiffness(200.0f);
            simulatedJoint->SetGravityFactor(1.5f);
            simulatedJoint->SetCollisionRadius(3.0f);
            simulatedJoint->SetGeometricAutoExclusion(false);
            simulatedJoint->SetColliderExclusionTags({"TagA", "TagB"});
            simulatedJoint->SetAutoExcludeMode(SimulatedJoint::AutoExcludeMode::Self);
        }
        object->GetSimulatedJoint(0)->SetPinned(true);

        // Serialize it and deserialize it.
        const AZStd::string serialized = SerializeSimulatedObjectSetup(GetActor());
        AZStd::unique_ptr<SimulatedObjectSetup> loadedSetup(DeserializeSimulatedObjectSetup(serialized));

        // Verify some of the contents of the deserialized version.
        ASSERT_EQ(loadedSetup->GetNumSimulatedObjects(), 1);
        SimulatedObject* loadedObject = loadedSetup->GetSimulatedObject(0);
        ASSERT_STREQ(loadedObject->GetName().c_str(), "Left Arm");
        ASSERT_EQ(loadedObject->GetNumSimulatedJoints(), jointNames.size());
        ASSERT_FLOAT_EQ(loadedObject->GetDampingFactor(), 2.0f);
        ASSERT_FLOAT_EQ(loadedObject->GetGravityFactor(), 3.0f);
        ASSERT_FLOAT_EQ(loadedObject->GetStiffnessFactor(), 4.0f);
        for (size_t i = 0; i < jointNames.size(); ++i)
        {
            const SimulatedJoint* loadedJoint = loadedObject->GetSimulatedJoint(i);
            ASSERT_STREQ(skeleton->GetNode(loadedJoint->GetSkeletonJointIndex())->GetName(), jointNames[i].c_str());
            ASSERT_FLOAT_EQ(loadedJoint->GetDamping(), 0.1f);
            ASSERT_FLOAT_EQ(loadedJoint->GetMass(), 2.0f);
            ASSERT_FLOAT_EQ(loadedJoint->GetStiffness(), 200.0f);
            ASSERT_FLOAT_EQ(loadedJoint->GetGravityFactor(), 1.5f);
            ASSERT_FLOAT_EQ(loadedJoint->GetCollisionRadius(), 3.0f);
            ASSERT_EQ(loadedJoint->IsPinned(), (i==0) ? true : false);
            ASSERT_EQ(loadedJoint->IsGeometricAutoExclusion(), false);
            ASSERT_EQ(loadedJoint->GetColliderExclusionTags().size(), 2);
            ASSERT_STREQ(loadedJoint->GetColliderExclusionTags()[0].c_str(), "TagA");
            ASSERT_STREQ(loadedJoint->GetColliderExclusionTags()[1].c_str(), "TagB");
            ASSERT_EQ(loadedJoint->GetAutoExcludeMode(), SimulatedJoint::AutoExcludeMode::Self);
        }
    }
} // namespace EMotionFX
