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

#include <Atom/Document/ShaderManagementConsoleDocumentModule.h>
#include <Document/ShaderManagementConsoleDocumentSystemComponent.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleDocumentModule::ShaderManagementConsoleDocumentModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            ShaderManagementConsoleDocumentSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList ShaderManagementConsoleDocumentModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AzToolsFramework::AssetSystem::AssetSystemComponent>(),
            azrtti_typeid<ShaderManagementConsoleDocumentSystemComponent>(),
        };
    }
}
