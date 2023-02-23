/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeVector2DecomposeNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeVector2DecomposeNode, AnimGraphAllocator)

    BlendTreeVector2DecomposeNode::BlendTreeVector2DecomposeNode()
        : AnimGraphNode()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPortAsVector2("Vector", INPUTPORT_VECTOR, PORTID_INPUT_VECTOR);

        // setup the output ports
        InitOutputPorts(2);
        SetupOutputPort("x", OUTPUTPORT_X, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_X);
        SetupOutputPort("y", OUTPUTPORT_Y, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_Y);
    }
        
    BlendTreeVector2DecomposeNode::~BlendTreeVector2DecomposeNode()
    {
    }

    bool BlendTreeVector2DecomposeNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* BlendTreeVector2DecomposeNode::GetPaletteName() const
    {
        return "Vector2 Decompose";
    }

    AnimGraphObject::ECategory BlendTreeVector2DecomposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }

    void BlendTreeVector2DecomposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeVector2DecomposeNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeVector2DecomposeNode::UpdateOutputPortValues(AnimGraphInstance* animGraphInstance)
    {
        AZ::Vector2 value = AZ::Vector2::CreateZero();
        if (!TryGetInputVector2(animGraphInstance, INPUTPORT_VECTOR, value))
        {
            return;
        }

        GetOutputFloat(animGraphInstance, OUTPUTPORT_X)->SetValue(value.GetX());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Y)->SetValue(value.GetY());
    }

    void BlendTreeVector2DecomposeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeVector2DecomposeNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeVector2DecomposeNode>("Vector2 Decompose", "Vector2 decompose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX
