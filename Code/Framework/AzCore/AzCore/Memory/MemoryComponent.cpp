/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/Memory/PoolAllocator.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    //=========================================================================
    // Activate
    // [5/29/2012]
    //=========================================================================
    void MemoryComponent::Activate()
    {
    }

    //=========================================================================
    // Deactivate
    // [5/29/2012]
    //=========================================================================
    void MemoryComponent::Deactivate()
    {
    }

    //=========================================================================
    // GetProvidedServices
    //=========================================================================
    void MemoryComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MemoryService", 0x5c4d473c));
    }

    //=========================================================================
    // GetIncompatibleServices
    //=========================================================================
    void MemoryComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("MemoryService", 0x5c4d473c));
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void MemoryComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<MemoryComponent, AZ::Component>()
                ->Version(1)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MemoryComponent>("Memory System", "Initializes and maintains fundamental memory allocators")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }
} // namespace AZ
