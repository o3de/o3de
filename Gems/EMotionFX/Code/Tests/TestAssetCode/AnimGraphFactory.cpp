/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>

namespace EMotionFX
{
    EmptyAnimGraph::EmptyAnimGraph()
    {
        // Emptry animgraph with nothing inside.
        SetRootStateMachine(aznew AnimGraphStateMachine());
        GetRootStateMachine()->SetName("rootStateMachine");
    }

    void EmptyAnimGraph::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EmptyAnimGraph>()
                ->Version(1);
        }
    }

    AnimGraphInstance* EmptyAnimGraph::GetAnimGraphInstance(ActorInstance* actorInstance, MotionSet* motionSet)
    {
        // TODO: Move this method to AnimGraphInstanceFactory when we create it.
        AnimGraphInstance* animGraphInstance = AnimGraphInstance::Create(this, actorInstance, motionSet);
        actorInstance->SetAnimGraphInstance(animGraphInstance);
        animGraphInstance->IncreaseReferenceCount();
        animGraphInstance->RecursiveInvalidateUniqueDatas();
        return animGraphInstance;
    }

    TwoMotionNodeAnimGraph::TwoMotionNodeAnimGraph()
    {
        /*
        Inside the root state machine:
            +-----------+
            |motionNodeA|
            +-----------+

            +-----------+
            |motionNodeB|
            +-----------+
        */
        m_motionNodeA = aznew AnimGraphMotionNode();
        m_motionNodeB = aznew AnimGraphMotionNode();
        m_motionNodeA->SetName("A");
        m_motionNodeB->SetName("B");
        GetRootStateMachine()->SetName("rootStateMachine");
        GetRootStateMachine()->AddChildNode(m_motionNodeA);
        GetRootStateMachine()->AddChildNode(m_motionNodeB);
        GetRootStateMachine()->SetEntryState(m_motionNodeA);
    }

    void TwoMotionNodeAnimGraph::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TwoMotionNodeAnimGraph>()
                ->Version(1);
        }
    }

    AnimGraphMotionNode* TwoMotionNodeAnimGraph::GetMotionNodeA()
    {
        return m_motionNodeA;
    }

    AnimGraphMotionNode* TwoMotionNodeAnimGraph::GetMotionNodeB()
    {
        return m_motionNodeB;
    }

    OneBlendTreeNodeAnimGraph::OneBlendTreeNodeAnimGraph()
    {
        /*
            +-----------+
            |m_blendTree|
            +-----------+
        */
        m_blendTree = aznew BlendTree();
        GetRootStateMachine()->AddChildNode(m_blendTree);
        GetRootStateMachine()->SetEntryState(m_blendTree);
    }

    void OneBlendTreeNodeAnimGraph::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<OneBlendTreeNodeAnimGraph>()
                ->Version(1);
        }
    }

    BlendTree* OneBlendTreeNodeAnimGraph::GetBlendTreeNode() const
    {
        return m_blendTree;
    }

    OneBlendTreeParameterNodeAnimGraph::OneBlendTreeParameterNodeAnimGraph()
    {
        /*
        Inside blend tree: 
            +---------------+    +-----------+
            |m_parameterNode|    |m_finalNode|
            +---------------+    +-----------+
        */
        m_parameterNode = aznew BlendTreeParameterNode();
        m_parameterNode->SetName("Parameters0");

        m_finalNode = aznew BlendTreeFinalNode();
        m_finalNode->SetName("FinalNode0");

        BlendTree* blendTree = aznew BlendTree();
        blendTree->SetName("BlendTree0");
        blendTree->AddChildNode(m_parameterNode);
        blendTree->AddChildNode(m_finalNode);

        GetRootStateMachine()->AddChildNode(blendTree);
        GetRootStateMachine()->SetEntryState(blendTree);

        InitAfterLoading();
    }

    BlendTreeParameterNode* OneBlendTreeParameterNodeAnimGraph::GetParameterNode() const
    {
        return m_parameterNode;
    }
} // namespace EMotionFX
