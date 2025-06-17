/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Configuration.h>

#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Format.h>

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceId.h>
#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace RPI
    {
        class Buffer;
        class BufferPool;

        //! Types of common buffer pools that buffer system provides.
        //! The intention is to provide most common used type of buffer pools.
        //! These pools are shared by any features.
        //! Note: you still need to build your own buffer pool if certain usage are not available in the list, such as predication buffer,
        //!     or you want to have more control over the pool such as define your own budget for the pool and not share the pool with others.
        enum class CommonBufferPoolType : uint8_t
        {
            Constant = 0, //<! For structured constants. They are often used as ConstantBuffer in shaders
            StaticInputAssembly, //<! For input assembly buffers that are not updated often.
            DynamicInputAssembly, //<! For input assembly buffers that are updated per frame
            ReadBack, //<! For gpu write cpu read buffers which is mainly used to read back gpu data
            Staging, //<! For gpu write cpu read buffers which is mainly used to read back gpu data
            ReadWrite, //<! For gpu read/write buffers. They are often used as both StructuredBuffer and RWStructuredBuffer in different shaders
            ReadOnly, //<! For buffers which are read only. They are usually only used as StructuredBuffer in shaders
            Indirect, //<! For buffers which are used as indirect call arguments

            Count,
            Invalid = Count
        };

        struct CommonBufferDescriptor
        {
            AZStd::string m_bufferName;             //<! A unique buffer name. The CreateBufferFromCommonPool may fail if a buffer with same name exists
            CommonBufferPoolType m_poolType = CommonBufferPoolType::Invalid;
            uint32_t m_elementSize = 1;
            RHI::Format m_elementFormat = RHI::Format::Unknown; //<! [optional] If it's specified with a valid format, the size of this format will be used instead of m_elementSize
            AZ::u64 m_byteCount = 0;
            const void* m_bufferData = nullptr;     //<! [optional] Initial data content of this buffer. This data buffer size needs to be same as m_bufferSizeInbytes
            //! Set to true if you want this buffer to be discoverable by BufferSystemInterface::FindCommonBuffer using m_bufferName.
            //! Note that create buffer may fail if there is a buffer with the same name.
            bool m_isUniqueName = false;
        };

        class ATOM_RPI_PUBLIC_API BufferSystemInterface
        {
        public:
            AZ_RTTI(BufferSystemInterface, "{6FD805CC-C3EC-4E58-A2AF-E9E918965122}");

            BufferSystemInterface() = default;
            virtual ~BufferSystemInterface() = default;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(BufferSystemInterface);

            static BufferSystemInterface* Get()
            {
                return Interface<BufferSystemInterface>::Get();
            }

            //! Get default buffer pool provided by RPI
            virtual RHI::Ptr<RHI::BufferPool> GetCommonBufferPool(CommonBufferPoolType poolType) = 0;

            //! Create buffer from common buffer pool
            virtual Data::Instance<Buffer> CreateBufferFromCommonPool(const CommonBufferDescriptor& descriptor) = 0;

            //! Find a buffer by name. The buffer has to be created by CreateBufferFromCommonPool function
            virtual Data::Instance<Buffer> FindCommonBuffer(AZStd::string_view uniqueBufferName) = 0;
        };
    } // namespace RPI
} // namespace AZ
