/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFrameworkModule.h>
#include <AtomToolsFrameworkSystemComponent.h>

namespace AtomToolsFramework
{
    AtomToolsFrameworkModule::AtomToolsFrameworkModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AtomToolsFrameworkSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList AtomToolsFrameworkModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AtomToolsFrameworkSystemComponent>(),
        };
    }
}

#if !defined(AtomToolsFramework_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AtomToolsFramework, AtomToolsFramework::AtomToolsFrameworkModule)
#endif
