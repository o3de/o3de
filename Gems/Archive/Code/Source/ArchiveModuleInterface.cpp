/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <Archive/ArchiveTypeIds.h>
#include <Clients/ArchiveReaderFactory.h>
#include <Clients/ArchiveSystemComponent.h>

namespace Archive
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(ArchiveModuleInterface,
        "ArchiveModuleInterface", ArchiveModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(ArchiveModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(ArchiveModuleInterface, AZ::SystemAllocator);

    ArchiveModuleInterface::ArchiveModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            ArchiveSystemComponent::CreateDescriptor(),
            });

        m_archiveReaderFactory = AZStd::make_unique<ArchiveReaderFactory>();
        ArchiveReaderFactoryInterface::Register(m_archiveReaderFactory.get());
    }

    ArchiveModuleInterface::~ArchiveModuleInterface()
    {
        ArchiveReaderFactoryInterface::Unregister(m_archiveReaderFactory.get());
    }

    AZ::ComponentTypeList ArchiveModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<ArchiveSystemComponent>(),
        };
    }
} // namespace Archive
