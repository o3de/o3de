/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GraphicsGem_AR_TestModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestTypeIds.h>

#include <Clients/GraphicsGem_AR_TestSystemComponent.h>

namespace GraphicsGem_AR_Test
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(GraphicsGem_AR_TestModuleInterface,
        "GraphicsGem_AR_TestModuleInterface", GraphicsGem_AR_TestModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(GraphicsGem_AR_TestModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(GraphicsGem_AR_TestModuleInterface, AZ::SystemAllocator);

    GraphicsGem_AR_TestModuleInterface::GraphicsGem_AR_TestModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            GraphicsGem_AR_TestSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList GraphicsGem_AR_TestModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<GraphicsGem_AR_TestSystemComponent>(),
        };
    }
} // namespace GraphicsGem_AR_Test
