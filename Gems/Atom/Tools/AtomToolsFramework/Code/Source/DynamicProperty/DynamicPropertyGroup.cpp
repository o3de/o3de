/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
