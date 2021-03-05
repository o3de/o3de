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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>

namespace AtomToolsFramework
{
    void DynamicPropertyGroup::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DynamicPropertyGroup>()
                ->Field("properties", &DynamicPropertyGroup::m_properties)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DynamicPropertyGroup>("DynamicPropertyGroup", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly) // hides the DynamicPropertyGroup row
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicPropertyGroup::m_properties, "properties", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly) // hides the m_properties row
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false) // probably not necessary since Visibility is children-only
                    ;
            }
        }
    }
} // namespace AtomToolsFramework
