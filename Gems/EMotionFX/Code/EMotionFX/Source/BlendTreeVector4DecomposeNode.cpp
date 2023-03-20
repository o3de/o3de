/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeVector4DecomposeNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeVector4DecomposeNode, AnimGraphAllocator)

    BlendTreeVector4DecomposeNode::BlendTreeVector4DecomposeNode()
        : AnimGraphNode()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPortAsVector4("Vector", INPUTPORT_VECTOR, PORTID_INPUT_VECTOR);

        // setup the output ports
        InitOutputPorts(4);
        SetupOutputPort("x", OUTPUTPORT_X, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_X);
        SetupOutputPort("y", OUTPUTPORT_Y, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_Y);
        SetupOutputPort("z", OUTPUTPORT_Z, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_Z);
        SetupOutputPort("w", OUTPUTPORT_W, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_W);
    }

    BlendTreeVector4DecomposeNode::~BlendTreeVector4DecomposeNode()
    {
    }

    bool BlendTreeVector4DecomposeNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* BlendTreeVector4DecomposeNode::GetPaletteName() const
    {
        return "Vector4 Decompose";
    }

    AnimGraphObject::ECategory BlendTreeVector4DecomposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }

    void BlendTreeVector4DecomposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeVector4DecomposeNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeVector4DecomposeNode::UpdateOutputPortValues(AnimGraphInstance* animGraphInstance)
    {
        // If there are no incoming connections, there is nothing to do.
        if (m_connections.size() == 0)
        {
            return;
        }

        AZ::Vector4 value = AZ::Vector4::CreateZero();
        TryGetInputVector4(animGraphInstance, INPUTPORT_VECTOR, value);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_X)->SetValue(value.GetX());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Y)->SetValue(value.GetY());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_Z)->SetValue(value.GetZ());
        GetOutputFloat(animGraphInstance, OUTPUTPORT_W)->SetValue(value.GetW());
    }

    void BlendTreeVector4DecomposeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeVector4DecomposeNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeVector4DecomposeNode>("Vector4 Decompose", "Vector4 decompose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX
