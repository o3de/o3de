/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeRotationMath2Node.h"
#include <MCore/Source/FastMath.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRotationMath2Node, AnimGraphAllocator)


    BlendTreeRotationMath2Node::BlendTreeRotationMath2Node()
    : AnimGraphNode()
    , m_mathFunction(MATHFUNCTION_MULTIPLY)
    , m_defaultValue(AZ::Quaternion::CreateIdentity())
    {
        // Setup the input ports
        InitInputPorts(2);
        SetupInputPort("x", INPUTPORT_X, MCore::AttributeQuaternion::TYPE_ID, PORTID_INPUT_X);
        SetupInputPort("y", INPUTPORT_Y, MCore::AttributeQuaternion::TYPE_ID, PORTID_INPUT_Y);

        // Setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Rotation", INPUTPORT_X, MCore::AttributeQuaternion::TYPE_ID, PORTID_OUTPUT_QUATERNION);

        if (m_animGraph)
        {
            Reinit();
        }
    }


    BlendTreeRotationMath2Node::~BlendTreeRotationMath2Node()
    {
    }


    void BlendTreeRotationMath2Node::Reinit()
    {
        switch (m_mathFunction)
        {
        case MATHFUNCTION_MULTIPLY:
            m_calculateFunc = CalculateMultiply;
            SetNodeInfo("Rotate");
            break;
        case MATHFUNCTION_INVERSE_MULTIPLY:
            m_calculateFunc = CalculateInverseMultiply;
            SetNodeInfo("Inverse rotate");
            break;
        default:
            AZ_Assert(false, "EMotionFX: Math function unknown.");
        }

        AnimGraphNode::Reinit();
    }


    bool BlendTreeRotationMath2Node::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* BlendTreeRotationMath2Node::GetPaletteName() const
    {
        return "Rotation Math2";
    }


    AnimGraphObject::ECategory BlendTreeRotationMath2Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }

    void BlendTreeRotationMath2Node::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode::Output(animGraphInstance);
        ExecuteMathLogic(animGraphInstance);
    }

    void BlendTreeRotationMath2Node::ExecuteMathLogic(EMotionFX::AnimGraphInstance * animGraphInstance)
    {
        // If there are no incoming connections, there is nothing to do
        if (m_connections.empty())
        {
            return;
        }

        // If both x and y inputs have connections
        AZ::Quaternion x = m_defaultValue;
        AZ::Quaternion y = x;
        if (m_connections.size() == 2)
        {

            x = GetInputQuaternion(animGraphInstance, INPUTPORT_X)->GetValue();
            y = GetInputQuaternion(animGraphInstance, INPUTPORT_Y)->GetValue();
        }
        else // Only x or y is connected
        {
            // If only x has something plugged in
            if (m_connections[0]->GetTargetPort() == INPUTPORT_X)
            {
                x = GetInputQuaternion(animGraphInstance, INPUTPORT_X)->GetValue();
            }
            else // Only y has an input
            {
                MCORE_ASSERT(m_connections[0]->GetTargetPort() == INPUTPORT_Y);
                y = GetInputQuaternion(animGraphInstance, INPUTPORT_Y)->GetValue();
            }
        }

        // Apply the operation
        AZ::Quaternion quaternionResult;
        m_calculateFunc(x, y, &quaternionResult);

        // Update the output value
        GetOutputQuaternion(animGraphInstance, OUTPUTPORT_RESULT_QUATERNION)->SetValue(quaternionResult);
    }

    void BlendTreeRotationMath2Node::SetMathFunction(EMathFunction func)
    {
        m_mathFunction = func;
        if (m_animGraph)
        {
            Reinit();
        }
    }

    //-----------------------------------------------
    // The math functions
    //-----------------------------------------------
    // Multiply
    void BlendTreeRotationMath2Node::CalculateMultiply(const AZ::Quaternion& inputA, const AZ::Quaternion& inputB, AZ::Quaternion* quaternionOutput)
    {
        *quaternionOutput = inputA * inputB;
    }

    // Inverse Multiply : X * Y^(-1)
    void BlendTreeRotationMath2Node::CalculateInverseMultiply(const AZ::Quaternion& inputA, const AZ::Quaternion& inputB, AZ::Quaternion* quaternionOutput)
    {
        *quaternionOutput = inputA * inputB.GetInverseFull();
    }

    void BlendTreeRotationMath2Node::SetDefaultValue(const AZ::Quaternion& value)
    {
        m_defaultValue = value;
    }

    void BlendTreeRotationMath2Node::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeRotationMath2Node, AnimGraphNode>()
            ->Version(1)
            ->Field("mathFunction", &BlendTreeRotationMath2Node::m_mathFunction)
            ->Field("defaultValue", &BlendTreeRotationMath2Node::m_defaultValue)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeRotationMath2Node>("Rotation Math2", "Rotation Math2 attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeRotationMath2Node::m_mathFunction, "Math Function", "The math function to use.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeRotationMath2Node::Reinit)
            ->EnumAttribute(MATHFUNCTION_MULTIPLY, "Rotate")
            ->EnumAttribute(MATHFUNCTION_INVERSE_MULTIPLY, "Inverse Rotate")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeRotationMath2Node::m_defaultValue, "Default Value", "The default value for x or y when one of them has no incomming connection.")
            ->Attribute(AZ::Edit::Attributes::Suffix, " Deg")
            ;
    }
} // namespace EMotionFX
