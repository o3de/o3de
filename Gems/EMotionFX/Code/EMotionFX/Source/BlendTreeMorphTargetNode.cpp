/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendTreeMorphTargetNode.h>
#include <EMotionFX/Source/MorphSetup.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMorphTargetNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMorphTargetNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeMorphTargetNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeMorphTargetNode::UniqueData::Update()
    {
        BlendTreeMorphTargetNode* morphTargetNode = azdynamic_cast<BlendTreeMorphTargetNode*>(m_object);
        AZ_Assert(morphTargetNode, "Unique data linked to incorrect node type.");

        // Force update the morph target indices.
        morphTargetNode->UpdateMorphIndices(m_animGraphInstance->GetActorInstance(), this, true);
    }

    BlendTreeMorphTargetNode::BlendTreeMorphTargetNode()
        : AnimGraphNode()
    {
        // Setup input ports.
        InitInputPorts(2);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Morph Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // Setup output ports.
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    void BlendTreeMorphTargetNode::Reinit()
    {
        AnimGraphNode::Reinit();

        // Update the node info string.
        if (!m_morphTargetNames.empty())
        {
            SetNodeInfo(m_morphTargetNames[0].c_str());
        }
        else
        {
            SetNodeInfoNone();
        }
    }


    bool BlendTreeMorphTargetNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        SetNodeInfoNone();
        Reinit();
        return true;
    }


    const char* BlendTreeMorphTargetNode::GetPaletteName() const
    {
        return "Morph Target";
    }


    AnimGraphObject::ECategory BlendTreeMorphTargetNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    // Update the morph indices, which will convert the morph target name into an index into the current LOD.
    void BlendTreeMorphTargetNode::UpdateMorphIndices(ActorInstance* actorInstance, UniqueData* uniqueData, bool forceUpdate)
    {
        // Check if our LOD level changed, if not, we don't need to refresh it.
        const size_t lodLevel = actorInstance->GetLODLevel();
        if (!forceUpdate && uniqueData->m_lastLodLevel == lodLevel)
        {
            return;
        }

        // Convert the morph target name into an index for fast lookup.
        if (!m_morphTargetNames.empty())
        {
            const EMotionFX::MorphSetup* morphSetup = actorInstance->GetActor()->GetMorphSetup(lodLevel);
            if (morphSetup)
            {
                const AZStd::string& morphTargetName = m_morphTargetNames[0];
                uniqueData->m_morphTargetIndex = morphSetup->FindMorphTargetIndexByName(morphTargetName.c_str());
            }
        }
        else
        {
            uniqueData->m_morphTargetIndex = InvalidIndex;
        }

        uniqueData->m_lastLodLevel = lodLevel;
    }


    // Calculate the output pose.
    void BlendTreeMorphTargetNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // Mark this node as having an error when the morph target cannot be found.
        // If there is none setup, we see that as a non-error state, otherwise you would see the node marked as error directly after creation.
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        if (GetEMotionFX().GetIsInEditorMode())
        {
            if (m_morphTargetNames.empty())
            {
                SetHasError(uniqueData, false);
            }
            else
            {
                SetHasError(uniqueData, uniqueData->m_morphTargetIndex == InvalidIndex);
            }
        }

        // Refresh the morph target indices when needed.
        // This has to happen when we changed LOD levels, as the new LOD might have another number of morph targets.
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        UpdateMorphIndices(actorInstance, uniqueData, false);
        
        // If there is no input pose init the uutput pose to the bind pose.
        AnimGraphPose* outputPose;
        if (!m_inputPorts[INPUTPORT_POSE].m_connection)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        }
        else // Init the output pose to the input pose.
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
        }

        // Try to modify the morph target weight with the value we specified as input.
        if (!m_disabled && uniqueData->m_morphTargetIndex != InvalidIndex)
        {
            // If we have an input to the weight port, read that value use that value to overwrite the pose value with.
            if (m_inputPorts[INPUTPORT_WEIGHT].m_connection)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_WEIGHT));
                const float morphWeight = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT);

                // Overwrite the morph target weight.
                outputPose->GetPose().SetMorphWeight(uniqueData->m_morphTargetIndex, morphWeight);
            }
        }

        // Debug visualize the output pose.
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }

    void BlendTreeMorphTargetNode::SetNodeInfoNone()
    {
        SetNodeInfo("<none>");
    }

    void BlendTreeMorphTargetNode::SetMorphTargetNames(const AZStd::vector<AZStd::string>& morphTargetNames)
    {
        m_morphTargetNames = morphTargetNames;
    }

    void BlendTreeMorphTargetNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeMorphTargetNode, AnimGraphNode>()
            ->Version(1)
            -> Field("morphTargetNames", &BlendTreeMorphTargetNode::m_morphTargetNames)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeMorphTargetNode>("Morph Target", "Morph target attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("ActorMorphTargetName"), &BlendTreeMorphTargetNode::m_morphTargetNames, "Morph Target", "The morph target to apply the weight changes to.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMorphTargetNode::Reinit)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ;
    }
} // namespace EMotionFX
