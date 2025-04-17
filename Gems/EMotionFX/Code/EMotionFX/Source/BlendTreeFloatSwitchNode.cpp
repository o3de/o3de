/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "BlendTreeFloatSwitchNode.h"
#include <MCore/Source/Random.h>
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFloatSwitchNode, AnimGraphAllocator)

    BlendTreeFloatSwitchNode::BlendTreeFloatSwitchNode()
        : AnimGraphNode()
        , m_value0(0.0f)
        , m_value1(0.0f)
        , m_value2(0.0f)
        , m_value3(0.0f)
        , m_value4(0.0f)
    {
        // create the input ports
        InitInputPorts(6);
        SetupInputPortAsNumber("Case 0", INPUTPORT_0, PORTID_INPUT_0);  // accept float/int/bool values
        SetupInputPortAsNumber("Case 1", INPUTPORT_1, PORTID_INPUT_1);  // accept float/int/bool values
        SetupInputPortAsNumber("Case 2", INPUTPORT_2, PORTID_INPUT_2);  // accept float/int/bool values
        SetupInputPortAsNumber("Case 3", INPUTPORT_3, PORTID_INPUT_3);  // accept float/int/bool values
        SetupInputPortAsNumber("Case 4", INPUTPORT_4, PORTID_INPUT_4);  // accept float/int/bool values
        SetupInputPortAsNumber("Decision Value", INPUTPORT_DECISION, PORTID_INPUT_DECISION);

        // create the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    BlendTreeFloatSwitchNode::~BlendTreeFloatSwitchNode()
    {
    }


    bool BlendTreeFloatSwitchNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeFloatSwitchNode::GetPaletteName() const
    {
        return "Float Switch";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFloatSwitchNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_LOGIC;
    }


    // the update function
    void BlendTreeFloatSwitchNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if the decision port has no incomming connection, there is nothing we can do
        if (m_inputPorts[INPUTPORT_DECISION].m_connection == nullptr)
        {
            return;
        }

        // get the index we choose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DECISION));
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISION), 0, 4); // max 5 cases

        // return the value for that port
        if (m_inputPorts[INPUTPORT_0 + decisionValue].m_connection)
        {
            //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_0 + decisionValue) );
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_0 + decisionValue));
        }
        else
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(GetValue(decisionValue));
        }
    }


    AZ::Color BlendTreeFloatSwitchNode::GetVisualColor() const
    {
        return AZ::Color(0.2f, 0.78f, 0.2f, 1.0f);
    }


    float BlendTreeFloatSwitchNode::GetValue(uint32 index) const
    {
        switch (index)
        {
        case 0:
            return m_value0;
        case 1:
            return m_value1;
        case 2:
            return m_value2;
        case 3:
            return m_value3;
        case 4:
            return m_value4;
        }

        AZ_Assert(false, "Cannot get value for float switch node. Index %i out of range.", index);
        return 0.0f;
    }

    void BlendTreeFloatSwitchNode::SetValue0(float value0)
    {
        m_value0 = value0;
    }

    void BlendTreeFloatSwitchNode::SetValue1(float value1)
    {
        m_value1 = value1;
    }

    void BlendTreeFloatSwitchNode::SetValue2(float value2)
    {
        m_value2 = value2;
    }

    void BlendTreeFloatSwitchNode::SetValue3(float value3)
    {
        m_value3 = value3;
    }

    void BlendTreeFloatSwitchNode::SetValue4(float value4)
    {
        m_value4 = value4;
    }

    void BlendTreeFloatSwitchNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeFloatSwitchNode, AnimGraphNode>()
            ->Version(1)
            ->Field("value0", &BlendTreeFloatSwitchNode::m_value0)
            ->Field("value1", &BlendTreeFloatSwitchNode::m_value1)
            ->Field("value2", &BlendTreeFloatSwitchNode::m_value2)
            ->Field("value3", &BlendTreeFloatSwitchNode::m_value3)
            ->Field("value4", &BlendTreeFloatSwitchNode::m_value4)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeFloatSwitchNode>("Float Math2", "Float Math2 attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatSwitchNode::m_value0, "Value Case 0", "Value used for case 0, when it has no input.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatSwitchNode::m_value1, "Value Case 1", "Value used for case 1, when it has no input.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatSwitchNode::m_value2, "Value Case 2", "Value used for case 2, when it has no input.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatSwitchNode::m_value3, "Value Case 3", "Value used for case 3, when it has no input.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatSwitchNode::m_value4, "Value Case 4", "Value used for case 4, when it has no input.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ;
    }
} // namespace EMotionFX
