/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/BlendTreeRagdollStrengthModifierNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/PoseDataRagdoll.h>
#include <EMotionFX/Source/RagdollInstance.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRagdollStrenghModifierNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRagdollStrenghModifierNode::UniqueData, AnimGraphAllocator)

    BlendTreeRagdollStrenghModifierNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeRagdollStrenghModifierNode::UniqueData::Update()
    {
        BlendTreeRagdollStrenghModifierNode* ragdollModifierNode = azdynamic_cast<BlendTreeRagdollStrenghModifierNode*>(m_object);
        AZ_Assert(ragdollModifierNode, "Unique data linked to incorrect node type.");

        const AZStd::vector<AZStd::string>& modifiedJointNames = ragdollModifierNode->GetModifiedJointNames();
        const Actor* actor = m_animGraphInstance->GetActorInstance()->GetActor();
        AnimGraphPropertyUtils::ReinitJointIndices(actor, modifiedJointNames, m_modifiedJointIndices);
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    BlendTreeRagdollStrenghModifierNode::BlendTreeRagdollStrenghModifierNode()
        : AnimGraphNode()
        , m_strength(1.0f)
        , m_dampingRatio(1.0f)
        , m_strengthInputType(STRENGTHINPUTTYPE_OVERWRITE)
        , m_dampingRatioInputType(DAMPINGRATIOINPUTTYPE_NONE)
    {
        InitInputPorts(3);
        SetupInputPort("Input Pose", PORTID_POSE, AttributePose::TYPE_ID, INPUTPORT_POSE);
        SetupInputPortAsNumber("Strength", PORTID_STRENGTH, INPUTPORT_STRENGTH);
        SetupInputPortAsNumber("Damping Ratio", PORTID_DAMPINGRATIO, INPUTPORT_DAMPINGRATIO);

        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", PORTID_OUTPUT_POSE, OUTPUTPORT_POSE);
    }

    bool BlendTreeRagdollStrenghModifierNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    void BlendTreeRagdollStrenghModifierNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        RequestPoses(animGraphInstance);
        AnimGraphPose* animGraphOutputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();

        if (HasConnectionAtInputPort(INPUTPORT_POSE))
        {
            // Forward the input pose to the output pose in case there is a connection.
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* animGraphInputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            *animGraphOutputPose = *animGraphInputPose;
        }
        else
        {
            // In case no target pose is connected, use the bind pose as base.
            animGraphOutputPose->InitFromBindPose(actorInstance);
            return;
        }

        // As we already forwarded the input pose at this point, we can just return in case the node is disabled.
        if (m_disabled)
        {
            return;
        }

        Pose& outputPose = animGraphOutputPose->GetPose();
        if (GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose, m_visualizeColor);
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        if (GetEMotionFX().GetIsInEditorMode())
        {
            // We have a connection plugged in while we expect to just forward the strengths or the damping ratios from the input pose.
            if ((HasConnectionAtInputPort(INPUTPORT_STRENGTH) && m_strengthInputType == STRENGTHINPUTTYPE_NONE) ||
                (HasConnectionAtInputPort(INPUTPORT_DAMPINGRATIO) && m_dampingRatioInputType == DAMPINGRATIOINPUTTYPE_NONE))
            {
                SetHasError(uniqueData, true);
            }
            else
            {
                SetHasError(uniqueData, false);
            }
        }

        RagdollInstance* ragdollInstance = actorInstance->GetRagdollInstance();
        if (ragdollInstance && !m_modifiedJointNames.empty())
        {
            // Make sure the output pose contains a ragdoll pose data linked to our actor instance (assures enough space for the ragdoll node state array).
            PoseDataRagdoll* outputPoseData = outputPose.GetAndPreparePoseData<PoseDataRagdoll>(actorInstance);

            if (m_strengthInputType != STRENGTHINPUTTYPE_NONE)
            {
                float inputStrength = m_strength;
                if (HasConnectionAtInputPort(INPUTPORT_STRENGTH))
                {
                    OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_STRENGTH));
                    inputStrength = GetInputFloat(animGraphInstance, INPUTPORT_STRENGTH)->GetValue();
                }

                switch (m_strengthInputType)
                {
                case STRENGTHINPUTTYPE_OVERWRITE:
                {
                    for (size_t jointIndex : uniqueData->m_modifiedJointIndices)
                    {
                        const AZ::Outcome<size_t> ragdollNodeIndex = ragdollInstance->GetRagdollNodeIndex(jointIndex);
                        if (ragdollNodeIndex.IsSuccess())
                        {
                            Physics::RagdollNodeState& ragdollNodeState = outputPoseData->GetRagdollNodeState(ragdollNodeIndex.GetValue());
                            ragdollNodeState.m_strength = inputStrength;
                        }
                    }
                    break;
                }
                case STRENGTHINPUTTYPE_MULTIPLY:
                {
                    for (size_t jointIndex : uniqueData->m_modifiedJointIndices)
                    {
                        const AZ::Outcome<size_t> ragdollNodeIndex = ragdollInstance->GetRagdollNodeIndex(jointIndex);
                        if (ragdollNodeIndex.IsSuccess())
                        {
                            Physics::RagdollNodeState& ragdollNodeState = outputPoseData->GetRagdollNodeState(ragdollNodeIndex.GetValue());
                            ragdollNodeState.m_strength *= inputStrength;
                        }
                    }
                    break;
                }
                }
            }

            if (m_dampingRatioInputType != DAMPINGRATIOINPUTTYPE_NONE)
            {
                float inputDampingRatio = m_dampingRatio;
                if (HasConnectionAtInputPort(INPUTPORT_DAMPINGRATIO))
                {
                    OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DAMPINGRATIO));
                    inputDampingRatio = GetInputFloat(animGraphInstance, INPUTPORT_DAMPINGRATIO)->GetValue();
                }

                for (size_t jointIndex : uniqueData->m_modifiedJointIndices)
                {
                    const AZ::Outcome<size_t> ragdollNodeIndex = ragdollInstance->GetRagdollNodeIndex(jointIndex);
                    if (ragdollNodeIndex.IsSuccess())
                    {
                        Physics::RagdollNodeState& ragdollNodeState = outputPoseData->GetRagdollNodeState(ragdollNodeIndex.GetValue());
                        ragdollNodeState.m_dampingRatio = inputDampingRatio;
                    }
                }
            }
        }
    }

    bool BlendTreeRagdollStrenghModifierNode::IsStrengthReadOnly() const
    {
        if (m_strengthInputType == STRENGTHINPUTTYPE_NONE)
        {
            return true;
        }

        return false;
    }

    bool BlendTreeRagdollStrenghModifierNode::IsDampingRatioReadOnly() const
    {
        if (m_dampingRatioInputType == DAMPINGRATIOINPUTTYPE_NONE)
        {
            return true;
        }

        return false;
    }

    AZStd::string BlendTreeRagdollStrenghModifierNode::GetModifiedJointName(int index) const
    {
        return m_modifiedJointNames[index];
    }

    void BlendTreeRagdollStrenghModifierNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BlendTreeRagdollStrenghModifierNode, AnimGraphNode>()
                ->Version(1)
                ->Field("strengthInputType", &BlendTreeRagdollStrenghModifierNode::m_strengthInputType)
                ->Field("strength", &BlendTreeRagdollStrenghModifierNode::m_strength)
                ->Field("dampingRatioInputType", &BlendTreeRagdollStrenghModifierNode::m_dampingRatioInputType)
                ->Field("dampingRatio", &BlendTreeRagdollStrenghModifierNode::m_dampingRatio)
                ->Field("modifiedJoints", &BlendTreeRagdollStrenghModifierNode::m_modifiedJointNames)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<BlendTreeRagdollStrenghModifierNode>("Ragdoll Strength Modifier", "Ragdoll strength modifier node properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeRagdollStrenghModifierNode::m_strengthInputType, "Strength input type", "Specifies if the joint strength shall be forwarded from the input pose, overwritten or multiplied with the given value.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->EnumAttribute(STRENGTHINPUTTYPE_NONE, "Use from input pose")
                        ->EnumAttribute(STRENGTHINPUTTYPE_OVERWRITE, "Overwrite strength")
                        ->EnumAttribute(STRENGTHINPUTTYPE_MULTIPLY, "Strength as multiplier")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeRagdollStrenghModifierNode::m_strength, "Strength", "Strength value that will be applied to the selected nodes in case no connection is connected to the input port.")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &BlendTreeRagdollStrenghModifierNode::IsStrengthReadOnly)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeRagdollStrenghModifierNode::m_dampingRatioInputType, "Damping ratio input type", "Specifies if the damping ratios shall be forwarded from the input pose or overwritten with the given value.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->EnumAttribute(DAMPINGRATIOINPUTTYPE_NONE, "Use from input pose")
                        ->EnumAttribute(DAMPINGRATIOINPUTTYPE_OVERWRITE, "Overwrite damping ratio")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeRagdollStrenghModifierNode::m_dampingRatio, "Damping ratio", "Damping ratio value that will be applied to the selected nodes in case no connection is connected to the input port.")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &BlendTreeRagdollStrenghModifierNode::IsDampingRatioReadOnly)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->DataElement(AZ_CRC_CE("ActorRagdollJoints"), &BlendTreeRagdollStrenghModifierNode::m_modifiedJointNames, "Modified joints", "The strengh and/or damping ratios of the selected joints will be modified.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeRagdollStrenghModifierNode::Reinit)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &BlendTreeRagdollStrenghModifierNode::GetModifiedJointName)
                        ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ_CRC_CE("ActorJointElement"))
                ;
            }
        }
    }
} // namespace EMotionFX
