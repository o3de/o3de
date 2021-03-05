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

#include <AzFramework/Physics/Joint.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Physics
{
    const char* JointLimitConfiguration::GetTypeName()
    {
        return "Base Joint";
    }

    void JointLimitConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JointLimitConfiguration>()
                ->Version(1)
                ->Field("ParentLocalRotation", &JointLimitConfiguration::m_parentLocalRotation)
                ->Field("ParentLocalPosition", &JointLimitConfiguration::m_parentLocalPosition)
                ->Field("ChildLocalRotation", &JointLimitConfiguration::m_childLocalRotation)
                ->Field("ChildLocalPosition", &JointLimitConfiguration::m_childLocalPosition)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<JointLimitConfiguration>(
                    "Joint Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointLimitConfiguration::m_parentLocalRotation,
                        "Parent local rotation", "The rotation of the parent joint frame relative to the parent body")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointLimitConfiguration::m_parentLocalPosition,
                        "Parent local position", "The position of the joint in the frame of the parent body")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointLimitConfiguration::m_childLocalRotation,
                        "Child local rotation", "The rotation of the child joint frame relative to the child body")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointLimitConfiguration::m_childLocalPosition,
                        "Child local position", "The position of the joint in the frame of the child body")
                    ;
            }
        }
    }
} // namespace Physics
