/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>

#include <AzCore/std/smart_ptr/intrusive_base.h>

#include <Atom/RHI/SingleDeviceIndexBufferView.h>
#include <Atom/RHI/SingleDeviceStreamBufferView.h>

namespace AZ
{
    namespace RPI
    {
        //! A DynamicBuffer represents a transient GPU buffer which only be valid for one frame after it's acquired.
        //! The acquired DynamicBuffers become invalid when DynamicDrawInterface::Get()->FrameEnd() is called.
        //! DynamicBuffers are allocated by DynamicBufferAllocator. Check the description of DynamicBufferAllocator class for detail. 
        //! The typical usage:
        //!     // For every frame
        //!     auto buffer = DynamicDrawInterface::Get()->GetDynamicBuffer(size, RHI::Alignment::InputAssembly);
        //!     if (buffer) // the buffer could be empty if the allocation failed.
        //!     {
        //!         // write data to the buffer
        //!         buffer->Write(data, size);
        //!         // Use the buffer view for DrawItem or etc.
        //!     }
        //! Note: DynamicBuffer should only be used for DynamicInputAssembly buffer or Constant buffer (not supported yet).
        class DynamicBuffer
            : public AZStd::intrusive_base
        {
            friend class DynamicBufferAllocator;

        public:
            AZ_RTTI(AZ::RPI::DynamicBuffer, "{812ED1A6-9E9C-4ED0-9D47-6615DB7A2226}");
            AZ_CLASS_ALLOCATOR(DynamicBuffer, AZ::SystemAllocator);
            //! Write data to the DyanmicBuffer. The write size can't be larger than this buffer's size
            bool Write(const void* data, uint32_t size);

            //! Get the buffer's size
            uint32_t GetSize();

            //! Get the buffer's address. User can write data to the address. 
            void* GetBufferAddress();

            //! Get IndexBufferView if this buffer is used as index buffer
            RHI::SingleDeviceIndexBufferView GetIndexBufferView(RHI::IndexFormat format);

            //! Get StreamBufferView if this buffer is used as vertex buffer
            //! @param strideByteCount the byte count of the element
            RHI::SingleDeviceStreamBufferView GetStreamBufferView(uint32_t strideByteCount);

        private:
            // Only DynamicBufferAllocator can allocate a DynamicBuffer
            DynamicBuffer() = default;

            // initialize function called by DynamicBufferAllocator which to initialize this buffer
            void Initialize(void* address, uint32_t size);

            void* m_address = nullptr;
            uint32_t m_size;

            // The allocator which allocated this DyanmicBuffer. 
            DynamicBufferAllocator* m_allocator;
        };
    }
}
