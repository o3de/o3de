/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/BlendTreeMaskNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMaskNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMaskNode::Mask, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMaskNode::UniqueData, AnimGraphObjectUniqueDataAllocator)
    const size_t BlendTreeMaskNode::s_numMasks = 4;

    BlendTreeMaskNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeMaskNode::UniqueData::Update()
    {
        BlendTreeMaskNode* maskNode = azdynamic_cast<BlendTreeMaskNode*>(m_object);
        AZ_Assert(maskNode, "Unique data linked to incorrect node type.");

        const Actor* actor = m_animGraphInstance->GetActorInstance()->GetActor();
        const size_t numMaskInstances = maskNode->GetNumUsedMasks();
        m_maskInstances.resize(numMaskInstances);
        size_t maskInstanceIndex = 0;

        m_motionExtractionInputPortNr.reset();
        const size_t motionExtractionJointIndex = m_animGraphInstance->GetActorInstance()->GetActor()->GetMotionExtractionNodeIndex();

        const AZStd::vector<Mask>& masks = maskNode->GetMasks();
        const size_t numMasks = masks.size();
        for (size_t i = 0; i < numMasks; ++i)
        {
            const Mask& mask = masks[i];
            if (!mask.m_jointNames.empty())
            {
                const size_t inputPortNr = INPUTPORT_START + i;

                // Get the joint indices by joint names and cache them in the unique data
                // so that we don't have to look them up at runtime.
                UniqueData::MaskInstance& maskInstance = m_maskInstances[maskInstanceIndex];
                AnimGraphPropertyUtils::ReinitJointIndices(actor, mask.m_jointNames, maskInstance.m_jointIndices);
                maskInstance.m_inputPortNr = inputPortNr;

                // Check if the motion extraction node is part of this mask and cache the mask index in that case.
                for (size_t jointIndex : maskInstance.m_jointIndices)
                {
                    if (jointIndex == motionExtractionJointIndex)
                    {
                        m_motionExtractionInputPortNr = inputPortNr;
                        break;
                    }
                }

                maskInstanceIndex++;
            }
        }
    }

    BlendTreeMaskNode::BlendTreeMaskNode()
        : AnimGraphNode()
    {
        m_masks.resize(s_numMasks);

        // Setup the input ports.
        InitInputPorts(1 + s_numMasks); // Base pose and the input poses for the masks.
        SetupInputPort("Base Pose", INPUTPORT_BASEPOSE, AttributePose::TYPE_ID, INPUTPORT_BASEPOSE);
        for (size_t i = 0; i < s_numMasks; ++i)
        {
            const uint32 portId = static_cast<uint32>(i) + INPUTPORT_START;
            SetupInputPort(
                AZStd::string::format("Pose %zu", i).c_str(),
                portId,
                AttributePose::TYPE_ID,
                portId);
        }

        // Setup the output ports.
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_RESULT);

        ActorNotificationBus::Handler::BusConnect();
    }

    BlendTreeMaskNode::~BlendTreeMaskNode()
    {
        ActorNotificationBus::Handler::BusDisconnect();
    }

    void BlendTreeMaskNode::Reinit()
    {
        AZ::u8 maskCounter = 0;
        for (Mask& mask : m_masks)
        {
            mask.m_maskIndex = maskCounter;
            mask.m_parent = this;
            maskCounter++;
        }

        AnimGraphNode::Reinit();
    }

    bool BlendTreeMaskNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    void BlendTreeMaskNode::OnMotionExtractionNodeChanged(Actor* actor, [[maybe_unused]] Node* newMotionExtractionNode)
    {
        if (!m_animGraph)
        {
            return;
        }

        bool needsReinit = false;
        const size_t numAnimGraphInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
            if (actor == animGraphInstance->GetActorInstance()->GetActor())
            {
                needsReinit = true;
                break;
            }
        }

        if (needsReinit)
        {
            Reinit();
        }
    }

    void BlendTreeMaskNode::Output(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        RequestPoses(animGraphInstance);
        AnimGraphPose* outputAnimGraphPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
        Pose& outputPose = outputAnimGraphPose->GetPose();

        // Use the input base pose as starting pose to apply the masks onto.
        AnimGraphNode* basePoseNode = GetInputNode(INPUTPORT_BASEPOSE);
        if (basePoseNode)
        {
            OutputIncomingNode(animGraphInstance, basePoseNode);
            *outputAnimGraphPose = *basePoseNode->GetMainOutputPose(animGraphInstance);
        }
        else
        {
            // Use bindpose in case no base pose node is connected.
            outputAnimGraphPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        }

        // Iterate over the non-empty masks and copy over its transforms.
        for (const UniqueData::MaskInstance& maskInstance : uniqueData->m_maskInstances)
        {
            const size_t inputPortNr = maskInstance.m_inputPortNr;
            AnimGraphNode* inputNode = GetInputNode(inputPortNr);
            if (inputNode)
            {
                OutputIncomingNode(animGraphInstance, inputNode);
                const Pose& inputPose = GetInputPose(animGraphInstance, inputPortNr)->GetValue()->GetPose();

                for (size_t jointIndex : maskInstance.m_jointIndices)
                {
                    outputPose.SetLocalSpaceTransform(jointIndex, inputPose.GetLocalSpaceTransform(jointIndex));
                }
            }
        }

        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputAnimGraphPose->GetPose(), m_visualizeColor);
        }
    }

    void BlendTreeMaskNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        AnimGraphNode* basePoseNode = GetInputNode(INPUTPORT_BASEPOSE);
        if (basePoseNode)
        {
            UpdateIncomingNode(animGraphInstance, basePoseNode, timePassedInSeconds);
            uniqueData->Init(animGraphInstance, basePoseNode);
        }
        else
        {
            uniqueData->Clear();
        }

        for (const UniqueData::MaskInstance& maskInstance : uniqueData->m_maskInstances)
        {
            AnimGraphNode* inputNode = GetInputNode(maskInstance.m_inputPortNr);
            if (inputNode)
            {
                UpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);
            }
        }
    }

    void BlendTreeMaskNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        RequestRefDatas(animGraphInstance);
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        AnimGraphNode* basePoseNode = GetInputNode(INPUTPORT_BASEPOSE);
        if (basePoseNode)
        {
            PostUpdateIncomingNode(animGraphInstance, basePoseNode, timePassedInSeconds);

            const AnimGraphNodeData* basePoseNodeUniqueData = basePoseNode->FindOrCreateUniqueNodeData(animGraphInstance);
            data->SetEventBuffer(basePoseNodeUniqueData->GetRefCountedData()->GetEventBuffer());
        }

        for (const UniqueData::MaskInstance& maskInstance : uniqueData->m_maskInstances)
        {
            const size_t inputPortNr = maskInstance.m_inputPortNr;
            AnimGraphNode* inputNode = GetInputNode(inputPortNr);
            if (!inputNode)
            {
                continue;
            }

            PostUpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);

            // If we want to output events for this input, add the incoming events to the output event buffer.
            if (GetOutputEvents(inputPortNr))
            {
                const AnimGraphEventBuffer& inputEventBuffer = inputNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData()->GetEventBuffer();

                AnimGraphEventBuffer& outputEventBuffer = data->GetEventBuffer();
                outputEventBuffer.AddAllEventsFrom(inputEventBuffer);
            }
        }

        // Apply motion extraction delta from either the base pose or one of the masks depending on if a mask has the joint set or not.
        bool motionExtractionApplied = false;
        if (uniqueData->m_motionExtractionInputPortNr.has_value())
        {
            AnimGraphNode* inputNode = GetInputNode(uniqueData->m_motionExtractionInputPortNr.value());
            if (inputNode)
            {
                AnimGraphRefCountedData* sourceData = inputNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
                data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
                data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
                motionExtractionApplied = true;
            }
        }

        // In case the motion extraction node is not part of any of the masks while the base pose is connected, use that as a fallback.
        if (!motionExtractionApplied && basePoseNode)
        {
            AnimGraphRefCountedData* sourceData = basePoseNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }
    }

    size_t BlendTreeMaskNode::GetNumUsedMasks() const
    {
        size_t result = 0;
        for (const Mask& mask : m_masks)
        {
            if (!mask.m_jointNames.empty())
            {
                result++;
            }
        }
        return result;
    }

    AZStd::string BlendTreeMaskNode::GetMaskJointName(size_t maskIndex, size_t jointIndex) const
    {
        return m_masks[maskIndex].m_jointNames[jointIndex];
    }

    bool BlendTreeMaskNode::GetOutputEvents(size_t inputPortNr) const
    {
        if (inputPortNr > INPUTPORT_BASEPOSE)
        {
            return m_masks[inputPortNr - INPUTPORT_START].m_outputEvents;
        }

        return true;
    }

    void BlendTreeMaskNode::SetMask(size_t maskIndex, const AZStd::vector<AZStd::string>& jointNames)
    {
        m_masks[maskIndex].m_jointNames = jointNames;
    }

    void BlendTreeMaskNode::SetOutputEvents(size_t maskIndex, bool outputEvents)
    {
        m_masks[maskIndex].m_outputEvents = outputEvents;
    }

    void BlendTreeMaskNode::Mask::Reinit()
    {
        if (m_parent)
        {
            m_parent->Reinit();
        }
    }

    AZStd::string BlendTreeMaskNode::Mask::GetMaskName() const
    {
        return AZStd::string::format("GetMask %d", m_maskIndex);
    }

    AZStd::string BlendTreeMaskNode::Mask::GetOutputEventsName() const
    {
        return AZStd::string::format("Output Events %d", m_maskIndex);
    }

    void BlendTreeMaskNode::Mask::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Mask>()
                ->Version(1)
                ->Field("jointNames", &BlendTreeMaskNode::Mask::m_jointNames)
                ->Field("outputEvents", &BlendTreeMaskNode::Mask::m_outputEvents)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Mask>("Pose Mask Node", "Pose mask attributes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ_CRC_CE("ActorNodes"), &BlendTreeMaskNode::Mask::m_jointNames, "Mask", "The mask to apply.")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &BlendTreeMaskNode::Mask::GetMaskName)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMaskNode::Mask::Reinit)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMaskNode::Mask::m_outputEvents, "Output Events", "Output events.")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &BlendTreeMaskNode::Mask::GetOutputEventsName)
                    ;
            }
        }
    }

    void BlendTreeMaskNode::Reflect(AZ::ReflectContext* context)
    {
        BlendTreeMaskNode::Mask::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BlendTreeMaskNode, AnimGraphNode>()
                ->Version(1)
                ->Field("masks", &BlendTreeMaskNode::m_masks)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<BlendTreeMaskNode>("Pose Mask", "Pose mark attributes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMaskNode::m_masks, "Masks", "The mask to apply on the Pose 1 input port.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMaskNode::Reinit)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }
} // namespace EMotionFX
