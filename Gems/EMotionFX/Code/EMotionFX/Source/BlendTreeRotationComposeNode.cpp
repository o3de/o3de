/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeRotationComposeNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRotationComposeNode, AnimGraphAllocator)

    BlendTreeRotationComposeNode::BlendTreeRotationComposeNode()
        : AnimGraphNode()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Rotation", OUTPUTPORT_ROTATION, MCore::AttributeQuaternion::TYPE_ID, PORTID_OUTPUT_ROTATION);
    }

    BlendTreeRotationComposeNode::~BlendTreeRotationComposeNode()
    {
    }

    void BlendTreeRotationComposeNode::Reinit()
    {
        RemoveInternalAttributesForAllInstances();
        m_inputPorts.clear();
        if (m_composeMode == ComposeMode::Euler)
        {
            // setup the input ports
            InitInputPorts(3);
            SetupInputPortAsNumber("pitch", INPUTPORT_PITCH, PORTID_INPUT_PITCH);
            SetupInputPortAsNumber("yaw", INPUTPORT_YAW, PORTID_INPUT_YAW);
            SetupInputPortAsNumber("roll", INPUTPORT_ROLL, PORTID_INPUT_ROLL);
        }
        else
        {
            // setup the input ports
            InitInputPorts(2);
            SetupInputPortAsVector3("axis", INPUTPORT_AXIS, PORTID_INPUT_AXIS);
            SetupInputPortAsNumber("angle", INPUTPORT_ANGLE, PORTID_INPUT_ANGLE);
        }

        InitInternalAttributesForAllInstances();

        AnimGraphNode::Reinit();
        SyncVisualObject();
    }

    bool BlendTreeRotationComposeNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* BlendTreeRotationComposeNode::GetPaletteName() const
    {
        return "Rotation Compose";
    }

    AnimGraphObject::ECategory BlendTreeRotationComposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }

    void BlendTreeRotationComposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeRotationComposeNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeRotationComposeNode::UpdateOutputPortValues(AnimGraphInstance* animGraphInstance)
    {
        if (m_composeMode == ComposeMode::Euler)
        {
            const float pitch = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_PITCH);
            const float yaw = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_YAW);
            const float roll = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_ROLL);
            GetOutputQuaternion(animGraphInstance, OUTPUTPORT_ROTATION)
                ->SetValue(AZ::Quaternion::CreateFromEulerDegreesZYX(AZ::Vector3(pitch, yaw, roll)));
        }
        else
        {
            AZ::Vector3 axis;
            if (!TryGetInputVector3(animGraphInstance, INPUTPORT_AXIS, axis))
            {
                return;
            }
            const float angle = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_ANGLE);
            GetOutputQuaternion(animGraphInstance, OUTPUTPORT_ROTATION)
                ->SetValue(AZ::Quaternion::CreateFromAxisAngle(axis, AZ::DegToRad(angle)));
        }
    }

    void BlendTreeRotationComposeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeRotationComposeNode, AnimGraphNode>()
            ->Version(1)
            ->Field("ComposeMode", &BlendTreeRotationComposeNode::m_composeMode)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeRotationComposeNode>("Rotation Compose", "Rotation compose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeRotationComposeNode::m_composeMode, "Compose Mode", "Compose mode")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeRotationComposeNode::Reinit)
            ->EnumAttribute(ComposeMode::Euler, "Euler")
            ->EnumAttribute(ComposeMode::AxisAngle, "Axis Angle")
            ;
    }
} // namespace EMotionFX
