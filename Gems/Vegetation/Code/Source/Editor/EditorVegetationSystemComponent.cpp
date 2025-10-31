/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        services.push_back(AZ_CRC_CE("EditorVegetationSystemService"));
    }

    void EditorVegetationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("EditorVegetationSystemService"));
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
