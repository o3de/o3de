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
#include "EditorVegetationSystemComponent.h"

#include <Vegetation/Editor/EditorAreaComponentBase.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>
#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>

namespace Vegetation
{
    void EditorVegetationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("EditorVegetationSystemService", 0xfc666830));
    }

    void EditorVegetationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("EditorVegetationSystemService", 0xfc666830));
    }

    void EditorVegetationSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void EditorVegetationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorVegetationSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<EditorVegetationSystemComponent>("Editor Vegetation System", "Manages and discovers surface tag list assets that define the dictionary of selectable tags at edit time")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void EditorVegetationSystemComponent::Activate()
    {
        // This is necessary for the m_spawnerType in Descriptor.cpp to display properly as a ComboBox
        AzToolsFramework::RegisterGenericComboBoxHandler<AZ::TypeId>();
    }

    void EditorVegetationSystemComponent::Deactivate()
    {
    }

}
