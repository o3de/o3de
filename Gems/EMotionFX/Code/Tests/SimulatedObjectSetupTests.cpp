/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

// SimulatedObjectSetup.h includes
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/RTTI/TypeInfo.h>

// SimulatedObjectSetup.cpp includes
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>

#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/Node.h>

namespace SimulatedObjectSetupTests
{
    // Import real implementations
    namespace EMotionFX
    {
        using ::EMotionFX::ActorAllocator;
    } // namespace EMotionFX

#include <Tests/Mocks/Node.h>
#include <Tests/Mocks/Skeleton.h>
#include <Tests/Mocks/Actor.h>

#include <Tests/Prefabs/LeftArmSkeleton.h>

#include <EMotionFX/Source/SimulatedObjectSetup_Interface.inl>
#include <EMotionFX/Source/SimulatedObjectSetup_Impl.inl>

    using namespace EMotionFX;

    class SimulatedObjectSetupTestsFixture
        : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(SimulatedObjectSetupTestsFixture, TestSimulatedObjectSetup_AddSimulatedObject)
    {
        Actor actor;
        SimulatedObjectSetup setup(&actor);

        const SimulatedObject* object = setup.AddSimulatedObject();
        EXPECT_EQ(setup.GetSimulatedObject(0), object);
    }

    TEST_F(SimulatedObjectSetupTestsFixture, TestSimulatedObjectSetup_InsertSimulatedObjectAt)
    {
        Actor actor;
        SimulatedObjectSetup setup(&actor);

        for (int i = 0; i < 5; ++i)
        {
            setup.AddSimulatedObject();
        }
        const size_t objectIndex = 3;
        const SimulatedObject* object = setup.InsertSimulatedObjectAt(objectIndex);
        EXPECT_EQ(setup.GetSimulatedObject(objectIndex), object);
    }

    TEST_F(SimulatedObjectSetupTestsFixture, TestSimulatedObjectSetup_RemoveSimulatedObject)
    {
        Actor actor;
        SimulatedObjectSetup setup(&actor);

        AZStd::vector<SimulatedObject*> objects
        {
            setup.AddSimulatedObject(),
            setup.AddSimulatedObject(),
            setup.AddSimulatedObject(),
            setup.AddSimulatedObject(),
            setup.AddSimulatedObject(),
        };
        const size_t objectIndex = 3;
        setup.RemoveSimulatedObject(objectIndex);
        objects.erase(AZStd::next(objects.begin(), objectIndex));
        EXPECT_EQ(setup.GetSimulatedObjects(), objects);
    }

    ///////////////////////////////////////////////////////////////////////////
    using SimulatedObjectTestsFixture = SimulatedObjectSetupTestsFixture;

    TEST_F(SimulatedObjectTestsFixture, TestSimulatedObject_FindSimulatedJointBySkeletonJointIndex)
    {
        PrefabLeftArmSkeleton leftArmSkeleton;

        Actor actor;
        EXPECT_CALL(actor, GetSkeleton())
            .WillRepeatedly(testing::Return(&leftArmSkeleton.m_skeleton));

        SimulatedObjectSetup setup(&actor);
        SimulatedObject* object = setup.AddSimulatedObject();

        const AZ::u32 jointIndex = PrefabLeftArmSkeleton::leftThumb1Index;
        const SimulatedJoint* joint = object->AddSimulatedJoint(jointIndex);
        EXPECT_EQ(object->FindSimulatedJointBySkeletonJointIndex(jointIndex), joint);
    }

    TEST_F(SimulatedObjectTestsFixture, TestSimulatedObject_ContainsSimulatedJoint)
    {
        PrefabLeftArmSkeleton leftArmSkeleton;

        Actor actor;
        EXPECT_CALL(actor, GetSkeleton())
            .WillRepeatedly(testing::Return(&leftArmSkeleton.m_skeleton));

        SimulatedObjectSetup setup(&actor);
        SimulatedObject* object1 = setup.AddSimulatedObject();
        SimulatedObject* object2 = setup.AddSimulatedObject();

        const SimulatedJoint* joint1 = object1->AddSimulatedJoint(PrefabLeftArmSkeleton::leftElbowIndex);
        const SimulatedJoint* joint2 = object2->AddSimulatedJoint(PrefabLeftArmSkeleton::leftElbowIndex);
        EXPECT_TRUE(object1->ContainsSimulatedJoint(joint1));
        EXPECT_FALSE(object1->ContainsSimulatedJoint(joint2));
        EXPECT_FALSE(object2->ContainsSimulatedJoint(joint1));
        EXPECT_TRUE(object2->ContainsSimulatedJoint(joint2));
    }

    struct AddSimulatedJointAndChildrenParams
    {
        AZ::u32 m_jointIndex;
        size_t m_expectedSimulatedJointCount;
    };

    class AddSimulatedJointAndChildrenFixture
        : public SimulatedObjectTestsFixture
        , public testing::WithParamInterface<AddSimulatedJointAndChildrenParams>
    {
    };

    TEST_P(AddSimulatedJointAndChildrenFixture, TestSimulatedObject_AddSimulatedJointAndChildren)
    {
        PrefabLeftArmSkeleton leftArmSkeleton;

        Actor actor;
        EXPECT_CALL(actor, GetSkeleton())
            .WillRepeatedly(testing::Return(&leftArmSkeleton.m_skeleton));

        SimulatedObjectSetup setup(&actor);
        SimulatedObject* object = setup.AddSimulatedObject();

        object->AddSimulatedJointAndChildren(GetParam().m_jointIndex);
        EXPECT_EQ(object->GetSimulatedJoints().size(), GetParam().m_expectedSimulatedJointCount);
    }

    INSTANTIATE_TEST_CASE_P(Test, AddSimulatedJointAndChildrenFixture,
        testing::ValuesIn(std::vector<AddSimulatedJointAndChildrenParams>
        {
            {PrefabLeftArmSkeleton::leftShoulderIndex, 13}, // leftShoulder is a root joint
            {PrefabLeftArmSkeleton::leftElbowIndex, 12},
            {PrefabLeftArmSkeleton::leftWristIndex, 11},
            {PrefabLeftArmSkeleton::leftHandIndex, 10},
            {PrefabLeftArmSkeleton::leftThumb1Index, 3},
            {PrefabLeftArmSkeleton::leftThumb2Index, 2},
            {PrefabLeftArmSkeleton::leftThumb3Index, 1}, // leftThumb is a leaf joint
        })
    );

    class GetSimulatedRootJointFixture
        : public SimulatedObjectTestsFixture
        , public testing::WithParamInterface<testing::tuple<int, int>>
    {
    };

    // Determine if left is a parent of right, if right is a parent of left, or
    // if neither of the two joints are a parent of the other
    // Returns: -1 if left is a parent of right,
    //          1 if right is a parent of left,
    //          0 if neither is a parent of the other
    int NodeHasCommonParentImpl(const Node* left, const Node* right, const Node* initialLeft, const Node* initialRight)
    {
        if (left == right || (!left && !right))
        {
            return 0;
        }
        if (left == initialRight)
        {
            // right is a parent of left
            return 1;
        }
        if (right == initialLeft)
        {
            // left is a parent of right
            return -1;
        }

        const Node* leftParent = left ? left->GetParentNode() : nullptr;
        const Node* rightParent = right ? right->GetParentNode() : nullptr;
        return NodeHasCommonParentImpl(leftParent, rightParent, initialLeft, initialRight);
    }
    int NodeHasCommonParent(const Node* left, const Node* right)
    {
        return NodeHasCommonParentImpl(left, right, left, right);
    }

