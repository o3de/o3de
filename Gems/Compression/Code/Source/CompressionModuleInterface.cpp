/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <Compression/CompressionTypeIds.h>

#include <Clients/CompressionSystemComponent.h>

namespace Compression
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(CompressionModuleInterface,
        "CompressionModuleInterface", CompressionModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(CompressionModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(CompressionModuleInterface, AZ::SystemAllocator);

    CompressionModuleInterface::CompressionModuleInterface()
    {
        m_descriptors.insert(m_descriptors.end(), {
            CompressionSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList CompressionModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<CompressionSystemComponent>(),
        };
    }
} // namespace Compression
