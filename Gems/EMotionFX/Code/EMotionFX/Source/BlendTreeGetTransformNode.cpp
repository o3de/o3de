/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendTreeGetTransformNode.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/TransformData.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeGetTransformNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeGetTransformNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeGetTransformNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeGetTransformNode::UniqueData::Update()
    {
        BlendTreeGetTransformNode* transformNode = azdynamic_cast<BlendTreeGetTransformNode*>(m_object);
        AZ_Assert(transformNode, "Unique data linked to incorrect node type.");

        m_nodeIndex = InvalidIndex;
        const AZStd::string& nodeName = transformNode->GetNodeName();
        const int actorInstanceParentDepth = transformNode->GetActorInstanceParentDepth();
        
        // lookup the actor instance to get the node from
        const ActorInstance* alignInstance = m_animGraphInstance->FindActorInstanceFromParentDepth(actorInstanceParentDepth);
        if (alignInstance)
        {
            const Node* alignNode = alignInstance->GetActor()->GetSkeleton()->FindNodeByName(nodeName);
            if (alignNode)
            {
                m_nodeIndex = alignNode->GetNodeIndex();
            }
        }
    }

    BlendTreeGetTransformNode::BlendTreeGetTransformNode()
        : AnimGraphNode()
        , m_transformSpace(TRANSFORM_SPACE_LOCAL)
    {
        m_actorNode.second = 0;

        // setup the input ports
        InitInputPorts(1);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);

        // setup the output ports
        InitOutputPorts(3);
        SetupOutputPort("Output Translation", OUTPUTPORT_TRANSLATION, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_TRANSLATION);
        SetupOutputPort("Output Rotation", OUTPUTPORT_ROTATION, MCore::AttributeQuaternion::TYPE_ID, PORTID_OUTPUT_ROTATION);
        SetupOutputPort("Output Scale", OUTPUTPORT_SCALE, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_SCALE);
    }

    BlendTreeGetTransformNode::~BlendTreeGetTransformNode()
    {
    }

    bool BlendTreeGetTransformNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* BlendTreeGetTransformNode::GetPaletteName() const
    {
        return "Get Transform";
    }


    AnimGraphObject::ECategory BlendTreeGetTransformNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    void BlendTreeGetTransformNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* inputPose = nullptr;
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(uniqueData, uniqueData->m_nodeIndex == InvalidIndex);
        }

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).m_connection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        }

        Pose* pose = nullptr;
        if (uniqueData->m_nodeIndex != InvalidIndex)
        {
            if (m_actorNode.second == 0)
            {
                // We operate over the input pose
                if (inputPose)
                {
                    pose = &inputPose->GetPose();
                }
            }
            else
            {
                const ActorInstance* alignInstance = animGraphInstance->FindActorInstanceFromParentDepth(m_actorNode.second);
                if (alignInstance)
                {
                    pose = alignInstance->GetTransformData()->GetCurrentPose();
                }
            }
        }

        Transform inputTransform;
        if (pose)
        {
            switch (m_transformSpace)
            {
            case TRANSFORM_SPACE_LOCAL:
                pose->GetLocalSpaceTransform(uniqueData->m_nodeIndex, &inputTransform);
                break;
            case TRANSFORM_SPACE_WORLD:
                pose->GetWorldSpaceTransform(uniqueData->m_nodeIndex, &inputTransform);
                break;
            case TRANSFORM_SPACE_MODEL:
                pose->GetModelSpaceTransform(uniqueData->m_nodeIndex, &inputTransform);
                break;
            default:
                AZ_Assert(false, "Unhandled transform space");
                inputTransform.Identity();
                break;
            }
        }
        else
        {
            inputTransform.Identity();
        }

        GetOutputVector3(animGraphInstance, OUTPUTPORT_TRANSLATION)->SetValue(inputTransform.m_position);
        GetOutputQuaternion(animGraphInstance, OUTPUTPORT_ROTATION)->SetValue(inputTransform.m_rotation);

        #ifndef EMFX_SCALE_DISABLED
            GetOutputVector3(animGraphInstance, OUTPUTPORT_SCALE)->SetValue(inputTransform.m_scale);
        #else
            GetOutputVector3(animGraphInstance, OUTPUTPORT_SCALE)->SetValue(AZ::Vector3::CreateOne());
        #endif
    }

    void BlendTreeGetTransformNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeGetTransformNode, AnimGraphNode>()
            ->Version(1)
            ->Field("actorNode", &BlendTreeGetTransformNode::m_actorNode)
            ->Field("transformSpace", &BlendTreeGetTransformNode::m_transformSpace)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeGetTransformNode>("Get Transform Node", "Get Transform node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("ActorGoalNode"), &BlendTreeGetTransformNode::m_actorNode, "Node", "The node to get the transform from.")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeGetTransformNode::InvalidateUniqueDatas)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeGetTransformNode::m_transformSpace)
        ;
    }
} // namespace EMotionFX
