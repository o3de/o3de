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
            AZ_RTTI(BufferPoolDescriptor, "{037845EE-53E4-4FC9-A264-10E9C449A071}", RHI::BufferPoolDescriptor);
            static void Reflect(AZ::ReflectContext* context);

            BufferPoolDescriptor();

            uint32_t m_bufferPoolPageSizeInBytes = RHI::DefaultValues::Memory::BufferPoolPageSizeInBytes;
        };
    }
}
