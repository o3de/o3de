/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <Joint/PhysXJointUtils.h>

namespace PhysX
{
    JointGenericProperties::JointGenericProperties(GenericJointFlag flags, float forceMax, float torqueMax)
        : m_flags(flags)
        , m_forceMax(forceMax)
        , m_torqueMax(torqueMax)
    {

    }

    JointLimitProperties::JointLimitProperties(
        bool isLimited, bool isSoftLimit, 
        float damping, float limitFirst, float limitSecond, float stiffness, float tolerance)
        : m_isLimited(isLimited)
        , m_isSoftLimit(isSoftLimit)
        , m_damping(damping)
        , m_limitFirst(limitFirst)
        , m_limitSecond(limitSecond)
        , m_stiffness(stiffness)
        , m_tolerance(tolerance)
    {

    }

    bool JointGenericProperties::IsFlagSet(GenericJointFlag flag) const
    {
        return static_cast<bool>(m_flags & flag);
    }

    void D6JointLimitConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<D6JointLimitConfiguration, AzPhysics::JointConfiguration>()
                ->Version(1)
                ->Field("SwingLimitY", &D6JointLimitConfiguration::m_swingLimitY)
                ->Field("SwingLimitZ", &D6JointLimitConfiguration::m_swingLimitZ)
                ->Field("TwistLowerLimit", &D6JointLimitConfiguration::m_twistLimitLower)
                ->Field("TwistUpperLimit", &D6JointLimitConfiguration::m_twistLimitUpper)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<D6JointLimitConfiguration>(
                    "PhysX D6 Joint Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_swingLimitY, "Swing limit Y",
                        "The rotation angle limit around the joint's Y axis.")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, JointConstants::MinSwingLimitDegrees)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_swingLimitZ, "Swing limit Z",
                        "The rotation angle limit around the joint's Z axis.")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, JointConstants::MinSwingLimitDegrees)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_twistLimitLower, "Twist lower limit",
                        "The lower rotation angle limit around the joint's X axis.")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_twistLimitUpper, "Twist upper limit",
                        "The upper rotation angle limit around the joint's X axis.")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                ;
            }
        }
    }

    AZStd::optional<float> D6JointLimitConfiguration::GetPropertyValue([[maybe_unused]] const AZ::Name& propertyName)
    {
        if (propertyName == AZ::Name("SwingLimitY"))
        {
            return m_swingLimitY;
        }
        else if (propertyName == AZ::Name("SwingLimitZ"))
        {
            return m_swingLimitZ;
        }
        else if (propertyName == AZ::Name("TwistLimitLower"))
        {
            return m_twistLimitLower;
        }
        else if (propertyName == AZ::Name("TwistLimitUpper"))
        {
            return m_twistLimitUpper;
        }
        else
        {
            AZ_ErrorOnce("PhysX Joint Configuration", false, "Property %s not recognized.", propertyName.GetCStr());
            return AZStd::nullopt;
        }
    }

    void D6JointLimitConfiguration::SetPropertyValue([[maybe_unused]] const AZ::Name& propertyName, [[maybe_unused]] float value)
    {
        if (propertyName == AZ::Name("SwingLimitY"))
        {
            m_swingLimitY = value;
            ValidateSwingLimitY();
        }
        else if (propertyName == AZ::Name("SwingLimitZ"))
        {
            m_swingLimitZ = value;
            ValidateSwingLimitZ();
        }
        else if (propertyName == AZ::Name("TwistLimitLower"))
        {
            m_twistLimitLower = value;
            ValidateTwistLimits();
        }
        else if (propertyName == AZ::Name("TwistLimitUpper"))
        {
            m_twistLimitUpper = value;
            ValidateTwistLimits();
        }
        else
        {
            AZ_ErrorOnce("PhysX Joint Configuration", false, "Property %s not recognized.", propertyName.GetCStr());
        }
    }

    void D6JointLimitConfiguration::ValidateSwingLimitY()
    {
        m_swingLimitY = AZ::GetClamp(m_swingLimitY, JointConstants::MinSwingLimitDegrees, 180.0f - JointConstants::MinSwingLimitDegrees);
    }

    void D6JointLimitConfiguration::ValidateSwingLimitZ()
    {
        m_swingLimitZ = AZ::GetClamp(m_swingLimitZ, JointConstants::MinSwingLimitDegrees, 180.0f - JointConstants::MinSwingLimitDegrees);
    }

    void D6JointLimitConfiguration::ValidateTwistLimits()
    {
        m_twistLimitLower = AZ::GetClamp(m_twistLimitLower, -180.0f, 180.0f);
        m_twistLimitUpper = AZ::GetClamp(m_twistLimitUpper, -180.0f, 180.0f);
        // make sure lower limit is less than upper limit
        if (m_twistLimitLower > m_twistLimitUpper)
        {
            AZStd::swap(m_twistLimitLower, m_twistLimitUpper);
        }
        // make sure the range between the lower and upper limits exceeds the minimum range
        if (m_twistLimitUpper < m_twistLimitLower + JointConstants::MinTwistLimitRangeDegrees)
        {
            if (m_twistLimitLower > 0.0f)
            {
                m_twistLimitLower = m_twistLimitUpper - JointConstants::MinTwistLimitRangeDegrees;
            }
            else
            {
                m_twistLimitUpper = m_twistLimitLower + JointConstants::MinTwistLimitRangeDegrees;
            }
        }
    }

    void JointGenericProperties::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JointGenericProperties>()
                ->Version(1)
                ->Field("Maximum Force", &JointGenericProperties::m_forceMax)
                ->Field("Maximum Torque", &JointGenericProperties::m_torqueMax)
                ->Field("Flags", &JointGenericProperties::m_flags)
                ;
        }
    }

    void JointLimitProperties::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JointLimitProperties>()
                ->Version(1)
                ->Field("First Limit", &JointLimitProperties::m_limitFirst)
                ->Field("Second Limit", &JointLimitProperties::m_limitSecond)
                ->Field("Tolerance", &JointLimitProperties::m_tolerance)
                ->Field("Is Limited", &JointLimitProperties::m_isLimited)
                ->Field("Is Soft Limit", &JointLimitProperties::m_isSoftLimit)
                ->Field("Damping", &JointLimitProperties::m_damping)
                ->Field("Spring", &JointLimitProperties::m_stiffness)
                ;
        }
    }

    void FixedJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FixedJointConfiguration, AzPhysics::JointConfiguration>()
                ->Version(1)
                ->Field("Generic Properties", &FixedJointConfiguration::m_genericProperties)
                ;
        }
    }

    void BallJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BallJointConfiguration, AzPhysics::JointConfiguration>()
                ->Version(1)
                ->Field("Generic Properties", &BallJointConfiguration::m_genericProperties)
                ->Field("Limit Properties", &BallJointConfiguration::m_limitProperties)
                ;
        }
    }
    
    void HingeJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HingeJointConfiguration, AzPhysics::JointConfiguration>()
                ->Version(1)
                ->Field("Generic Properties", &HingeJointConfiguration::m_genericProperties)
                ->Field("Limit Properties", &HingeJointConfiguration::m_limitProperties)
                ->Field("Motor Properties", &HingeJointConfiguration::m_motorProperties)
                ;
        }
    }

    void PrismaticJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PrismaticJointConfiguration, AzPhysics::JointConfiguration>()
                ->Version(1)
                ->Field("Generic Properties", &PrismaticJointConfiguration::m_genericProperties)
                ->Field("Limit Properties", &PrismaticJointConfiguration::m_limitProperties)
                ->Field("Motor Properties", &PrismaticJointConfiguration::m_motorProperties)
                ;
        }
    }

    void JointMotorProperties::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JointMotorProperties>()
                ->Version(1)
                ->Field("UseMotor", &JointMotorProperties::m_useMotor)
                ->Field("ForceLimit", &JointMotorProperties::m_driveForceLimit);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::JointMotorProperties>("PhysX Joint Motor Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &PhysX::JointMotorProperties::m_useMotor, "Use Motor", "Enable motor in the joint")
                    ->DataElement(0, &PhysX::JointMotorProperties::m_driveForceLimit, "Force Limit Value", "Sets force limit value");
            }
        }
    }
} // namespace PhysX
