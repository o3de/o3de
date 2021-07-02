/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <Tests/D6JointLimitConfiguration.h>

namespace EMotionFX
{
    void D6JointLimitConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<D6JointLimitConfiguration, Physics::JointLimitConfiguration>()
                ->Version(1)
                ->Field("SwingLimitY", &D6JointLimitConfiguration::m_swingLimitY)
                ->Field("SwingLimitZ", &D6JointLimitConfiguration::m_swingLimitZ)
                ->Field("TwistLowerLimit", &D6JointLimitConfiguration::m_twistLimitLower)
                ->Field("TwistUpperLimit", &D6JointLimitConfiguration::m_twistLimitUpper)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<D6JointLimitConfiguration>(
                    "PhysX D6 Joint Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_swingLimitY, "Swing limit Y",
                        "Maximum angle from the Y axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_swingLimitZ, "Swing limit Z",
                        "Maximum angle from the Z axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_twistLimitLower, "Twist lower limit",
                        "Lower limit for rotation about the X axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointLimitConfiguration::m_twistLimitUpper, "Twist upper limit",
                        "Upper limit for rotation about the X axis of the joint frame")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Min, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                ;
            }
        }
    }
} // namespace EMotionFX
