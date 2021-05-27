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

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
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
} // namespace PhysX
