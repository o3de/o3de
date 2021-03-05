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
