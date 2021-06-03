/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <PhysX_precompiled.h>
#include <Joint/Configuration/PhysXJointConfiguration.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <Joint/PhysXApiJointUtils.h>

namespace PhysX
{
    bool ApiJointGenericProperties::IsFlagSet(GenericApiJointFlag flag) const
    {
        return static_cast<bool>(m_flags & flag);
    }

    void D6ApiJointLimitConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<D6ApiJointLimitConfiguration, AzPhysics::ApiJointConfiguration>()
                ->Version(1)
                ->Field("SwingLimitY", &D6ApiJointLimitConfiguration::m_swingLimitY)
                ->Field("SwingLimitZ", &D6ApiJointLimitConfiguration::m_swingLimitZ)
                ->Field("TwistLowerLimit", &D6ApiJointLimitConfiguration::m_twistLimitLower)
                ->Field("TwistUpperLimit", &D6ApiJointLimitConfiguration::m_twistLimitUpper)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<D6ApiJointLimitConfiguration>(
                    "PhysX D6 Joint Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6ApiJointLimitConfiguration::m_swingLimitY, "Swing limit Y",
                        "Maximum angle from the Y axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, JointConstants::MinSwingLimitDegrees)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6ApiJointLimitConfiguration::m_swingLimitZ, "Swing limit Z",
                        "Maximum angle from the Z axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, JointConstants::MinSwingLimitDegrees)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6ApiJointLimitConfiguration::m_twistLimitLower, "Twist lower limit",
                        "Lower limit for rotation about the X axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6ApiJointLimitConfiguration::m_twistLimitUpper, "Twist upper limit",
                        "Upper limit for rotation about the X axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                ;
            }
        }
    }

    void ApiJointGenericProperties::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ApiJointGenericProperties>()
                ->Version(1)
                ->Field("Maximum Force", &ApiJointGenericProperties::m_forceMax)
                ->Field("Maximum Torque", &ApiJointGenericProperties::m_torqueMax)
                ->Field("Flags", &ApiJointGenericProperties::m_flags)
                ;
        }
    }

    void ApiJointLimitProperties::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ApiJointLimitProperties>()
                ->Version(1)
                ->Field("First Limit", &ApiJointLimitProperties::m_limitFirst)
                ->Field("Second Limit", &ApiJointLimitProperties::m_limitSecond)
                ->Field("Tolerance", &ApiJointLimitProperties::m_tolerance)
                ->Field("Is Limited", &ApiJointLimitProperties::m_isLimited)
                ->Field("Is Soft Limit", &ApiJointLimitProperties::m_isSoftLimit)
                ->Field("Damping", &ApiJointLimitProperties::m_damping)
                ->Field("Spring", &ApiJointLimitProperties::m_stiffness)
                ;
        }
    }

    void FixedApiJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FixedApiJointConfiguration, AzPhysics::ApiJointConfiguration>()
                ->Version(1)
                ->Field("Generic Properties", &FixedApiJointConfiguration::m_genericProperties)
                ;
        }
    }

    void BallApiJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BallApiJointConfiguration, AzPhysics::ApiJointConfiguration>()
                ->Version(1)
                ->Field("Generic Properties", &BallApiJointConfiguration::m_genericProperties)
                ->Field("Limit Properties", &BallApiJointConfiguration::m_limitProperties)
                ;
        }
    }
    
    void HingeApiJointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HingeApiJointConfiguration, AzPhysics::ApiJointConfiguration>()
                ->Version(1)
                ->Field("Generic Properties", &HingeApiJointConfiguration::m_genericProperties)
                ->Field("Limit Properties", &HingeApiJointConfiguration::m_limitProperties)
                ;
        }
    }
} // namespace PhysX
