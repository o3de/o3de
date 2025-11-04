/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeRotationDecomposeNode.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRotationDecomposeNode, AnimGraphAllocator)

    BlendTreeRotationDecomposeNode::BlendTreeRotationDecomposeNode()
        : AnimGraphNode()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPort("Rotation", INPUTPORT_ROTATION, MCore::AttributeQuaternion::TYPE_ID, PORTID_INPUT_ROTATION);
    }

    BlendTreeRotationDecomposeNode::~BlendTreeRotationDecomposeNode()
    {
    }

    void BlendTreeRotationDecomposeNode::Reinit()
    {
        RemoveInternalAttributesForAllInstances();
        m_outputPorts.clear();
        if (m_decomposeMode == DecomposeMode::Euler)
        {
            // setup the output ports
            InitOutputPorts(3);
            SetupOutputPort("pitch", OUTPUTPORT_PITCH, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_PITCH);
            SetupOutputPort("yaw", OUTPUTPORT_YAW, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_YAW);
            SetupOutputPort("roll", OUTPUTPORT_ROLL, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_ROLL);
        }
        else
        {
            // setup the output ports
            InitOutputPorts(2);
            SetupOutputPort("axis", OUTPUTPORT_AXIS, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_AXIS);
            SetupOutputPort("angle", OUTPUTPORT_ANGLE, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_ANGLE);
        }

        InitInternalAttributesForAllInstances();

        AnimGraphNode::Reinit();
        SyncVisualObject();
    }

    bool BlendTreeRotationDecomposeNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* BlendTreeRotationDecomposeNode::GetPaletteName() const
    {
        return "Rotation Decompose";
    }

    AnimGraphObject::ECategory BlendTreeRotationDecomposeNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }

    void BlendTreeRotationDecomposeNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeRotationDecomposeNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
        UpdateOutputPortValues(animGraphInstance);
    }

    void BlendTreeRotationDecomposeNode::UpdateOutputPortValues(AnimGraphInstance* animGraphInstance)
    {
        AZ::Quaternion quat;
        MCore::AttributeQuaternion* attr = GetInputQuaternion(animGraphInstance, INPUTPORT_ROTATION);
        if (!attr)
        {
            return;
        }

        quat = attr->GetValue();
        if (m_decomposeMode == DecomposeMode::Euler)
        {
            AZ::Vector3 value = quat.GetEulerDegreesZYX();
            GetOutputFloat(animGraphInstance, OUTPUTPORT_PITCH)->SetValue(value.GetX());
            GetOutputFloat(animGraphInstance, OUTPUTPORT_YAW)->SetValue(value.GetY());
            GetOutputFloat(animGraphInstance, OUTPUTPORT_ROLL)->SetValue(value.GetZ());
        }
        else
        {
            AZ::Vector3 axis;
            float angle;
            quat.ConvertToAxisAngle(axis, angle);
            GetOutputVector3(animGraphInstance, OUTPUTPORT_AXIS)->SetValue(axis);
            GetOutputFloat(animGraphInstance, OUTPUTPORT_ANGLE)->SetValue(AZ::RadToDeg(angle));
        }
    }

    void BlendTreeRotationDecomposeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeRotationDecomposeNode, AnimGraphNode>()
            ->Version(1)
            ->Field("DecomposeMode", &BlendTreeRotationDecomposeNode::m_decomposeMode)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeRotationDecomposeNode>("Rotation Decompose", "Rotation decompose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeRotationDecomposeNode::m_decomposeMode, "Decompose Mode", "Decompose mode")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeRotationDecomposeNode::Reinit)
            ->EnumAttribute(DecomposeMode::Euler, "Euler")
            ->EnumAttribute(DecomposeMode::AxisAngle, "Axis Angle")
            ;
    }
} // namespace EMotionFX
