/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFrameworkModule.h>
#include <AtomToolsFrameworkSystemComponent.h>
#include <Window/AtomToolsMainWindowSystemComponent.h>
#include <PerformanceMonitor/PerformanceMonitorSystemComponent.h>
#include <PreviewRenderer/PreviewRendererSystemComponent.h>

namespace AtomToolsFramework
{
    AtomToolsFrameworkModule::AtomToolsFrameworkModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
                AtomToolsFrameworkSystemComponent::CreateDescriptor(),
                AtomToolsMainWindowSystemComponent::CreateDescriptor(),
                PerformanceMonitorSystemComponent::CreateDescriptor(),
                PreviewRendererSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList AtomToolsFrameworkModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AtomToolsFrameworkSystemComponent>(),
            azrtti_typeid<AtomToolsMainWindowSystemComponent>(),
            azrtti_typeid<PerformanceMonitorSystemComponent>(),
            azrtti_typeid<PreviewRendererSystemComponent>(),
        };
    }
}

#if !defined(AtomToolsFramework_EDITOR)
    #if defined(O3DE_GEM_NAME)
    AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AtomToolsFramework::AtomToolsFrameworkModule)
    #else
    AZ_DECLARE_MODULE_CLASS(Gem_AtomToolsFramework, AtomToolsFramework::AtomToolsFrameworkModule)
    #endif
#endif
