/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FreeListAllocator.h>
#include <RHI/CommandList.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
        class ArgumentBuffer;
        
        //! This class is responsible for managing the global bindless heap. It provides
        //! support for this heap via both unbounded array as well as bounded arrays.
        class BindlessArgumentBuffer
        {
        public:
            BindlessArgumentBuffer() = default;
            RHI::ResultCode Init(Device* m_device);

            //! Provide access to the bindless argument buffer
            RHI::Ptr<ArgumentBuffer> GetBindlessArgbuffer() const;

            //! Add/Update a read only image pointer to the global bindless heap 
            uint32_t AttachReadImage(id <MTLTexture> mtlTexture);

            //! Add/Update a read write image pointer to the global bindless heap 
            uint32_t AttachReadWriteImage(id <MTLTexture> mtlTexture);

            //! Add/Update a read only buffer pointer to the global bindless heap 
            uint32_t AttachReadBuffer(id <MTLBuffer> mtlTexture, uint32_t offfset);

            //! Add/Update a read write buffer pointer to the global bindless heap 
            uint32_t AttachReadWriteBuffer(id <MTLBuffer> mtlTexture, uint32_t offfset);

            //! Remove the index associated with a read only image descriptor from the free list allocator
            void DetachReadImage(uint32_t index);

            //! Remove the index associated with a read write image descriptor from the free list allocator
            void DetachReadWriteImage(uint32_t index);

            //! Remove the index associated with a read only buffer descriptor from the free list allocator
            void DetachReadBuffer(uint32_t index);

            //! Remove the index associated with a read write buffer descriptor from the free list allocator
            void DetachReadWriteBuffer(uint32_t index);

            //! Garbage collect all the free list allocators related to all the bindless resource types
            void GarbageCollect();

            //! Add all the argument buffers related to the global bindless heap to the maps passed in
            //! so that it can be bound to the encoder efficiently
            void BindBindlessArgumentBuffer(uint32_t slotIndex,
                                            CommandEncoderType commandEncoderType,
                                            CommandList::MetalArgumentBufferArray& mtlVertexArgBuffers,
                                            CommandList::MetalArgumentBufferArrayOffsets& mtlVertexArgBufferOffsets,
                                            CommandList::MetalArgumentBufferArray& mtlFragmentOrComputeArgBuffers,
                                            CommandList::MetalArgumentBufferArrayOffsets& mtlFragmentOrComputeArgBufferOffsets,
                                            uint32_t& bufferVertexRegisterIdMin,
                                            uint32_t& bufferVertexRegisterIdMax,
                                            uint32_t& bufferFragmentOrComputeRegisterIdMin,
                                            uint32_t& bufferFragmentOrComputeRegisterIdMax);

            //! Add all all the bindless resource views indirectly bound to the maps passed in so that they can be made resident
            void MakeBindlessArgumentBuffersResident(CommandEncoderType commandEncoderType,
                                                     ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesGfxRead,
                                                     ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeRead);
        private:

            //Bindless ABs + the rootAB which will act as a container. This is used for unbounded arrays.
            RHI::Ptr<ArgumentBuffer> m_rootArgBuffer;
            RHI::Ptr<ArgumentBuffer> m_bindlessTextureArgBuffer;
            RHI::Ptr<ArgumentBuffer> m_bindlessRWTextureArgBuffer;
            RHI::Ptr<ArgumentBuffer> m_bindlessBufferArgBuffer;
            RHI::Ptr<ArgumentBuffer> m_bindlessRWBufferArgBuffer;
             
            //Bounded AB in order to simulate bindless behaviour for plats that do not support unbounded arrays.
            RHI::Ptr<ArgumentBuffer> m_boundedArgBuffer;
            
            // Free list allocator per bindless resource type
            RHI::FreeListAllocator m_allocators[static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::Count)];
            Device* m_device = nullptr;
            bool m_unboundedArraySupported = false;
            // Mutex to protect bindless heap related updates
            AZStd::mutex m_mutex;
        };
    }
}
