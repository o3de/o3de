/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/MathUtils.h>
#include "BlendTreeSmoothingNode.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"

#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSmoothingNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSmoothingNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeSmoothingNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeSmoothingNode::UniqueData::Update()
    {
        BlendTreeSmoothingNode* smoothingNode = azdynamic_cast<BlendTreeSmoothingNode*>(m_object);
        AZ_Assert(smoothingNode, "Unique data linked to incorrect node type.");

        if (!smoothingNode->GetInputNode(BlendTreeSmoothingNode::INPUTPORT_DEST))
        {
            m_currentValue = 0.0f;
        }
    }

    BlendTreeSmoothingNode::BlendTreeSmoothingNode()
        : AnimGraphNode()
    {
        // create the input ports
        InitInputPorts(1);
        SetupInputPortAsNumber("Dest", INPUTPORT_DEST, PORTID_INPUT_DEST);

        // create the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);
    }


    BlendTreeSmoothingNode::~BlendTreeSmoothingNode()
    {
    }


    bool BlendTreeSmoothingNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeSmoothingNode::GetPaletteName() const
    {
        return "Smoothing";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeSmoothingNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // update
    void BlendTreeSmoothingNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all incoming nodes
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        // if there are no incoming connections, there is nothing to do
        if (m_connections.size() == 0)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(0.0f);
            return;
        }

        // if we are disabled, output the dest value directly
        //OutputIncomingNode( animGraphInstance, GetInputNode(INPUTPORT_DEST) );
        const float destValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DEST);
        if (m_disabled)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(destValue);
            return;
        }

        // perform interpolation
        const float sourceValue = uniqueData->m_currentValue;
        const float interpolationSpeed = m_interpolationSpeed * uniqueData->m_frameDeltaTime * 10.0f;
        const float interpolationResult = (interpolationSpeed < 0.99999f) ? AZ::Lerp(sourceValue, destValue, interpolationSpeed) : destValue;
        // If the interpolation result is close to the dest value within the tolerance, snap to the destination value.
        if (AZ::IsClose((interpolationResult - destValue), 0.0f, m_snapTolerance))
        {
            uniqueData->m_currentValue = destValue;
        }
        else
        {
            // pass the interpolated result to the output port and the current value of the unique data
            uniqueData->m_currentValue = interpolationResult;
        }
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(interpolationResult);

        uniqueData->m_frameDeltaTime = timePassedInSeconds;
    }


    // rewind
    void BlendTreeSmoothingNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<BlendTreeSmoothingNode::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        // check if the current value needs to be reset to the input or the start value when rewinding the node
        if (m_useStartValue)
        {
            uniqueData->m_currentValue = m_startValue;
        }
        else
        {
            // set the current value to the current input value
            UpdateAllIncomingNodes(animGraphInstance, 0.0f);
            uniqueData->m_currentValue = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DEST);
        }
    }

    AZ::Color BlendTreeSmoothingNode::GetVisualColor() const
    {
        return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
    }


    bool BlendTreeSmoothingNode::GetSupportsDisable() const
    {
        return true;
    }


    AZ::Crc32 BlendTreeSmoothingNode::GetStartValueVisibility() const
    {
        return m_useStartValue ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void BlendTreeSmoothingNode::SetInterpolationSpeed(float interpolationSpeed)
    {
        m_interpolationSpeed = interpolationSpeed;
    }

    void BlendTreeSmoothingNode::SetStartVAlue(float startValue)
    {
        m_startValue = startValue;
    }

    void BlendTreeSmoothingNode::SetUseStartVAlue(bool useStartValue)
    {
        m_useStartValue = useStartValue;
    }


    void BlendTreeSmoothingNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeSmoothingNode, AnimGraphNode>()
            ->Version(2)
            ->Field("interpolationSpeed", &BlendTreeSmoothingNode::m_interpolationSpeed)
            ->Field("useStartValue", &BlendTreeSmoothingNode::m_useStartValue)
            ->Field("startValue", &BlendTreeSmoothingNode::m_startValue)
            ->Field("snapTolerance", &BlendTreeSmoothingNode::m_snapTolerance)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeSmoothingNode>("Smoothing", "Smoothing attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Slider, &BlendTreeSmoothingNode::m_interpolationSpeed, "Interpolation Speed", "Specifies how fast the output value moves towards the input value. Higher values make it move faster.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeSmoothingNode::m_useStartValue, "Use Start Value", "Enable this to use the start value, otherwise the first input value will be used as start value.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeSmoothingNode::m_startValue, "Start Value", "When the blend tree gets activated the smoothing node will start interpolating from this value.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeSmoothingNode::GetStartValueVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeSmoothingNode::m_snapTolerance, "Snap Tolerance", "If the current value is within the tolerance from the destination value, the smoothing node output will snap to the destination value.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
        ;
    }
} // namespace EMotionFX
