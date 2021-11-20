/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ActorFixture.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>


namespace EMotionFX
{
    class SkeletalLODFixture
        : public ActorFixture
    {
    public:
        void SetUp()
        {
            ActorFixture::SetUp();
            GetActor()->AddLODLevel();
            DisableJointsForLOD(m_disabledJointNames, 1);
        }

        void DisableJointsForLOD(const std::vector<std::string>& jointNames, size_t lodLevel)
        {
            const Skeleton* skeleton = GetActor()->GetSkeleton();
            for (const std::string& jointName : jointNames)
            {
                Node* joint = skeleton->FindNodeByName(jointName.c_str());
                ASSERT_NE(joint, nullptr);

                joint->SetSkeletalLODStatus(lodLevel, false);
            }
        }

        static void VerifySkeletalLODFlags(const ActorInstance* actorInstance, const std::vector<std::string>& disabledJointNames, size_t lodLevel)
        {
            EXPECT_EQ(actorInstance->GetLODLevel(), lodLevel)
                << "Please note that setting the LOD level is delayed and happend with the next UpdateTransforms().";

            const Actor* actor = actorInstance->GetActor();
            const Skeleton* skeleton = actor->GetSkeleton();

            const AZStd::vector<AZ::u16>& enabledJoints = actorInstance->GetEnabledNodes();
            const size_t numEnabledJoints = enabledJoints.size();
            EXPECT_EQ(actorInstance->GetNumEnabledNodes(), actor->GetNumNodes() - disabledJointNames.size())
                << "The enabled joints on the actor instance are not in sync with the enabledJoints.";

            const size_t numJoints = skeleton->GetNumNodes();
            for (size_t i = 0; i < numJoints; ++i)
            {
                const Node* joint = skeleton->GetNode(i);

                // Check the skeletal LOD flag on the joint (actor asset).
                const bool isJointEnabled = std::find(disabledJointNames.begin(), disabledJointNames.end(), joint->GetName()) == disabledJointNames.end();
                EXPECT_EQ(isJointEnabled, joint->GetSkeletalLODStatus(lodLevel))
                    << "The skeletal LOD flag on the joint does not match the disabled joints set by the test.";

                // Check if the enabled joints on the actor instance is in sync.
                bool foundInEnabledJoints = false;
                for (size_t j = 0; j < numEnabledJoints; ++j)
                {
                    const AZ::u16 enabledJointIndex = actorInstance->GetEnabledNode(j);
                    const Node* enabledJoint = skeleton->GetNode(enabledJointIndex);
                    if (joint == enabledJoint)
                    {
                        foundInEnabledJoints = true;
                        break;
                    }
                }

                EXPECT_EQ(isJointEnabled, foundInEnabledJoints)
                    << "The joint is disabled (enabled) but has (not) been found in the enabled joints in the actor instance.";
            }
        }

    public:
        const std::vector<std::string> m_disabledJointNames = { "r_thumb1", "r_thumb2", "r_thumb3", "r_index1", "r_index2", "r_index3" };
    };

    TEST_F(SkeletalLODFixture, SkeletalLODTest)
    {
        // Check default LOD 0
        m_actorInstance->UpdateTransformations(0.0f);
        VerifySkeletalLODFlags(m_actorInstance, {}, 0);

        // Set to LOD 1
        m_actorInstance->SetLODLevel(1);
        m_actorInstance->UpdateTransformations(0.0f);
        VerifySkeletalLODFlags(m_actorInstance, m_disabledJointNames, 1);

        // Set back to LOD0
        m_actorInstance->SetLODLevel(0);
        m_actorInstance->UpdateTransformations(0.0f);
        VerifySkeletalLODFlags(m_actorInstance, {}, 0);
    }
} // namespace EMotionFX
