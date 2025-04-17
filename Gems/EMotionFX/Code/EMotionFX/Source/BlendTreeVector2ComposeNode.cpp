/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeVector2ComposeNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeVector2ComposeNode, AnimGraphAllocator)

    BlendTreeVector2ComposeNode::BlendTreeVector2ComposeNode()
        : AnimGraphNode()
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X);
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Vector", OUTPUTPORT_VECTOR, MCore::AttributeVector2::TYPE_ID, PORTID_OUTPUT_VECTOR);
    }

    BlendTreeVector2ComposeNode::~BlendTreeVector2ComposeNode()
    {
    }

    bool BlendTreeVector2ComposeNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* BlendTreeVector2ComposeNode::GetPaletteName() const
    {
        return "Vector2 Compose";
    }

    AnimGraphObject::ECategory BlendTreeVector2ComposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }

    void BlendTreeVector2ComposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeVector2ComposeNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeVector2ComposeNode::UpdateOutputPortValues(AnimGraphInstance* animGraphInstance)
    {
        const float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
        const float y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
        GetOutputVector2(animGraphInstance, OUTPUTPORT_VECTOR)->SetValue(AZ::Vector2(x, y));
    }

    void BlendTreeVector2ComposeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeVector2ComposeNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeVector2ComposeNode>("Vector2 compose", "Vector2 compose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX
