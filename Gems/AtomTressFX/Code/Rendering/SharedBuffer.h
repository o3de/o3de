/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Name/Name.h>

#include <Atom/RHI/FreeListAllocator.h>
#include <Atom/RHI.Reflect/Format.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

// Hair specific
#include <Rendering/HairSharedBufferInterface.h>

namespace AZ
{
    namespace RPI
    {
        class BufferAsset;
        class Buffer;
    }

    namespace Render
    {
        //! This structure contains information regarding the naming of the buffer on both the CPU
        //!  and the GPU 
        //! This structure is also used to determine the maximum alignment required for the buffer
        //!  when allocating sub-buffers
        struct SrgBufferDescriptor
        {
            //! Pool type to determine how a resource pool should be generated.
            RPI::CommonBufferPoolType m_poolType;
            //! The format used for the buffer
            //! Should be Unknown for structured buffers, or R32 for raw buffers.
            RHI::Format m_elementFormat;
            //! The size in bytes of each element in the stream
            uint32_t m_elementSize;
            //! Amount of elements required to create the buffer
            uint32_t m_elementCount;
            //! The name used for the buffer view
            Name m_bufferName;
            //! The name used by the shader Srg in the GPU for this shader parameter 
            Name m_paramNameInSrg;
            //! The assigned SRG slot in the CPU / GPU for this shader resource
            uint32_t m_resourceShaderIndex;
            //! If using a buffer view within a shared buffer, this represents
            //! the view offset from the shared buffer origin in bytes.
            uint32_t m_viewOffsetInBytes;

            SrgBufferDescriptor() = default;
            SrgBufferDescriptor(
                RPI::CommonBufferPoolType poolType,
                RHI::Format elementFormat,
                uint32_t elementSize,
                uint32_t elementCount,
                Name bufferName,
                Name paramNameInSrg,
                uint32_t resourceShaderIndex,
                uint32_t viewOffsetInBytes
            ) : m_poolType(poolType), m_elementFormat(elementFormat),
                m_elementSize(elementSize), m_elementCount(elementCount),
                m_bufferName(bufferName), m_paramNameInSrg(paramNameInSrg),
                m_resourceShaderIndex(resourceShaderIndex), m_viewOffsetInBytes(viewOffsetInBytes)
            {};
        };

        //! This class represents a single RPI::Buffer used to allocate sub-buffers from the
        //!  existing buffer that can then be used per draw.   In a way, this buffer is used as a memory
        //!  pool from which sub-buffers are being created.
        //! This is very useful when we want to synchronize the use of these buffers via barriers 
        //!  so we declare and pass the entire buffer between passes and therefore we are creating
        //!  a dependency and barrier for this single buffer, yet as a result all sub-buffers are
        //!  now getting synced between passes.
        //! 
        // Adopted from SkinnedMeshOutputStreamManager in order to make it more generic and context free usage
        class SharedBuffer
            : public HairSharedBufferInterface
            , private SystemTickBus::Handler
        {
        public:
            SharedBuffer();
            SharedBuffer(AZStd::string bufferName, AZStd::vector<SrgBufferDescriptor>& buffersDescriptors);

            AZ_RTTI(AZ::Render::SharedBuffer, "{D910C301-99F7-41B6-A2A6-D566F3B2C030}", AZ::Render::HairSharedBufferInterface);

            ~SharedBuffer();
            void Init(AZStd::string bufferName, AZStd::vector<SrgBufferDescriptor>& buffersDescriptors);

            // SharedBufferInterface
            AZStd::intrusive_ptr<HairSharedBufferAllocation> Allocate(size_t byteCount) override;
            void DeAllocate(RHI::VirtualAddress allocation) override;
            void DeAllocateNoSignal(RHI::VirtualAddress allocation) override;
            Data::Asset<RPI::BufferAsset> GetBufferAsset() const override;

            Data::Instance<RPI::Buffer> GetBuffer() override;

            //! Update buffer's content with sourceData at an offset of bufferByteOffset
            bool UpdateData(const void* sourceData, uint64_t sourceDataSizeInBytes, uint64_t bufferByteOffset = 0) override;

            //! Utility function to create a resource view of different type than the shared buffer data.
            //! Since this class is sub-buffer container, this method should be used after creating
            //!  a new allocation to be used as a sub-buffer.
            static RHI::BufferViewDescriptor CreateResourceViewWithDifferentFormat(
                uint32_t offsetInBytes, uint32_t elementCount, uint32_t elementSize,
                RHI::Format format, RHI::BufferBindFlags overrideBindFlags);

        private:
            // SystemTickBus
            void OnSystemTick() override;

            void GarbageCollect();
            void CalculateAlignment(AZStd::vector<SrgBufferDescriptor>& buffersDescriptors);
            void InitAllocator();
            void CreateBufferAsset();
            void CreateBuffer();


            AZStd::string m_bufferName = "GenericSharedBuffer";
            Data::Asset<AZ::RPI::ResourcePoolAsset> m_bufferPoolAsset;
            Data::Instance<RPI::Buffer> m_buffer = nullptr;
            Data::Asset<RPI::BufferAsset> m_bufferAsset = {};

            RHI::FreeListAllocator m_freeListAllocator;
            AZStd::mutex m_allocatorMutex;
            uint64_t m_alignment = 16;     // This will be overridden by the size of the largest allocated element
            //! Currently the shared buffer size is fixed. Going towards dynamic size can be a better
            //! solution but requires using re-allocations and proper synchronizing between all existing buffers.
            //! This amount of memory should be enough for 2-3 very details cinematic hair or for 4-6 high
            //! fidelity hair objects.
            //! Additional attention should be given to the fact that because the buffers in Atom are NOT triple
            //! buffered but instead they are delayed via garbage collection mechanism, during reallocation
            //! the amount of memory required might reach close to double of the run-time.
            size_t m_sizeInBytes = 256u * (1024u * 1024u);   
            bool m_memoryWasFreed = false;
            bool m_broadcastMemoryAvailableEvent = false;
        };
    } // namespace Render
} // namespace AZ

