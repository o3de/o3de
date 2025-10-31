/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeRangeRemapperNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRangeRemapperNode, AnimGraphAllocator)


    BlendTreeRangeRemapperNode::BlendTreeRangeRemapperNode()
        : AnimGraphNode()
        , m_inputMin(0.0f)
        , m_inputMax(1.0f)
        , m_outputMin(0.0f)
        , m_outputMax(1.0f)
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X); // accept float/int/bool values

                                                                  // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    BlendTreeRangeRemapperNode::~BlendTreeRangeRemapperNode()
    {
    }


    bool BlendTreeRangeRemapperNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* BlendTreeRangeRemapperNode::GetPaletteName() const
    {
        return "Range Remapper";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeRangeRemapperNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // the update function
    void BlendTreeRangeRemapperNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        const size_t numConnections = m_connections.size();
        if (numConnections == 0 || m_disabled)
        {
            if (numConnections > 0) // pass the input value as output in case we are disabled
            {
                //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_X) );
                GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X));
            }

            return;
        }

        // get the input value and the input and output ranges as a float, convert if needed
        //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_X) );
        float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);

        // output the original input, so without remapping, if this node is disabled
        if (m_disabled)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(x);
            return;
        }

        // clamp it so that the value is in the valid input range
        x = MCore::Clamp<float>(x, m_inputMin, m_inputMax);

        // apply the simple linear conversion
        float result;
        if (MCore::Math::Abs(m_inputMax - m_inputMin) > MCore::Math::epsilon)
        {
            result = ((x - m_inputMin) / (m_inputMax - m_inputMin)) * (m_outputMax - m_outputMin) + m_outputMin;
        }
        else
        {
            result = m_outputMin;
        }

        // update the output value
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(result);
    }


    void BlendTreeRangeRemapperNode::SetInputMin(float value)
    {
        m_inputMin = value;
    }

    void BlendTreeRangeRemapperNode::SetInputMax(float value)
    {
        m_inputMax = value;
    }

    void BlendTreeRangeRemapperNode::SetOutputMin(float value)
    {
        m_outputMin = value;
    }

    void BlendTreeRangeRemapperNode::SetOutputMax(float value)
    {
        m_outputMax = value;
    }

    void BlendTreeRangeRemapperNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeRangeRemapperNode, AnimGraphNode>()
            ->Version(1)
            ->Field("inputMin", &BlendTreeRangeRemapperNode::m_inputMin)
            ->Field("inputMax", &BlendTreeRangeRemapperNode::m_inputMax)
            ->Field("outputMin", &BlendTreeRangeRemapperNode::m_outputMin)
            ->Field("outputMax", &BlendTreeRangeRemapperNode::m_outputMax)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeRangeRemapperNode>("Range Remapper", "Range remapper attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeRangeRemapperNode::m_inputMin, "Input Min", "The minimum incoming value. Values smaller than this will be clipped.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeRangeRemapperNode::m_inputMax, "Input Max", "The maximum incoming value. Values bigger than this will be clipped.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeRangeRemapperNode::m_outputMin, "Output Min", "The minimum outcoming value. The minimum incoming value will be mapped to the minimum outcoming value. The output port can't hold a smaller value than this.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeRangeRemapperNode::m_outputMax, "Output Max", "The maximum outcoming value. The maximum incoming value will be mapped to the maximum outcoming value. The output port can't hold a bigger value than this.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ;
    }
} // namespace EMotionFX
