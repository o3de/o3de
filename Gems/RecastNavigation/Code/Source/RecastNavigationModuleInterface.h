/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Components/RecastNavigationAgentComponent.h>
#include <Components/RecastNavigationMeshComponent.h>

namespace RecastNavigation
{
    class RecastNavigationModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(RecastNavigationModuleInterface, "{057059f2-6317-43b5-a4ca-4ec789ec69a9}", AZ::Module);
        AZ_CLASS_ALLOCATOR(RecastNavigationModuleInterface, AZ::SystemAllocator, 0);

        RecastNavigationModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                RecastNavigationAgentComponent::CreateDescriptor(),
                RecastNavigationMeshComponent::CreateDescriptor(),
                });
        }
    };
}// namespace RecastNavigation
