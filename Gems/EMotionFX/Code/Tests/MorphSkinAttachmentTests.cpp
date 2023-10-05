/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/AttachmentSkin.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <Tests/JackGraphFixture.h>
#include <Tests/TestAssetCode/TestMotionAssets.h>

namespace EMotionFX
{
    class MorphSkinAttachmentFixture
        : public JackGraphFixture
    {
    public:
        void AddMotionData(Motion* newMotion, const AZStd::string& motionId)
        {
            auto motionData = newMotion->GetMotionData();

            // Create some morph sub motions.
            const AZStd::vector<AZStd::string> morphNames { "morph1", "morph2", "morph3", "morph4", "newmorph1", "newmorph2" };
            for (size_t i = 0; i < morphNames.size(); ++i)
            {
                motionData->AddMorph(morphNames[i], static_cast<float>(i + 1) / 10.0f);
            }

            // Add the motion to the motion set.
            EMotionFX::MotionSet::MotionEntry* newMotionEntry = aznew EMotionFX::MotionSet::MotionEntry();
            newMotionEntry->SetMotion(newMotion);
            m_motionSet->AddMotionEntry(newMotionEntry);
            m_motionSet->SetMotionEntryId(newMotionEntry, motionId);
        }

        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();
            
            // Motion of Jack walking forward (Y-axis change) with right arm aiming towards front.
            AddMotionData(TestMotionAssets::GetJackWalkForward(), "jack_walk_forward_aim_zup");

            // Anim graph:
            //
            // +-----------------+       +------------+       +---------+
            // |m_floatConstNode |------>|m_motionNode|------>|finalNode|
            // +-----------------+       |------------+       +---------+

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_floatConstNode = aznew BlendTreeFloatConstantNode();
            m_motionNode = aznew AnimGraphMotionNode();

            // Control motion and effects to be used.
            m_motionNode->AddMotionId("jack_walk_forward_aim_zup");
            m_motionNode->SetLoop(true);

            m_blendTree = aznew BlendTree();
            m_blendTree->AddChildNode(m_motionNode);
            m_blendTree->AddChildNode(m_floatConstNode);
            m_blendTree->AddChildNode(finalNode);

            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            finalNode->AddConnection(m_motionNode, AnimGraphMotionNode::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);
        }

        void OnPostActorCreated() override
        {
            m_attachmentActor = m_actor->Clone();

            // Create a few morph targets in the main actor.
            MorphSetup* morphSetup = MorphSetup::Create();
            m_actor->SetMorphSetup(0, morphSetup);
            morphSetup->AddMorphTarget(MorphTargetStandard::Create("morph1"));
            morphSetup->AddMorphTarget(MorphTargetStandard::Create("morph2"));
            morphSetup->AddMorphTarget(MorphTargetStandard::Create("morph3"));
            morphSetup->AddMorphTarget(MorphTargetStandard::Create("morph4"));

            // Create a few other morphs in our attachment.
            MorphSetup* attachMorphSetup = MorphSetup::Create();
            m_attachmentActor->SetMorphSetup(0, attachMorphSetup);
            attachMorphSetup->AddMorphTarget(MorphTargetStandard::Create("newmorph1"));
            attachMorphSetup->AddMorphTarget(MorphTargetStandard::Create("newmorph2"));
            attachMorphSetup->AddMorphTarget(MorphTargetStandard::Create("morph1"));
            attachMorphSetup->AddMorphTarget(MorphTargetStandard::Create("morph2"));

            // Make sure our morphs are registered in the transform data poses.
            m_actor->ResizeTransformData();
            m_attachmentActor->ResizeTransformData();
            m_attachmentActorInstance = ActorInstance::Create(m_attachmentActor.get());
        }

        void TearDown() override
        {
            m_attachmentActorInstance->Destroy();
            m_attachmentActor.reset();

            JackGraphFixture::TearDown();
        }

    protected:
        AnimGraphMotionNode* m_motionNode = nullptr;
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatConstantNode* m_floatConstNode = nullptr;
        AZStd::unique_ptr<Actor> m_attachmentActor = nullptr;
        ActorInstance* m_attachmentActorInstance = nullptr;
    };

    TEST_F(MorphSkinAttachmentFixture, TransferTestUnattached)
    {
        GetEMotionFX().Update(1.0f / 60.0f);

        // The main actor instance should receive the morph sub motion values.
        const Pose& curPose = *m_actorInstance->GetTransformData()->GetCurrentPose();
        ASSERT_EQ(curPose.GetNumMorphWeights(), 4);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(0), 0.1f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(1), 0.2f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(2), 0.3f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(3), 0.4f);

        // We expect no transfer of morph weights, since we aren't attached.
        const Pose& attachPose = *m_attachmentActorInstance->GetTransformData()->GetCurrentPose();
        ASSERT_EQ(attachPose.GetNumMorphWeights(), 4);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(0), 0.0f);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(1), 0.0f);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(2), 0.0f);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(3), 0.0f);
    };

    TEST_F(MorphSkinAttachmentFixture, TransferTestAttached)
    {
        // Create the attachment.
        AttachmentSkin* skinAttachment = AttachmentSkin::Create(m_actorInstance, m_attachmentActorInstance);
        m_actorInstance->AddAttachment(skinAttachment);

        GetEMotionFX().Update(1.0f / 60.0f);

        // The main actor instance should receive the morph sub motion values.
        const Pose& curPose = *m_actorInstance->GetTransformData()->GetCurrentPose();
        ASSERT_EQ(curPose.GetNumMorphWeights(), 4);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(0), 0.1f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(1), 0.2f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(2), 0.3f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(3), 0.4f);

        // The skin attachment should now receive morph values from the main actor.
        const Pose& attachPose = *m_attachmentActorInstance->GetTransformData()->GetCurrentPose();
        ASSERT_EQ(attachPose.GetNumMorphWeights(), 4);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(0), 0.0f);    // Once we auto register missing morphs this should be 0.5. See LY-100212
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(1), 0.0f);    // Once we auto register missing morphs this should be 0.6. See LY-100212
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(2), 0.1f);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(3), 0.2f);
    };
} // end namespace EMotionFX
