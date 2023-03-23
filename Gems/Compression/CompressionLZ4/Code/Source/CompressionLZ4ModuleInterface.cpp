/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionLZ4ModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <CompressionLZ4/CompressionLZ4TypeIds.h>

#include <Clients/CompressionLZ4SystemComponent.h>

namespace CompressionLZ4
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(CompressionLZ4ModuleInterface,
        "CompressionLZ4ModuleInterface", CompressionLZ4ModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(CompressionLZ4ModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(CompressionLZ4ModuleInterface, AZ::SystemAllocator);

    CompressionLZ4ModuleInterface::CompressionLZ4ModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            CompressionLZ4SystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList CompressionLZ4ModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<CompressionLZ4SystemComponent>(),
        };
    }
} // namespace CompressionLZ4
