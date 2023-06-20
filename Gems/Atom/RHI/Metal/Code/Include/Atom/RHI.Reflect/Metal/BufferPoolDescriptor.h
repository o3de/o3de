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
    namespace Metal
    {
        class BufferPoolDescriptor
            : public RHI::BufferPoolDescriptor
        {
        public:
            AZ_CLASS_ALLOCATOR(BufferPoolDescriptor, SystemAllocator)
            AZ_RTTI(BufferPoolDescriptor, "{037845EE-53E4-4FC9-A264-10E9C449A071}", RHI::BufferPoolDescriptor);
            static void Reflect(AZ::ReflectContext* context);

            BufferPoolDescriptor();

            uint32_t m_bufferPoolPageSizeInBytes = RHI::DefaultValues::Memory::BufferPoolPageSizeInBytes;
        };
    }
}
