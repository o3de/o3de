/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/BufferView.h>

namespace AZ
{
    namespace RHI
    {
        class MemoryStatisticsBuilder;
        class BufferFrameAttachment;
        struct BufferViewDescriptor;
    
        //! A buffer corresponds to a region of linear memory and used for rendering operations. The user
        //! manages the lifecycle of a buffer through a BufferPool.
        class Buffer
            : public Resource
        {
            using Base = Resource;
            friend class BufferPoolBase;
        public:
            AZ_RTTI(Buffer, "{3C918323-F39C-4DC6-BEE9-BC220DBA9414}", Resource);
            virtual ~Buffer() = default;

            const BufferDescriptor& GetDescriptor() const;
            
            //! This implementation estimates memory usage using the descriptor. Platforms may
            //! override to report more accurate usage metrics.
            void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const override;

            /// Returns the buffer frame attachment if the buffer is currently attached.
            const BufferFrameAttachment* GetFrameAttachment() const;

            Ptr<BufferView> GetBufferView(const BufferViewDescriptor& bufferViewDescriptor);
            
        protected:
            Buffer() = default;

            void SetDescriptor(const BufferDescriptor& descriptor);

        private:
            
            // Get the hash associated with the passed view descriptor
            const size_t GetHash(const BufferViewDescriptor& bufferViewDescriptor) const;
            
            // The RHI descriptor for this buffer.
            BufferDescriptor m_descriptor;

            // A debug reference count to track use of map / unmap operations.
            AZStd::atomic_int m_mapRefCount = {0};
        };
    }
}
