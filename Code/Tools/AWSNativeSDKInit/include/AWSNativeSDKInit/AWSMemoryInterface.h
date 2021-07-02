/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
#include <aws/core/utils/memory/MemorySystemInterface.h>
#else
#include <cstddef>
#endif

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AWSNativeSDKInit
{
    class AWSNativeSDKAllocator final
        : public AZ::SystemAllocator
    {
    public:
        AZ_CLASS_ALLOCATOR(AWSNativeSDKAllocator, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(AWSNativeSDKAllocator, "{8B4DA42F-2507-4A5B-B13C-4B2A72BC161E}");

        ///////////////////////////////////////////////////////////////////////////////////////////
        // IAllocator
        const char* GetName() const override
        {
            return "AWSNativeSDKAllocator";
        }

        const char* GetDescription() const override
        {
            return "Allocator used by the AWSNativeSDK";
        }
        ///////////////////////////////////////////////////////////////////////////////////////////
    };

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
    class MemoryManager : public Aws::Utils::Memory::MemorySystemInterface
    {
        static const char* AWS_API_ALLOC_TAG;
    public:

        void Begin() override;
        void End() override;

        void* AllocateMemory(std::size_t blockSize, std::size_t alignment, const char* allocationTag = nullptr) override;
        void FreeMemory(void* memoryPtr) override;

        AZ::AllocatorWrapper<AWSNativeSDKAllocator> m_allocator;
        bool m_systemAllocatorCreated{ false };
    };
#else

    class MemoryManager
    {
        static const char* AWS_API_ALLOC_TAG;
    public:
        void Begin();
        void End();
        void* AllocateMemory(std::size_t blockSize, std::size_t alignment, const char* allocationTag = nullptr);
        void FreeMemory(void* memoryPtr);
    };
#endif
}
