/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/BufferMemoryView.h>

namespace AZ
{
    namespace Metal
    {
        class Buffer;

        class BufferView final
            : public RHI::BufferView
        {
            using Base = RHI::BufferView;
        public:
            AZ_CLASS_ALLOCATOR(BufferView, AZ::ThreadPoolAllocator);
            AZ_RTTI(BufferView, "{9CD198D5-BA56-4591-947F-A16DCF50B3E5}", Base);

            static RHI::Ptr<BufferView> Create();

            void Init(Buffer& buffer, const RHI::BufferViewDescriptor& descriptor);
            const Buffer& GetBuffer() const;

            const MemoryView& GetMemoryView() const;
            MemoryView& GetMemoryView();
            
            const MemoryView& GetTextureBufferView() const;
            MemoryView& GetTextureBufferView();
            
            //! Get the index related to the position of the read and readwrite view within the global Bindless Argument Buffer
            uint32_t GetBindlessReadIndex() const override;
            uint32_t GetBindlessReadWriteIndex() const override;
            
        private:
            BufferView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::BufferView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::Resource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            void ReleaseViews();
            void ReleaseBindlessIndices();

            //Buffer view
            MemoryView m_memoryView;
            
            //TextureBuffer view. Used for texture_buffer variables
            MemoryView m_imageBufferMemoryView;
            
            //! Index related to the position of the read and readwrite view within the global Bindless Argument Buffer
            uint32_t m_readIndex = InvalidBindlessIndex;
            uint32_t m_readWriteIndex = InvalidBindlessIndex;
        };
    }
}
