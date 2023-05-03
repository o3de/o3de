/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFloatConstantNode, AnimGraphAllocator)

    BlendTreeFloatConstantNode::BlendTreeFloatConstantNode()
        : AnimGraphNode()
        , m_value(0.0f)
    {
        InitOutputPorts(1);
        SetupOutputPort("Output", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    BlendTreeFloatConstantNode::~BlendTreeFloatConstantNode()
    {
    }


    void BlendTreeFloatConstantNode::Reinit()
    {
        AZStd::string valueString = AZStd::string::format("%.6f", m_value);

        // Remove the zero's at the end.
        const size_t dotZeroOffset = valueString.find(".0");
        if (AzFramework::StringFunc::Strip(valueString, "0", false, false, true))   // If something got stripped.
        {
            // If it was say "5.0" and we removed all zeros at the end, we are left with "5.".
            // Turn that into 5.0 again by adding the 0.
            // We detect this by checking if we can still find a ".0" after the stripping of zeros.
            // If there was a ".0" in the string before, and now there isn't, we have to add the 0 back to it.
            const size_t dotZeroOffsetAfterTrim = valueString.find(".0");
            if (dotZeroOffset != AZStd::string::npos && dotZeroOffsetAfterTrim == AZStd::string::npos)
                valueString += "0";
        }

        SetNodeInfo(valueString.c_str());

        AnimGraphNode::Reinit();
    }


    bool BlendTreeFloatConstantNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* BlendTreeFloatConstantNode::GetPaletteName() const
    {
        return "Float Constant";
    }


    AnimGraphObject::ECategory BlendTreeFloatConstantNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    void BlendTreeFloatConstantNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AZ_UNUSED(timePassedInSeconds);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(m_value);
    }


    AZ::Color BlendTreeFloatConstantNode::GetVisualColor() const
    {
        return AZ::Color(0.5f, 1.0f, 1.0f, 1.0f);
    }


    bool BlendTreeFloatConstantNode::GetSupportsDisable() const
    {
        return false;
    }

    void BlendTreeFloatConstantNode::SetValue(float value)
    {
        m_value = value;
    }

    void BlendTreeFloatConstantNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeFloatConstantNode, AnimGraphNode>()
            ->Version(1)
            -> Field("value", &BlendTreeFloatConstantNode::m_value)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeFloatConstantNode>("Float Constant", "Float constant attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatConstantNode::m_value, "Constant Value", "The value that the node will output.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFloatConstantNode::Reinit)
            ;
    }
} // namespace EMotionFX
