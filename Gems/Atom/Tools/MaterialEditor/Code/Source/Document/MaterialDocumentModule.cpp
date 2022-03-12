/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Document/MaterialDocumentModule.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <Document/MaterialDocumentSystemComponent.h>

namespace MaterialEditor
{
    MaterialDocumentModule::MaterialDocumentModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            MaterialDocumentSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList MaterialDocumentModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AzToolsFramework::AssetSystem::AssetSystemComponent>(),
            azrtti_typeid<MaterialDocumentSystemComponent>(),
        };
    }
}
