/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/IO/IStreamerTypes.h>

namespace AZ::IO::IStreamerTypes
{
    class RequestMemoryAllocatorMock
        : public RequestMemoryAllocator
    {
    public:
        ~RequestMemoryAllocatorMock() override = default;

        MOCK_METHOD0(LockAllocator, void());
        MOCK_METHOD0(UnlockAllocator, void());
        MOCK_METHOD3(Allocate, RequestMemoryAllocatorResult(AZ::u64, AZ::u64, size_t));
        MOCK_METHOD1(Release, void(void*));

        inline RequestMemoryAllocatorResult ForwardAllocate(AZ::u64 minimalSize, AZ::u64 recommendedSize, size_t alignment)
        {
            return m_defaultAllocator.Allocate(minimalSize, recommendedSize, alignment);
        }

        inline void ForwardRelease(void* address)
        {
            return m_defaultAllocator.Release(address);
        }

    private:
        DefaultRequestMemoryAllocator m_defaultAllocator;
    };
} // namespace AZ::IO::IStreamerTypes
