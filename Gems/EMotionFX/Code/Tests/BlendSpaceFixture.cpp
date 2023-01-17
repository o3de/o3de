/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <Tests/BlendSpaceFixture.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceFixture::TestEventHandler, EventHandlerAllocator)
    void BlendSpaceFixture::ConstructGraph()
    {
        AnimGraphFixture::ConstructGraph();

        m_blendTree = aznew BlendTree();
        m_rootStateMachine->AddChildNode(m_blendTree);
        m_rootStateMachine->SetEntryState(m_blendTree);

        m_finalNode = aznew BlendTreeFinalNode();
        m_blendTree->AddChildNode(m_finalNode);
    }

    void BlendSpaceFixture::AddEvent(Motion* motion, float time)
    {
        MotionEventTrack* eventTrack = MotionEventTrack::Create(motion);
        AZStd::shared_ptr<const TwoStringEventData> eventData = EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestEvent", motion->GetName());
        eventTrack->AddEvent(time, eventData);
        motion->GetEventTable()->AddTrack(eventTrack);
    }

    size_t BlendSpaceFixture::CreateSubMotion(Motion* motion, const std::string& name, const Transform& transform)
    {
        return motion->GetMotionData()->AddJoint(name.c_str(), transform, transform);
    }

    void BlendSpaceFixture::CreateMotions()
    {
        // Idle motion
        {
            Motion* motion = aznew Motion("Idle");
            auto motionData = aznew NonUniformMotionData();
            motion->SetMotionData(motionData);
            const size_t motionJointIndex = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            motionData->AllocateJointPositionSamples(motionJointIndex, 2);
            motionData->SetJointPositionSample(motionJointIndex, 0, {0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});
            motionData->SetJointPositionSample(motionJointIndex, 1, {1.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});

            m_motions.emplace_back(motion);
            m_idleMotion = motion;
        }

        // Forward motion
        {
            Motion* motion = aznew Motion("Forward");
            auto motionData = aznew NonUniformMotionData();
            motion->SetMotionData(motionData);
            const size_t motionJointIndex = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            motionData->AllocateJointPositionSamples(motionJointIndex, 2);
            motionData->SetJointPositionSample(motionJointIndex, 0, {0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});
            motionData->SetJointPositionSample(motionJointIndex, 1, {1.0f, AZ::Vector3(0.0f, 1.0f, 0.0f)});

            m_motions.emplace_back(motion);
            m_forwardMotion = motion;
        }

        // Run motion
        {
            Motion* motion = aznew Motion("Run");
            auto motionData = aznew NonUniformMotionData();
            motion->SetMotionData(motionData);
            const size_t motionJointIndex = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            motionData->AllocateJointPositionSamples(motionJointIndex, 2);
            motionData->SetJointPositionSample(motionJointIndex, 0, {0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});
            motionData->SetJointPositionSample(motionJointIndex, 1, {1.0f, AZ::Vector3(0.0f, 2.0f, 0.0f)});

            m_motions.emplace_back(motion);
            m_runMotion = motion;
        }

        // Strafe motion
        {
            Motion* motion = aznew Motion("Strafe");
            auto motionData = aznew NonUniformMotionData();
            motion->SetMotionData(motionData);
            const size_t motionJointIndex = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            motionData->AllocateJointPositionSamples(motionJointIndex, 2);
            motionData->SetJointPositionSample(motionJointIndex, 0, {0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});
            motionData->SetJointPositionSample(motionJointIndex, 1, {1.0f, AZ::Vector3(1.0f, 0.0f, 0.0f)});

            m_motions.emplace_back(motion);
            m_strafeMotion = motion;
        }

        // Rotate left motion
        {
            Motion* motion = aznew Motion("Rotate Left");
            auto motionData = aznew NonUniformMotionData();
            motion->SetMotionData(motionData);
            const size_t motionJointIndex = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            motionData->AllocateJointRotationSamples(motionJointIndex, 2);
            motionData->SetJointRotationSample(motionJointIndex, 0, {0.0f, AZ::Quaternion::CreateIdentity()});
            motionData->SetJointRotationSample(motionJointIndex, 1, {1.0f, AZ::Quaternion::CreateRotationZ(0.5f)});

            m_motions.emplace_back(motion);
            m_rotateLeftMotion = motion;
        }

        // Forward strafe 45 deg
        {
            Motion* motion = aznew Motion("Forward strafe 45 deg");
            auto motionData = aznew NonUniformMotionData();
            motion->SetMotionData(motionData);
            const size_t motionJointIndex = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            motionData->AllocateJointPositionSamples(motionJointIndex, 2);
            motionData->SetJointPositionSample(motionJointIndex, 0, {0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});
            motionData->SetJointPositionSample(motionJointIndex, 1, {1.0f, AZ::Vector3(1.0f, 1.0f, 0.0f)});

            m_motions.emplace_back(motion);
            m_forwardStrafe45 = motion;
        }

        // Forward slope 45 deg
        {
            Motion* motion = aznew Motion("Forward slope 45 deg");
            auto motionData = aznew NonUniformMotionData();
            motion->SetMotionData(motionData);
            const size_t motionJointIndex = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            motionData->AllocateJointPositionSamples(motionJointIndex, 2);
            motionData->SetJointPositionSample(motionJointIndex, 0, {0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});
            motionData->SetJointPositionSample(motionJointIndex, 1, {1.0f, AZ::Vector3(0.0f, 1.0f, 1.0f)});

            m_motions.emplace_back(motion);
            m_forwardSlope45 = motion;
        }

        for (Motion* motion : m_motions)
        {
            AddEvent(motion, 0.1f);
            motion->UpdateDuration();

            MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
            m_motionSet->AddMotionEntry(motionEntry);
        }
    }

    MotionSet::MotionEntry* BlendSpaceFixture::FindMotionEntry(Motion* motion) const
    {
        MotionSet::MotionEntry* entry = m_motionSet->FindMotionEntry(motion);
        EXPECT_NE(entry, nullptr) << "Cannot find motion entry for motion " << motion->GetName() << ".";
        return entry;
    }

    void BlendSpaceFixture::SetUp()
    {
        AnimGraphFixture::SetUp();

        m_eventHandler = aznew TestEventHandler();
        EMotionFX::GetEventManager().AddEventHandler(m_eventHandler);
        
        Node* rootJoint = m_actor->GetSkeleton()->GetNode(0);
        m_rootJointName = rootJoint->GetName();
        m_actor->SetMotionExtractionNode(rootJoint);

        CreateMotions();
    }

    void BlendSpaceFixture::TearDown()
    {
        for (Motion* motion : m_motions)
        {
            motion->Destroy();
        }
        m_motions.clear();

        EMotionFX::GetEventManager().RemoveEventHandler(m_eventHandler);
        delete m_eventHandler;

        AnimGraphFixture::TearDown();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BlendSpace1DFixture::ConstructGraph()
    {
        BlendSpaceFixture::ConstructGraph();

        m_blendSpace1DNode = aznew BlendSpace1DNode();
        m_blendTree->AddChildNode(m_blendSpace1DNode);

        m_floatNodeX = aznew BlendTreeFloatConstantNode();
        m_blendTree->AddChildNode(m_floatNodeX);

        m_finalNode->AddConnection(m_blendSpace1DNode, BlendSpace1DNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
        m_blendSpace1DNode->AddConnection(m_floatNodeX, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendSpace1DNode::INPUTPORT_VALUE);
    }

    void BlendSpace1DFixture::SetUp()
    {
        BlendSpaceFixture::SetUp();

        m_blendSpace1DNode->SetEvaluatorType(azrtti_typeid<BlendSpaceMoveSpeedParamEvaluator>());
        m_blendSpace1DNode->SetMotions({
            FindMotionEntry(m_idleMotion)->GetId(),
            FindMotionEntry(m_forwardMotion)->GetId(),
            FindMotionEntry(m_runMotion)->GetId()
            });

        m_blendSpace1DNode->Reinit();
        GetEMotionFX().Update(0.0f);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BlendSpace2DFixture::ConstructGraph()
    {
        BlendSpaceFixture::ConstructGraph();

        m_blendSpace2DNode = aznew BlendSpace2DNode();
        m_blendTree->AddChildNode(m_blendSpace2DNode);

        m_floatNodeX = aznew BlendTreeFloatConstantNode();
        m_blendTree->AddChildNode(m_floatNodeX);

        m_floatNodeY = aznew BlendTreeFloatConstantNode();
        m_blendTree->AddChildNode(m_floatNodeY);

        m_finalNode->AddConnection(m_blendSpace2DNode, BlendSpace2DNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
        m_blendSpace2DNode->AddConnection(m_floatNodeX, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendSpace2DNode::INPUTPORT_XVALUE);
        m_blendSpace2DNode->AddConnection(m_floatNodeY, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendSpace2DNode::INPUTPORT_YVALUE);
    }

    void BlendSpace2DFixture::SetUp()
    {
        BlendSpaceFixture::SetUp();

        m_blendSpace2DNode->SetEvaluatorTypeX(azrtti_typeid<BlendSpaceMoveSpeedParamEvaluator>());
        m_blendSpace2DNode->SetEvaluatorTypeY(azrtti_typeid<BlendSpaceLeftRightVelocityParamEvaluator>());
        m_blendSpace2DNode->SetMotions({
            FindMotionEntry(m_idleMotion)->GetId(),
            FindMotionEntry(m_forwardMotion)->GetId(),
            FindMotionEntry(m_runMotion)->GetId(),
            FindMotionEntry(m_strafeMotion)->GetId()
            });

        m_blendSpace2DNode->Reinit();
        GetEMotionFX().Update(0.0f);
    }
}
