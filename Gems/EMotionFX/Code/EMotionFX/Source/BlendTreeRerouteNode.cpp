/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/AttributeFloat.h>

#include "AnimGraphAttributeTypes.h"
#include "BlendTreeRerouteNode.h"

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRerouteNode, AnimGraphAllocator)

    BlendTreeRerouteNode::BlendTreeRerouteNode()
        : AnimGraphNode()
        , m_dataTypeId(MCore::AttributeFloat::TYPE_ID)
    {
        ConfigurePorts();
    }

    BlendTreeRerouteNode::~BlendTreeRerouteNode()
    {
        delete m_defaultValue;
    }

    void BlendTreeRerouteNode::ConfigurePorts()
    {
        delete m_defaultValue;
        m_defaultValue = nullptr;

        m_inputPorts.clear();
        InitInputPorts(1);
        SetupInputPort("In", INPUTPORT_VALUE, m_dataTypeId, PORTID_INPUT_VALUE);

        m_outputPorts.clear();
        InitOutputPorts(1);
        if (GetHasOutputPose())
        {
            SetupOutputPortAsPose("Out", OUTPUTPORT_VALUE, PORTID_OUTPUT_VALUE);
        }
        else
        {
            SetupOutputPort("Out", OUTPUTPORT_VALUE, m_dataTypeId, PORTID_OUTPUT_VALUE);
            m_defaultValue = MCore::GetAttributeFactory().CreateAttributeByType(m_dataTypeId);
        }
    }

    bool BlendTreeRerouteNode::InitAfterLoading(AnimGraph* animGraph)
    {
        // Configure the ports from the deserialized type before the base class restores
        // each connection's pointer into its target input port.
        ConfigurePorts();

        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();
        Reinit();
        return true;
    }

    void BlendTreeRerouteNode::Reinit()
    {
        const bool portsNeedReconfiguration =
            m_inputPorts.empty() || m_outputPorts.empty() ||
            m_inputPorts[INPUTPORT_VALUE].m_compatibleTypes[0] != m_dataTypeId ||
            m_outputPorts[OUTPUTPORT_VALUE].m_compatibleTypes[0] != m_dataTypeId;

        if (portsNeedReconfiguration)
        {
            if (m_animGraph)
            {
                RemoveInternalAttributesForAllInstances();
            }

            ConfigurePorts();

            if (m_animGraph)
            {
                InitInternalAttributesForAllInstances();
            }
        }

        AnimGraphNode::Reinit();
    }

    bool BlendTreeRerouteNode::GetHasOutputPose() const
    {
        return m_dataTypeId == AttributePose::TYPE_ID;
    }

    AnimGraphPose* BlendTreeRerouteNode::GetMainOutputPose(AnimGraphInstance* animGraphInstance) const
    {
        return GetHasOutputPose() ? GetOutputPose(animGraphInstance, OUTPUTPORT_VALUE)->GetValue() : nullptr;
    }

    void BlendTreeRerouteNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        AnimGraphNode* sourceNode = GetInputNode(INPUTPORT_VALUE);
        if (!sourceNode)
        {
            uniqueData->Clear();
            return;
        }

        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);
        if (sourceNode->GetHasOutputPose())
        {
            uniqueData->Init(animGraphInstance, sourceNode);
        }
    }

    void BlendTreeRerouteNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode* sourceNode = GetInputNode(INPUTPORT_VALUE);
        if (GetHasOutputPose())
        {
            if (!sourceNode)
            {
                RequestPoses(animGraphInstance);
                AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_VALUE)->GetValue();
                outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
                return;
            }

            OutputIncomingNode(animGraphInstance, sourceNode);
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_VALUE)->GetValue();
            *outputPose = *sourceNode->GetMainOutputPose(animGraphInstance);
            return;
        }

        MCore::Attribute* outputValue = GetOutputAttribute(animGraphInstance, OUTPUTPORT_VALUE);
        if (!sourceNode)
        {
            if (outputValue && m_defaultValue)
            {
                outputValue->InitFrom(m_defaultValue);
            }
            return;
        }

        OutputIncomingNode(animGraphInstance, sourceNode);
        MCore::Attribute* inputValue = GetInputAttribute(animGraphInstance, INPUTPORT_VALUE);
        if (outputValue && inputValue)
        {
            outputValue->InitFrom(inputValue);
        }
    }

    void BlendTreeRerouteNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeRerouteNode, AnimGraphNode>()
            ->Version(1)
            ->Field("dataTypeId", &BlendTreeRerouteNode::m_dataTypeId);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<BlendTreeRerouteNode>("Reroute", "Passes a blend tree value or pose through unchanged.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    }
} // namespace EMotionFX