    // This test verifies that when any two joint indexes are added to a
    // simulated object, it results in the correct root joints. It is
    // parameterized on tuple<int, int>, where the two ints are the joint
    // indexes of the PrefabLeftArmSkeleton to add. Index 2 may be -1, in which
    // case only joint index 1 will be added to the object.
    // For any two joints, it is possible that one joint is in the parent list
    // of another joint. In this case, the number of root joints that should be
    // in the simulated object is 1. If the two joints are siblings of each
    // other, or in some other way unrelated (like not having any common
    // parents), then the expected number of root joints is 2.
    TEST_P(GetSimulatedRootJointFixture, TestSimulatedObject_GetSimulatedRootJoint)
    {
        PrefabLeftArmSkeleton leftArmSkeleton;

        Actor actor;
        EXPECT_CALL(actor, GetSkeleton())
            .WillRepeatedly(testing::Return(&leftArmSkeleton.m_skeleton));

        SimulatedObjectSetup setup(&actor);
        SimulatedObject* object = setup.AddSimulatedObject();

        const int jointIndex1 = testing::get<0>(GetParam());
        const int jointIndex2 = testing::get<1>(GetParam());

        object->AddSimulatedJointAndChildren(jointIndex1);
        if (jointIndex2 >= 0)
        {
            object->AddSimulatedJointAndChildren(jointIndex2);
        }

        const SimulatedJoint* simulatedJoint1 = object->FindSimulatedJointBySkeletonJointIndex(jointIndex1);
        EXPECT_TRUE(simulatedJoint1);

        if (jointIndex2 >= 0 && jointIndex2 != jointIndex1)
        {
            const SimulatedJoint* simulatedJoint2 = object->FindSimulatedJointBySkeletonJointIndex(jointIndex2);
            EXPECT_TRUE(simulatedJoint2);

            const Skeleton& skeleton = leftArmSkeleton.m_skeleton;
            const int hasCommonParent = NodeHasCommonParent(skeleton.GetNode(jointIndex1), skeleton.GetNode(jointIndex2));

            if (hasCommonParent == 0)
            {
                // simulatedJoint1 and simulatedJoint2 are not in each other's
                // parent list
                EXPECT_EQ(object->GetSimulatedRootJoint(object->GetSimulatedRootJointIndex(simulatedJoint1)), simulatedJoint1);
                EXPECT_EQ(object->GetSimulatedRootJoint(object->GetSimulatedRootJointIndex(simulatedJoint2)), simulatedJoint2);
                EXPECT_EQ(object->GetNumSimulatedRootJoints(), 2);
            }
            else if (hasCommonParent < 0)
            {
                // simulatedJoint1 is a parent of simulatedJoint2
                EXPECT_EQ(object->GetSimulatedRootJoint(object->GetSimulatedRootJointIndex(simulatedJoint1)), simulatedJoint1);
                EXPECT_EQ(object->GetNumSimulatedRootJoints(), 1);
            }
            else
            {
                // simulatedJoint2 is a parent of simulatedJoint1
                EXPECT_EQ(object->GetSimulatedRootJoint(object->GetSimulatedRootJointIndex(simulatedJoint2)), simulatedJoint2);
                EXPECT_EQ(object->GetNumSimulatedRootJoints(), 1);
            }
        }
        else
        {
            EXPECT_EQ(object->GetSimulatedRootJoint(object->GetSimulatedRootJointIndex(simulatedJoint1)), simulatedJoint1);
            EXPECT_EQ(object->GetNumSimulatedRootJoints(), 1);
        }
    }

    INSTANTIATE_TEST_CASE_P(Test, GetSimulatedRootJointFixture,
        ::testing::Combine(
            ::testing::Values(
                PrefabLeftArmSkeleton::leftShoulderIndex,
                PrefabLeftArmSkeleton::leftElbowIndex,
                PrefabLeftArmSkeleton::leftWristIndex,
                PrefabLeftArmSkeleton::leftHandIndex,
                PrefabLeftArmSkeleton::leftThumb1Index,
                PrefabLeftArmSkeleton::leftThumb2Index,
                PrefabLeftArmSkeleton::leftThumb3Index,
                PrefabLeftArmSkeleton::leftIndex1Index,
                PrefabLeftArmSkeleton::leftIndex2Index,
                PrefabLeftArmSkeleton::leftIndex3Index,
                PrefabLeftArmSkeleton::leftPinky1Index,
                PrefabLeftArmSkeleton::leftPinky2Index,
                PrefabLeftArmSkeleton::leftPinky3Index
            ),
            ::testing::Values(
                -1,
                PrefabLeftArmSkeleton::leftThumb1Index,
                PrefabLeftArmSkeleton::leftThumb2Index,
                PrefabLeftArmSkeleton::leftThumb3Index,
                PrefabLeftArmSkeleton::leftIndex1Index,
                PrefabLeftArmSkeleton::leftIndex2Index,
                PrefabLeftArmSkeleton::leftIndex3Index,
                PrefabLeftArmSkeleton::leftPinky1Index,
                PrefabLeftArmSkeleton::leftPinky2Index,
                PrefabLeftArmSkeleton::leftPinky3Index
            )
        ),
        [](const ::testing::TestParamInfo<::testing::tuple<int, int>>& info)
        {
            const auto getJointName = [](const int jointIndex) -> std::string
            {
                switch (jointIndex)
                {
                case PrefabLeftArmSkeleton::leftShoulderIndex:
                    return "leftShoulder";
                    break;
                case PrefabLeftArmSkeleton::leftElbowIndex:
                    return "leftElbow";
                    break;
                case PrefabLeftArmSkeleton::leftWristIndex:
                    return "leftWrist";
                    break;
                case PrefabLeftArmSkeleton::leftHandIndex:
                    return "leftHand";
                    break;
                case PrefabLeftArmSkeleton::leftThumb1Index:
                    return "leftThumb1";
                    break;
                case PrefabLeftArmSkeleton::leftThumb2Index:
                    return "leftThumb2";
                    break;
                case PrefabLeftArmSkeleton::leftThumb3Index:
                    return "leftThumb3";
                    break;
                case PrefabLeftArmSkeleton::leftIndex1Index:
                    return "leftIndex1";
                    break;
                case PrefabLeftArmSkeleton::leftIndex2Index:
                    return "leftIndex2";
                    break;
                case PrefabLeftArmSkeleton::leftIndex3Index:
                    return "leftIndex3";
                    break;
                case PrefabLeftArmSkeleton::leftPinky1Index:
                    return "leftPinky1";
                    break;
                case PrefabLeftArmSkeleton::leftPinky2Index:
                    return "leftPinky2";
                    break;
                case PrefabLeftArmSkeleton::leftPinky3Index:
                    return "leftPinky3";
                    break;
                default:
                    return "None";
                };
            };

            return "WithJoints_" + getJointName(testing::get<0>(info.param)) + "_and_" + getJointName(testing::get<1>(info.param));
        }
    );

