/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeRotationLimitNode.h"
#include <MCore/Source/FastMath.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/ConstraintTransformRotationAngles.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRotationLimitNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRotationLimitNode::RotationLimit, AnimGraphAllocator)


    const float BlendTreeRotationLimitNode::RotationLimit::s_rotationLimitRangeMin = -360.0f;
    const float BlendTreeRotationLimitNode::RotationLimit::s_rotationLimitRangeMax = 360.0f;

    BlendTreeRotationLimitNode::RotationLimit::RotationLimit(BlendTreeRotationLimitNode::RotationLimit::Axis axis)
        : m_axis(axis)
    {}

    void BlendTreeRotationLimitNode::RotationLimit::SetMin(float min)
    {
        m_min = min;
    }

    void BlendTreeRotationLimitNode::RotationLimit::SetMax(float max)
    {
        m_max = max;
    }

    const char* BlendTreeRotationLimitNode::RotationLimit::GetLabel() const
    {
        const char* label = nullptr;
        switch (m_axis)
        {
        case RotationLimit::Axis::AXIS_X:
            label = "<font color='red'>X</font>";
            break;
        case RotationLimit::Axis::AXIS_Y:
            label = "<font color='green'>Y</font>";
            break;
        case RotationLimit::Axis::AXIS_Z:
            label = "<font color='blue'>Z</font>";
            break;
        default:
            AZ_Assert(false, "Unknown axis type for Rotation limit");
            label = "";
            break;
        }

        return label;
    }

    const BlendTreeRotationLimitNode::RotationLimit& BlendTreeRotationLimitNode::GetRotationLimitX() const
    {
        return m_rotationLimits[RotationLimit::Axis::AXIS_X];
    }

    const BlendTreeRotationLimitNode::RotationLimit& BlendTreeRotationLimitNode::GetRotationLimitY() const
    {
        return m_rotationLimits[RotationLimit::Axis::AXIS_Y];
    }
    const BlendTreeRotationLimitNode::RotationLimit& BlendTreeRotationLimitNode::GetRotationLimitZ() const
    {
        return m_rotationLimits[RotationLimit::Axis::AXIS_Z];
    }

    float BlendTreeRotationLimitNode::RotationLimit::GetLimitMin() const
    {
        return m_min;
    }

    float BlendTreeRotationLimitNode::RotationLimit::GetLimitMax() const
    {
        return m_max;
    }

    void BlendTreeRotationLimitNode::RotationLimit::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<RotationLimit>()
            ->Version(1)
            ->Field("min", &RotationLimit::m_min)
            ->Field("max", &RotationLimit::m_max)
            ->Field("axis", &RotationLimit::m_axis)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<RotationLimit>("Rotation limit", "Rotation limit")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, &RotationLimit::GetLabel);
    }

    BlendTreeRotationLimitNode::BlendTreeRotationLimitNode()
    : AnimGraphNode()
    {
        InitInputPorts(1);
        SetupInputPort("Input Rotation", INPUTPORT_ROTATION, MCore::AttributeQuaternion::TYPE_ID, PORTID_INPUT);

        InitOutputPorts(1);
        SetupOutputPort("Rotation", OUTPUTPORT_RESULT_QUATERNION, MCore::AttributeQuaternion::TYPE_ID, PORTID_OUTPUT_QUATERNION);
    }

    bool BlendTreeRotationLimitNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* BlendTreeRotationLimitNode::GetPaletteName() const
    {
        return "Rotation Limit";
    }


    AnimGraphObject::ECategory BlendTreeRotationLimitNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }

    void BlendTreeRotationLimitNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode::Output(animGraphInstance);
        ExecuteMathLogic(animGraphInstance);
    }

    void BlendTreeRotationLimitNode::ExecuteMathLogic(EMotionFX::AnimGraphInstance * animGraphInstance)
    {
        // If there are no incoming connections, there is nothing to do
        if (m_connections.empty())
        {
            return;
        }

        m_constraintTransformRotationAngles.SetTwistAxis(m_twistAxis);
        m_constraintTransformRotationAngles.GetTransform().m_rotation = GetInputQuaternion(animGraphInstance, INPUTPORT_ROTATION)->GetValue();

        m_constraintTransformRotationAngles.SetMaxRotationAngles(AZ::Vector2(GetRotationLimitY().m_max, GetRotationLimitX().m_max));
        m_constraintTransformRotationAngles.SetMinRotationAngles(AZ::Vector2(GetRotationLimitY().m_min, GetRotationLimitX().m_min));
        m_constraintTransformRotationAngles.SetMinTwistAngle(GetRotationLimitZ().m_min);
        m_constraintTransformRotationAngles.SetMaxTwistAngle(GetRotationLimitZ().m_max);
        m_constraintTransformRotationAngles.Execute();

        GetOutputQuaternion(animGraphInstance, OUTPUTPORT_RESULT_QUATERNION)->SetValue(m_constraintTransformRotationAngles.GetTransform().m_rotation);
    }

    void BlendTreeRotationLimitNode::Reflect(AZ::ReflectContext* context)
    {
        RotationLimit::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeRotationLimitNode, AnimGraphNode>()
            ->Version(1)
            ->Field("rotationLimits", &BlendTreeRotationLimitNode::m_rotationLimits)
            ->Field("twistAxis", &BlendTreeRotationLimitNode::m_twistAxis)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeRotationLimitNode>("Rotation Math2", "Rotation Math2 attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeRotationLimitNode::m_twistAxis, "Twist axis", "The twist axis to calculate the rotation limits")
            ->DataElement(AZ_CRC_CE("BlendTreeRotationLimitContainerHandler"), &BlendTreeRotationLimitNode::m_rotationLimits, "Rotation limits", "Rotation limits")
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ_CRC_CE("BlendTreeRotationLimitHandler"))
            ;
    }

    void BlendTreeRotationLimitNode::SetRotationLimitsX(float min, float max)
    {
        m_rotationLimits[RotationLimit::Axis::AXIS_X].m_min = min;
        m_rotationLimits[RotationLimit::Axis::AXIS_X].m_max = max;
    }

    void BlendTreeRotationLimitNode::SetRotationLimitsY(float min, float max)
    {
        m_rotationLimits[RotationLimit::Axis::AXIS_Y].m_min = min;
        m_rotationLimits[RotationLimit::Axis::AXIS_Y].m_max = max;
    }

    void BlendTreeRotationLimitNode::SetRotationLimitsZ(float min, float max)
    {
        m_rotationLimits[RotationLimit::Axis::AXIS_Z].m_min = min;
        m_rotationLimits[RotationLimit::Axis::AXIS_Z].m_max = max;
    }

    void BlendTreeRotationLimitNode::SetTwistAxis(ConstraintTransformRotationAngles::EAxis twistAxis)
    {
        m_twistAxis = twistAxis;
    }

} // namespace EMotionFX
