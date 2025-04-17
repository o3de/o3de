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
#include <EMotionFX/Source/BlendTreeSetTransformNode.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/EMotionFXManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSetTransformNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSetTransformNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeSetTransformNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeSetTransformNode::UniqueData::Update()
    {
        BlendTreeSetTransformNode* transformNode = azdynamic_cast<BlendTreeSetTransformNode*>(m_object);
        AZ_Assert(transformNode, "Unique data linked to incorrect node type.");

        ActorInstance* actorInstance = m_animGraphInstance->GetActorInstance();
        Actor* actor = actorInstance->GetActor();

        m_nodeIndex = InvalidIndex;

        const AZStd::string& jointName = transformNode->GetJointName();
        if (!jointName.empty())
        {
            const Node* joint = actor->GetSkeleton()->FindNodeByName(jointName);
            if (joint)
            {
                m_nodeIndex = joint->GetNodeIndex();
            }
        }
    }

    BlendTreeSetTransformNode::BlendTreeSetTransformNode()
        : AnimGraphNode()
        , m_transformSpace(TRANSFORM_SPACE_WORLD)
    {
        // setup the input ports
        InitInputPorts(4);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsVector3("Translation", INPUTPORT_TRANSLATION, PORTID_INPUT_TRANSLATION);
        SetupInputPort("Rotation", INPUTPORT_ROTATION, MCore::AttributeQuaternion::TYPE_ID, PORTID_INPUT_ROTATION);
        SetupInputPortAsVector3("Scale", INPUTPORT_SCALE, PORTID_INPUT_SCALE);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }

    BlendTreeSetTransformNode::~BlendTreeSetTransformNode()
    {
    }

    bool BlendTreeSetTransformNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* BlendTreeSetTransformNode::GetPaletteName() const
    {
        return "Set Transform";
    }


    AnimGraphObject::ECategory BlendTreeSetTransformNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    void BlendTreeSetTransformNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        AnimGraphPose* outputPose;

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(uniqueData, uniqueData->m_nodeIndex == InvalidIndex);
        }

        OutputAllIncomingNodes(animGraphInstance);

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).m_connection)
        {
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            *outputPose = *inputPose;
        }
        else // init on the input pose
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);
        }

        if (GetIsEnabled())
        {
            // get the local transform from our node
            if (uniqueData->m_nodeIndex != InvalidIndex)
            {
                Transform outputTransform;

                switch (m_transformSpace)
                {
                case TRANSFORM_SPACE_LOCAL:
                    outputPose->GetPose().GetLocalSpaceTransform(uniqueData->m_nodeIndex, &outputTransform);
                    break;
                case TRANSFORM_SPACE_WORLD:
                    outputPose->GetPose().GetWorldSpaceTransform(uniqueData->m_nodeIndex, &outputTransform);
                    break;
                case TRANSFORM_SPACE_MODEL:
                    outputPose->GetPose().GetModelSpaceTransform(uniqueData->m_nodeIndex, &outputTransform);
                    break;
                default:
                    outputTransform.Identity();
                    AZ_Assert(false, "Unhandled transform space");
                    break;
                }

                // process the translation
                AZ::Vector3 translation;
                if (TryGetInputVector3(animGraphInstance, INPUTPORT_TRANSLATION, translation))
                {
                    outputTransform.m_position = translation;
                }

                // process the rotation
                if (GetInputPort(INPUTPORT_ROTATION).m_connection)
                {
                    const AZ::Quaternion& rotation = GetInputQuaternion(animGraphInstance, INPUTPORT_ROTATION)->GetValue();
                    outputTransform.m_rotation = rotation;
                }

                // process the scale
                EMFX_SCALECODE
                (
                    AZ::Vector3 scale;
                    if (TryGetInputVector3(animGraphInstance, INPUTPORT_SCALE, scale))
                    {
                        outputTransform.m_scale = scale;
                    }
                )

                // update the transformation of the node
                switch (m_transformSpace)
                {
                case TRANSFORM_SPACE_LOCAL:
                    outputPose->GetPose().SetLocalSpaceTransform(uniqueData->m_nodeIndex, outputTransform);
                    break;
                case TRANSFORM_SPACE_WORLD:
                    outputPose->GetPose().SetWorldSpaceTransform(uniqueData->m_nodeIndex, outputTransform);
                    break;
                case TRANSFORM_SPACE_MODEL:
                    outputPose->GetPose().SetModelSpaceTransform(uniqueData->m_nodeIndex, outputTransform);
                    break;
                default:
                    outputTransform.Identity();
                    AZ_Assert(false, "Unhandled transform space");
                    break;
                }
            }
        }

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }

    void BlendTreeSetTransformNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeSetTransformNode, AnimGraphNode>()
            ->Version(1)
            ->Field("nodeName", &BlendTreeSetTransformNode::m_nodeName)
            ->Field("transformSpace", &BlendTreeSetTransformNode::m_transformSpace)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeSetTransformNode>("Set Transform Node", "Transform node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeSetTransformNode::m_nodeName, "Node", "The node to apply the transform to.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeSetTransformNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeSetTransformNode::m_transformSpace)
        ;
    }
} // namespace EMotionFX
