/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFrameworkModule.h>
#include <AtomToolsFrameworkSystemComponent.h>
#include <Document/AtomToolsDocumentSystemComponent.h>
#include <Window/AtomToolsMainWindowSystemComponent.h>
#include <PreviewRenderer/PreviewRendererSystemComponent.h>

namespace AtomToolsFramework
{
    AtomToolsFrameworkModule::AtomToolsFrameworkModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
                AtomToolsFrameworkSystemComponent::CreateDescriptor(),
                AtomToolsDocumentSystemComponent::CreateDescriptor(),
                AtomToolsMainWindowSystemComponent::CreateDescriptor(),
                PreviewRendererSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList AtomToolsFrameworkModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AtomToolsFrameworkSystemComponent>(),
            azrtti_typeid<AtomToolsDocumentSystemComponent>(),
            azrtti_typeid<AtomToolsMainWindowSystemComponent>(),
            azrtti_typeid<PreviewRendererSystemComponent>(),
        };
    }
}

#if !defined(AtomToolsFramework_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AtomToolsFramework, AtomToolsFramework::AtomToolsFrameworkModule)
#endif
