/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/EditorSystemComponent.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/PropertyTypes.h>

namespace NvCloth
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    void EditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NvClothEditorService"));
    }

    void EditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NvClothEditorService"));
    }

    void EditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NvClothService"));
    }

    void EditorSystemComponent::Activate()
    {
        m_propertyHandlers = Editor::RegisterPropertyTypes();
    }

    void EditorSystemComponent::Deactivate()
    {
        Editor::UnregisterPropertyTypes(m_propertyHandlers);
    }
} // namespace NvCloth
