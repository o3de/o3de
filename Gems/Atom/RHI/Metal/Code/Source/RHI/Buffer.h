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

#include <Atom/RHI/Buffer.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/BufferMemoryView.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
        class BufferPool;
        
        class Buffer final
            : public RHI::Buffer
        {
            using Base = RHI::Buffer;
        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Buffer, "{50D79542-AD49-46C8-8660-583A84802105}", Base);
            ~Buffer() = default;
            
            static RHI::Ptr<Buffer> Create();

            // Returns the memory view allocated to this buffer.
            const MemoryView& GetMemoryView() const;
            MemoryView& GetMemoryView();
            
        private:
            Buffer() = default;
            friend class BufferPool;
            friend class AliasedHeap;
            friend class BufferPoolResolver;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::Resource
            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const override;
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::Buffer
            using RHI::Buffer::SetDescriptor;
            //////////////////////////////////////////////////////////////////////////

            BufferMemoryView m_memoryView;
            
            // The number of resolve operations pending for this buffer.
            AZStd::atomic<uint32_t> m_pendingResolves = 0;

        };
    }
}
