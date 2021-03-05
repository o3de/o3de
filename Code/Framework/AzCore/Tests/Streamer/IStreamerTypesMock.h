/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
