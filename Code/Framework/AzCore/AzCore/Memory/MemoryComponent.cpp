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
    // MemoryComponent
    // [5/29/2012]
    //=========================================================================
    MemoryComponent::MemoryComponent()
    {
        m_isPoolAllocator = true;
        m_isThreadPoolAllocator = true;

        m_createdPoolAllocator = false;
        m_createdThreadPoolAllocator = false;
    }

    //=========================================================================
    // ~MemoryComponent
    // [5/29/2012]
    //=========================================================================
    MemoryComponent::~MemoryComponent()
    {
        // Normally we will just call destroy destroy in the deactivate function
        // and create in activate. But memory component is special that
        // it must be operational after Init so all parts of the engine can be operational.
        // This is why we must check the destructor (which is symmetrical to Init() anyway)
        if (m_createdThreadPoolAllocator && AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        }
        if (m_createdPoolAllocator && AZ::AllocatorInstance<AZ::PoolAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        }
    }

    //=========================================================================
    // Init
    // [5/29/2012]
    //=========================================================================
    void MemoryComponent::Init()
    {
        // IMPORTANT: The SystemAllocator is already operational

        // TODO: add the setting to the serialization layer
        if (m_isPoolAllocator)
        {
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            m_createdPoolAllocator = true;
        }
        if (m_isThreadPoolAllocator)
        {
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
            m_createdThreadPoolAllocator = true;
        }
    }

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
                ->Field("isPoolAllocator", &MemoryComponent::m_isPoolAllocator)
                ->Field("isThreadPoolAllocator", &MemoryComponent::m_isThreadPoolAllocator)
                ;

            ;
            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MemoryComponent>("Memory System", "Initializes and maintains fundamental memory allocators")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MemoryComponent::m_isPoolAllocator, "Pool allocator", "Fast allocation pooling for small allocations < 256 bytes, use from main thread only!")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MemoryComponent::m_isThreadPoolAllocator, "Thread pool allocator", "Fast allocation pool that can be used from any thread, if uses more memory! (as it keeps the pools per thread)")
                    ;
            }
        }
    }
} // namespace AZ
