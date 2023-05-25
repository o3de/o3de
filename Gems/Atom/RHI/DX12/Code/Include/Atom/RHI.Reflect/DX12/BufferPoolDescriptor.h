/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ
{
    namespace DX12
    {
        class BufferPoolDescriptor
            : public RHI::BufferPoolDescriptor
        {
        public:
            AZ_CLASS_ALLOCATOR(BufferPoolDescriptor, SystemAllocator)
            AZ_RTTI(BufferPoolDescriptor, "{FE6EF527-6D42-4A1D-9EC8-7F3B4CD0722D}", RHI::BufferPoolDescriptor);
            static void Reflect(AZ::ReflectContext* context);

            BufferPoolDescriptor();

            uint32_t m_bufferPoolPageSizeInBytes = RHI::DefaultValues::Memory::BufferPoolPageSizeInBytes;
        };
    }
}