    ///////////////////////////////////////////////////////////////////////////
    using SimulatedJointTestsFixture = SimulatedObjectSetupTestsFixture;

    TEST_F(SimulatedJointTestsFixture, TestSimulatedJoint_GettersSetters)
    {
        SimulatedJoint joint;

        EXPECT_EQ(joint.GetConeAngleLimit(), 60.0f);
        EXPECT_EQ(joint.GetMass(), 1.0f);
        EXPECT_EQ(joint.GetStiffness(), 0.0f);
        EXPECT_EQ(joint.GetDamping(), 0.001f);
        EXPECT_EQ(joint.GetGravityFactor(), 1.0f);
        EXPECT_EQ(joint.GetFriction(), 0.0f);
        EXPECT_EQ(joint.IsPinned(), false);

        const float newConeAngleLimit = 90.0f;
        const float newMass = 3.0f;
        const float newStiffness = 0.5f;
        const float newDamping = 0.1f;
        const float newGravityFactor = 1.2f;
        const float newFriction = 0.3f;
        const bool newPinned = true;

        joint.SetConeAngleLimit(newConeAngleLimit);
        joint.SetMass(newMass);
        joint.SetStiffness(newStiffness);
        joint.SetDamping(newDamping);
        joint.SetGravityFactor(newGravityFactor);
        joint.SetFriction(newFriction);
        joint.SetPinned(newPinned);

        EXPECT_EQ(joint.GetConeAngleLimit(), newConeAngleLimit);
        EXPECT_EQ(joint.GetMass(), newMass);
        EXPECT_EQ(joint.GetStiffness(), newStiffness);
        EXPECT_EQ(joint.GetDamping(), newDamping);
        EXPECT_EQ(joint.GetGravityFactor(), newGravityFactor);
        EXPECT_EQ(joint.GetFriction(), newFriction);
        EXPECT_EQ(joint.IsPinned(), newPinned);
    }

    TEST_F(SimulatedJointTestsFixture, TestSimulatedJoint_FindParentSimulatedJoint)
    {
        PrefabLeftArmSkeleton leftArmSkeleton;

        Actor actor;
        EXPECT_CALL(actor, GetSkeleton())
            .WillRepeatedly(testing::Return(&leftArmSkeleton.m_skeleton));

        SimulatedObjectSetup setup(&actor);
        SimulatedObject* object = setup.AddSimulatedObject();

        SimulatedJoint* simulatedThumb1 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftThumb1Index);
        SimulatedJoint* simulatedThumb2 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftThumb2Index);

        EXPECT_EQ(simulatedThumb2->FindParentSimulatedJoint(), simulatedThumb1);
        EXPECT_EQ(simulatedThumb1->FindParentSimulatedJoint(), nullptr);
    }

    TEST_F(SimulatedJointTestsFixture, TestSimulatedJoint_FindChildSimulatedJoint)
    {
        PrefabLeftArmSkeleton leftArmSkeleton;

        Actor actor;
        EXPECT_CALL(actor, GetSkeleton())
            .WillRepeatedly(testing::Return(&leftArmSkeleton.m_skeleton));

        SimulatedObjectSetup setup(&actor);
        SimulatedObject* object = setup.AddSimulatedObject();

        SimulatedJoint* simulatedHand = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftHandIndex);
        SimulatedJoint* simulatedThumb1 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftThumb1Index);
        SimulatedJoint* simulatedThumb2 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftThumb2Index);
        SimulatedJoint* simulatedThumb3 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftThumb3Index);
        SimulatedJoint* simulatedIndex1 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftIndex1Index);
        SimulatedJoint* simulatedIndex2 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftIndex2Index);
        SimulatedJoint* simulatedIndex3 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftIndex3Index);

        EXPECT_EQ(simulatedHand->FindChildSimulatedJoint(0), simulatedThumb1);
        EXPECT_EQ(simulatedHand->FindChildSimulatedJoint(1), simulatedIndex1);
        EXPECT_EQ(simulatedHand->FindChildSimulatedJoint(2), nullptr);

        EXPECT_EQ(simulatedThumb1->FindChildSimulatedJoint(0), simulatedThumb2);
        EXPECT_EQ(simulatedThumb1->FindChildSimulatedJoint(1), nullptr);

        EXPECT_EQ(simulatedThumb2->FindChildSimulatedJoint(0), simulatedThumb3);
        EXPECT_EQ(simulatedThumb2->FindChildSimulatedJoint(1), nullptr);

        EXPECT_EQ(simulatedThumb3->FindChildSimulatedJoint(0), nullptr);

        EXPECT_EQ(simulatedIndex1->FindChildSimulatedJoint(0), simulatedIndex2);
        EXPECT_EQ(simulatedIndex1->FindChildSimulatedJoint(1), nullptr);

        EXPECT_EQ(simulatedIndex2->FindChildSimulatedJoint(0), simulatedIndex3);
        EXPECT_EQ(simulatedIndex2->FindChildSimulatedJoint(1), nullptr);

        EXPECT_EQ(simulatedIndex3->FindChildSimulatedJoint(0), nullptr);
    }

    TEST_F(SimulatedJointTestsFixture, TestSimulatedJoint_CalculateSimulatedJointIndex)
    {
        PrefabLeftArmSkeleton leftArmSkeleton;

        Actor actor;
        EXPECT_CALL(actor, GetSkeleton())
            .WillRepeatedly(testing::Return(&leftArmSkeleton.m_skeleton));

        SimulatedObjectSetup setup(&actor);
        SimulatedObject* object = setup.AddSimulatedObject();

        SimulatedJoint* simulatedIndex3 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftIndex3Index);
        SimulatedJoint* simulatedIndex2 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftIndex2Index);
        SimulatedJoint* simulatedIndex1 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftIndex1Index);
        SimulatedJoint* simulatedThumb3 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftThumb3Index);
        SimulatedJoint* simulatedThumb2 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftThumb2Index);
        SimulatedJoint* simulatedThumb1 = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftThumb1Index);
        SimulatedJoint* simulatedHand = object->AddSimulatedJoint(PrefabLeftArmSkeleton::leftHandIndex);

        // Joints get sorted based on their skeletal joint index. Even though
        // these joints were added in reverse order, they are stored in a
        // sorted array internally.
        EXPECT_EQ(simulatedHand->CalculateSimulatedJointIndex().GetValue(), 0);
        EXPECT_EQ(simulatedThumb1->CalculateSimulatedJointIndex().GetValue(), 1);
        EXPECT_EQ(simulatedThumb2->CalculateSimulatedJointIndex().GetValue(), 2);
        EXPECT_EQ(simulatedThumb3->CalculateSimulatedJointIndex().GetValue(), 3);
        EXPECT_EQ(simulatedIndex1->CalculateSimulatedJointIndex().GetValue(), 4);
        EXPECT_EQ(simulatedIndex2->CalculateSimulatedJointIndex().GetValue(), 5);
        EXPECT_EQ(simulatedIndex3->CalculateSimulatedJointIndex().GetValue(), 6);
    }
} // namespace SimulatedObjectSetupTests
