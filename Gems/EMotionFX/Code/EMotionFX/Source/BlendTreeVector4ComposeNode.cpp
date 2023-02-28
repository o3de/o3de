/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeVector4ComposeNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeVector4ComposeNode, AnimGraphAllocator)

    BlendTreeVector4ComposeNode::BlendTreeVector4ComposeNode()
        : AnimGraphNode()
    {
        // setup the input ports
        InitInputPorts(4);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X);
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y);
        SetupInputPortAsNumber("z", INPUTPORT_Z, PORTID_INPUT_Z);
        SetupInputPortAsNumber("w", INPUTPORT_W, PORTID_INPUT_W);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Vector", OUTPUTPORT_VECTOR, MCore::AttributeVector4::TYPE_ID, PORTID_OUTPUT_VECTOR);
    }

    BlendTreeVector4ComposeNode::~BlendTreeVector4ComposeNode()
    {
    }

    bool BlendTreeVector4ComposeNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* BlendTreeVector4ComposeNode::GetPaletteName() const
    {
        return "Vector4 Compose";
    }

    AnimGraphObject::ECategory BlendTreeVector4ComposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }

    void BlendTreeVector4ComposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeVector4ComposeNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeVector4ComposeNode::UpdateOutputPortValues(AnimGraphInstance* animGraphInstance)
    {
        const float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
        const float y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
        const float z = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Z);
        const float w = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_W);
        GetOutputVector4(animGraphInstance, OUTPUTPORT_VECTOR)->SetValue(AZ::Vector4(x, y, z, w));
    }

    void BlendTreeVector4ComposeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeVector4ComposeNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeVector4ComposeNode>("Vector4 Compose", "Vector4 compose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX
