/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "EMotionFXConfig.h"
#include "AnimGraphObjectFactory.h"
#include "AnimGraphObject.h"
#include "AnimGraphManager.h"

#include <MCore/Source/LogManager.h>

// default object types
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeBlend2NodeBase.h>
#include <EMotionFX/Source/BlendTreeBlend2AdditiveNode.h>
#include <EMotionFX/Source/BlendTreeBlend2LegacyNode.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/BlendTreePoseSwitchNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeFloatMath1Node.h>
#include <EMotionFX/Source/BlendTreeFloatMath2Node.h>
#include <EMotionFX/Source/BlendTreeFloatSwitchNode.h>
#include <EMotionFX/Source/BlendTreeFloatConditionNode.h>
#include <EMotionFX/Source/BlendTreeGetTransformNode.h>
#include <EMotionFX/Source/BlendTreeBoolLogicNode.h>
#include <EMotionFX/Source/BlendTreeSmoothingNode.h>
#include <EMotionFX/Source/BlendTreeMaskNode.h>
#include <EMotionFX/Source/BlendTreeMaskLegacyNode.h>
#include <EMotionFX/Source/BlendTreeMorphTargetNode.h>
#include <EMotionFX/Source/BlendTreeMotionFrameNode.h>
#include <EMotionFX/Source/BlendTreeVector3Math1Node.h>
#include <EMotionFX/Source/BlendTreeVector3Math2Node.h>
#include <EMotionFX/Source/BlendTreeVector2DecomposeNode.h>
#include <EMotionFX/Source/BlendTreeVector3DecomposeNode.h>
#include <EMotionFX/Source/BlendTreeVector4DecomposeNode.h>
#include <EMotionFX/Source/BlendTreeVector2ComposeNode.h>
#include <EMotionFX/Source/BlendTreeVector3ComposeNode.h>
#include <EMotionFX/Source/BlendTreeVector4ComposeNode.h>
#include <EMotionFX/Source/BlendTreeRotationMath2Node.h>
#include <EMotionFX/Source/BlendTreeRotationLimitNode.h>
#include <EMotionFX/Source/BlendTreeTwoLinkIKNode.h>
#include <EMotionFX/Source/BlendTreeLookAtNode.h>
#include <EMotionFX/Source/BlendTreeTransformNode.h>
#include <EMotionFX/Source/BlendTreeAccumTransformNode.h>
#include <EMotionFX/Source/BlendTreeRangeRemapperNode.h>
#include <EMotionFX/Source/BlendTreeSetTransformNode.h>
#include <EMotionFX/Source/BlendTreeDirectionToWeightNode.h>
#include <EMotionFX/Source/BlendTreeMirrorPoseNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeMirrorPoseNode.h>
#include <EMotionFX/Source/BlendTreePoseSubtractNode.h>
#include <EMotionFX/Source/TransformSpace.h>
#include <EMotionFX/Source/BlendTreeRagdollNode.h>
#include <EMotionFX/Source/BlendTreeRagdollStrengthModifierNode.h>
#include <EMotionFX/Source/BlendTreeFootIKNode.h>
#include <EMotionFX/Source/BlendTreeRaycastNode.h>
#include <EMotionFX/Source/BlendTreeSimulatedObjectNode.h>

#include "AnimGraphBindPoseNode.h"
#include "AnimGraphMotionNode.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphExitNode.h"
#include "AnimGraphEntryNode.h"
#include "AnimGraphHubNode.h"
#include "AnimGraphReferenceNode.h"

#include "AnimGraphTransitionCondition.h"
#include "AnimGraphParameterCondition.h"
#include "AnimGraphMotionCondition.h"
#include "AnimGraphStateCondition.h"
#include "AnimGraphTimeCondition.h"
#include "AnimGraphTagCondition.h"
#include "AnimGraphVector2Condition.h"
#include "AnimGraphPlayTimeCondition.h"

#include "AnimGraphTriggerAction.h"
#include "AnimGraphParameterAction.h"
#include "AnimGraphFollowerParameterAction.h"
#include "AnimGraphSymbolicFollowerParameterAction.h"
#include "AnimGraphSimpleStateAction.h"
#include "Allocators.h"

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphObjectFactory, AnimGraphAllocator)

    AnimGraphObjectFactory::AnimGraphObjectFactory()
    {
        UITypesSet& uiTypes = GetUITypes();
        for (const AZ::TypeId& type : uiTypes)
        {
            m_animGraphObjectPrototypes.emplace_back(Create(type));
        }
    }

    AnimGraphObjectFactory::~AnimGraphObjectFactory()
    {
        for (AnimGraphObject* object : m_animGraphObjectPrototypes)
        {
            delete object;
        }
        m_animGraphObjectPrototypes.clear();
    }

    void AnimGraphObjectFactory::ReflectTypes(AZ::ReflectContext* context)
    {
        TransformSpace::Reflect(context);

        AnimGraphNode::Reflect(context);
        AnimGraphStateMachine::Reflect(context);
        AnimGraphStateTransition::Reflect(context);
        AnimGraphExitNode::Reflect(context);
        AnimGraphEntryNode::Reflect(context);

        AnimGraphMotionNode::Reflect(context);
        BlendSpaceNode::Reflect(context);
        BlendSpace1DNode::Reflect(context);
        BlendSpace2DNode::Reflect(context);
        AnimGraphBindPoseNode::Reflect(context);
        AnimGraphHubNode::Reflect(context);
        AnimGraphReferenceNode::Reflect(context);

        AnimGraphParameterCondition::Reflect(context);
        AnimGraphVector2Condition::Reflect(context);
        AnimGraphMotionCondition::Reflect(context);
        AnimGraphStateCondition::Reflect(context);
        AnimGraphTimeCondition::Reflect(context);
        AnimGraphTransitionCondition::Reflect(context);
        AnimGraphPlayTimeCondition::Reflect(context);
        AnimGraphTagCondition::Reflect(context);

        TriggerActionSetup::Reflect(context);
        AnimGraphParameterAction::Reflect(context);
        AnimGraphFollowerParameterAction::Reflect(context);
        AnimGraphSymbolicFollowerParameterAction::Reflect(context);
        AnimGraphSimpleStateAction::Reflect(context);
        AnimGraphTriggerAction::Reflect(context);

        // Blend tree
        BlendTree::Reflect(context);
        BlendTreeConnection::Reflect(context);
        BlendTreeFinalNode::Reflect(context);
        BlendTreeBlend2NodeBase::Reflect(context);
        BlendTreeBlend2Node::Reflect(context);
        BlendTreeBlend2AdditiveNode::Reflect(context);
        BlendTreeBlend2LegacyNode::Reflect(context);
        BlendTreeBlendNNode::Reflect(context);
        BlendTreeParameterNode::Reflect(context);
        BlendTreeFloatMath1Node::Reflect(context);
        BlendTreeFloatMath2Node::Reflect(context);
        BlendTreeFloatConditionNode::Reflect(context);
        BlendTreeFloatConstantNode::Reflect(context);
        BlendTreeFloatSwitchNode::Reflect(context);
        BlendTreeBoolLogicNode::Reflect(context);
        BlendTreePoseSwitchNode::Reflect(context);
        BlendTreeMaskNode::Reflect(context);
        BlendTreeMaskLegacyNode::Reflect(context);
        BlendTreeMorphTargetNode::Reflect(context);
        BlendTreeMotionFrameNode::Reflect(context);
        BlendTreeVector3Math1Node::Reflect(context);
        BlendTreeVector3Math2Node::Reflect(context);
        BlendTreeVector2DecomposeNode::Reflect(context);
        BlendTreeVector3DecomposeNode::Reflect(context);
        BlendTreeVector4DecomposeNode::Reflect(context);
        BlendTreeVector2ComposeNode::Reflect(context);
        BlendTreeVector3ComposeNode::Reflect(context);
        BlendTreeRotationMath2Node::Reflect(context);
        BlendTreeRotationLimitNode::Reflect(context);
        BlendTreeVector4ComposeNode::Reflect(context);
        BlendTreeSmoothingNode::Reflect(context);
        BlendTreeRangeRemapperNode::Reflect(context);
        BlendTreeDirectionToWeightNode::Reflect(context);
        BlendTreeMirrorPoseNode::Reflect(context);
        BlendTreeTwoLinkIKNode::Reflect(context);
        BlendTreeLookAtNode::Reflect(context);
        BlendTreeTransformNode::Reflect(context);
        BlendTreeGetTransformNode::Reflect(context);
        BlendTreeSetTransformNode::Reflect(context);
        BlendTreeAccumTransformNode::Reflect(context);
        BlendTreePoseSubtractNode::Reflect(context);
        BlendTreeRagdollNode::Reflect(context);
        BlendTreeRagdollStrenghModifierNode::Reflect(context);
        BlendTreeFootIKNode::Reflect(context);
        BlendTreeRaycastNode::Reflect(context);
        BlendTreeSimulatedObjectNode::Reflect(context);
    }

    AnimGraphObjectFactory::UITypesSet& AnimGraphObjectFactory::GetUITypes()
    {
        static UITypesSet uitypes = {
            azrtti_typeid<AnimGraphBindPoseNode>(),
            azrtti_typeid<AnimGraphStateMachine>(),
            azrtti_typeid<AnimGraphMotionNode>(),
            azrtti_typeid<AnimGraphHubNode>(),
            azrtti_typeid<AnimGraphExitNode>(),
            azrtti_typeid<AnimGraphEntryNode>(),
            azrtti_typeid<AnimGraphReferenceNode>(),
            azrtti_typeid<BlendTree>(),
            azrtti_typeid<BlendTreeFinalNode>(),
            azrtti_typeid<BlendSpace1DNode>(),
            azrtti_typeid<BlendSpace2DNode>(),
            azrtti_typeid<BlendTreeBlend2Node>(),
            azrtti_typeid<BlendTreeBlend2AdditiveNode>(),
            azrtti_typeid<BlendTreeBlend2LegacyNode>(),
            azrtti_typeid<BlendTreeBlendNNode>(),
            azrtti_typeid<BlendTreeParameterNode>(),
            azrtti_typeid<BlendTreeFloatMath1Node>(),
            azrtti_typeid<BlendTreeFloatMath2Node>(),
            azrtti_typeid<BlendTreeFloatConditionNode>(),
            azrtti_typeid<BlendTreeFloatConstantNode>(),
            azrtti_typeid<BlendTreeFloatSwitchNode>(),
            azrtti_typeid<BlendTreeBoolLogicNode>(),
            azrtti_typeid<BlendTreePoseSwitchNode>(),
            azrtti_typeid<BlendTreeMaskNode>(),
            azrtti_typeid<BlendTreeMaskLegacyNode>(),
            azrtti_typeid<BlendTreeMorphTargetNode>(),
            azrtti_typeid<BlendTreeMotionFrameNode>(),
            azrtti_typeid<BlendTreeVector3Math1Node>(),
            azrtti_typeid<BlendTreeVector3Math2Node>(),
            azrtti_typeid<BlendTreeVector2DecomposeNode>(),
            azrtti_typeid<BlendTreeVector3DecomposeNode>(),
            azrtti_typeid<BlendTreeVector4DecomposeNode>(),
            azrtti_typeid<BlendTreeVector2ComposeNode>(),
            azrtti_typeid<BlendTreeVector3ComposeNode>(),
            azrtti_typeid<BlendTreeVector4ComposeNode>(),
            azrtti_typeid<BlendTreeSmoothingNode>(),
            azrtti_typeid<BlendTreeRangeRemapperNode>(),
            azrtti_typeid<BlendTreeDirectionToWeightNode>(),
            azrtti_typeid<BlendTreeMirrorPoseNode>(),
            azrtti_typeid<BlendTreeTwoLinkIKNode>(),
            azrtti_typeid<BlendTreeLookAtNode>(),
            azrtti_typeid<BlendTreeTransformNode>(),
            azrtti_typeid<BlendTreeGetTransformNode>(),
            azrtti_typeid<BlendTreeSetTransformNode>(),
            azrtti_typeid<BlendTreeAccumTransformNode>(),
            azrtti_typeid<BlendTreePoseSubtractNode>(),
            azrtti_typeid<BlendTreeRagdollNode>(),
            azrtti_typeid<BlendTreeRagdollStrenghModifierNode>(),
            azrtti_typeid<BlendTreeRotationLimitNode>(),
            azrtti_typeid<BlendTreeRotationMath2Node>(),
            azrtti_typeid<BlendTreeFootIKNode>(),
            azrtti_typeid<BlendTreeRaycastNode>(),
            azrtti_typeid<BlendTreeSimulatedObjectNode>(),
            azrtti_typeid<AnimGraphStateTransition>(),
            azrtti_typeid<AnimGraphParameterCondition>(),
            azrtti_typeid<AnimGraphVector2Condition>(),
            azrtti_typeid<AnimGraphMotionCondition>(),
            azrtti_typeid<AnimGraphStateCondition>(),
            azrtti_typeid<AnimGraphTimeCondition>(),
            azrtti_typeid<AnimGraphPlayTimeCondition>(),
            azrtti_typeid<AnimGraphTagCondition>(),
            azrtti_typeid<AnimGraphParameterAction>(),
            azrtti_typeid<AnimGraphFollowerParameterAction>(),
            azrtti_typeid<AnimGraphSymbolicFollowerParameterAction>(),
            azrtti_typeid<AnimGraphSimpleStateAction>(),
            azrtti_typeid<BlendTreeRotationLimitNode>(),
            azrtti_typeid<BlendTreeRotationMath2Node>()
        };

        return uitypes;
    }


    AnimGraphObject* AnimGraphObjectFactory::Create(const AZ::TypeId& type, AnimGraph* animGraph)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(type);
        if (!classData)
        {
            AZ_Warning("EMotionFX", false, "Can't find class data for this type.");
            return nullptr;
        }

        AnimGraphObject* animGraphObject = reinterpret_cast<AnimGraphObject*>(classData->m_factory->Create(classData->m_name));
        if (animGraph)
        {
            animGraphObject->InitAfterLoading(animGraph);
        }
        return animGraphObject;
    }
} // namespace EMotionFX
