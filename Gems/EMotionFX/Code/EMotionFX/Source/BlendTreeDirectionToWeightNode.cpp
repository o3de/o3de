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
#include "BlendTreeDirectionToWeightNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeDirectionToWeightNode, AnimGraphAllocator)

    BlendTreeDirectionToWeightNode::BlendTreeDirectionToWeightNode()
        : AnimGraphNode()
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPort("Direction X", INPUTPORT_DIRECTION_X, MCore::AttributeFloat::TYPE_ID, PORTID_INPUT_DIRECTION_X);
        SetupInputPort("Direction Y", INPUTPORT_DIRECTION_Y, MCore::AttributeFloat::TYPE_ID, PORTID_INPUT_DIRECTION_Y);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Weight", OUTPUTPORT_WEIGHT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_WEIGHT);
    }


    BlendTreeDirectionToWeightNode::~BlendTreeDirectionToWeightNode()
    {
    }


    bool BlendTreeDirectionToWeightNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeDirectionToWeightNode::GetPaletteName() const
    {
        return "Direction To Weight";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeDirectionToWeightNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // the update function
    void BlendTreeDirectionToWeightNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are less than two incoming connections, there is nothing to do
        const size_t numConnections = m_connections.size();
        if (numConnections < 2 || m_disabled)
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_WEIGHT)->SetValue(0.0f);
            return;
        }

        // get the input values as a floats
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DIRECTION_X));
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DIRECTION_Y));

        float directionX = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DIRECTION_X);
        float directionY = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_DIRECTION_Y);

        // in case the direction is very close to the origin, default it to the x-axis
        if (MCore::Compare<float>::CheckIfIsClose(directionX, 0.0f, MCore::Math::epsilon) && MCore::Compare<float>::CheckIfIsClose(directionY, 0.0f, MCore::Math::epsilon))
        {
            directionX = 1.0f;
            directionY = 0.0f;
        }

        // convert it to a vector2 and normalize it
        AZ::Vector2 direction(directionX, directionY);
        direction.Normalize();

        // right side
        float alpha = 0.0f;
        if (directionX >= 0.0f)
        {
            // up-right
            if (directionY >= 0.0f)
            {
                alpha = MCore::Math::ACos(direction.GetX());
            }
            // down-right
            else
            {
                alpha = MCore::Math::twoPi - MCore::Math::ACos(direction.GetX());
            }
        }
        // left side
        else
        {
            // up-left
            if (directionY >= 0.0f)
            {
                alpha = MCore::Math::halfPi + MCore::Math::ACos(direction.GetY());
            }
            // down-left
            else
            {
                alpha = MCore::Math::pi + MCore::Math::ACos(-direction.GetX());
            }
        }

        // rescale from the radian circle to a normalized value
        const float result = alpha / MCore::Math::twoPi;

        // update the output value
        GetOutputFloat(animGraphInstance, OUTPUTPORT_WEIGHT)->SetValue(result);
    }


    AZ::Color BlendTreeDirectionToWeightNode::GetVisualColor() const
    {
        return AZ::Color(0.2f, 0.78f, 0.2f, 1.0f);
    }


    void BlendTreeDirectionToWeightNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeDirectionToWeightNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeDirectionToWeightNode>("Direction To Weight", "Direction to weight attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX

