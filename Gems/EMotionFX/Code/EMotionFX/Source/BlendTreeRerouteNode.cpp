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
        // Restore typed ports before the base class reconnects deserialized inputs.
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
            m_inputPorts.empty()
            || m_outputPorts.empty()
            || m_inputPorts[INPUTPORT_VALUE].m_compatibleTypes[0] != m_dataTypeId
            || m_outputPorts[OUTPUTPORT_VALUE].m_compatibleTypes[0] != m_dataTypeId;

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
        if (GetHasOutputPose())
        {
            return GetOutputPose(animGraphInstance, OUTPUTPORT_VALUE)->GetValue();
        }

        return nullptr;
    }

    void BlendTreeRerouteNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        AnimGraphNode* sourceNode = GetInputNode(INPUTPORT_VALUE);
        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);

        if (!GetHasOutputPose())
        {
            UpdateOutputValue(animGraphInstance);
        }

        if (!sourceNode)
        {
            uniqueData->Clear();
            return;
        }

        if (GetHasOutputPose())
        {
            uniqueData->Init(animGraphInstance, sourceNode);
        }
    }

    void BlendTreeRerouteNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode* sourceNode = GetInputNode(INPUTPORT_VALUE);
        OutputIncomingNode(animGraphInstance, sourceNode);

        if (!GetHasOutputPose())
        {
            UpdateOutputValue(animGraphInstance);
            return;
        }

        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_VALUE)->GetValue();
        if (sourceNode)
        {
            *outputPose = *GetInputPose(animGraphInstance, INPUTPORT_VALUE)->GetValue();
        }
        else
        {
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        }
    }

    void BlendTreeRerouteNode::UpdateOutputValue(AnimGraphInstance* animGraphInstance)
    {
        MCore::Attribute* outputValue = GetOutputAttribute(animGraphInstance, OUTPUTPORT_VALUE);
        const MCore::Attribute* inputValue = GetInputAttribute(animGraphInstance, INPUTPORT_VALUE);
        if (!inputValue)
        {
            inputValue = m_defaultValue;
        }

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
