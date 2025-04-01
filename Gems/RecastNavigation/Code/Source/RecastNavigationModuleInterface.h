/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RecastNavigationSystemComponent.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Components/DetourNavigationComponent.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <Components/RecastNavigationPhysXProviderComponent.h>

namespace RecastNavigation
{
    class RecastNavigationModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(RecastNavigationModuleInterface, "{d1f30353-6d97-4392-b367-a82587ce439c}", AZ::Module);
        AZ_CLASS_ALLOCATOR(RecastNavigationModuleInterface, AZ::SystemAllocator);

        RecastNavigationModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                RecastNavigationSystemComponent::CreateDescriptor(),
                DetourNavigationComponent::CreateDescriptor(),
                RecastNavigationMeshComponent::CreateDescriptor(),
                RecastNavigationPhysXProviderComponent::CreateDescriptor(),
                });
        }

        //! Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<RecastNavigationSystemComponent>(),
            };
        }
    };
}// namespace RecastNavigation
