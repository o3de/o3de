/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceDispatchRaysIndirectBuffer.h>

namespace AZ
{
    namespace Metal
    {
        class DispatchRaysIndirectBuffer : public RHI::DeviceDispatchRaysIndirectBuffer
        {
        public:
            AZ_RTTI(AZ::Metal::DispatchRaysIndirectBuffer, "{D6767937-A314-43A8-82BB-8F8E7D32A937}", RHI::DispatchRaysIndirectBuffer)
            DispatchRaysIndirectBuffer() = default;
            virtual ~DispatchRaysIndirectBuffer() = default;
            DispatchRaysIndirectBuffer(const DispatchRaysIndirectBuffer&) = delete;
            DispatchRaysIndirectBuffer(DispatchRaysIndirectBuffer&&) = delete;
            DispatchRaysIndirectBuffer& operator=(const DispatchRaysIndirectBuffer&) = delete;
            DispatchRaysIndirectBuffer& operator=(const DispatchRaysIndirectBuffer&&) = delete;

            static RHI::Ptr<DispatchRaysIndirectBuffer> Create()
            {
                return aznew DispatchRaysIndirectBuffer;
            }

            void Init(RHI::DeviceBufferPool*, RHI::DeviceBufferPool*) override
            {
            }
            void Build(RHI::DeviceRayTracingShaderTable*) override
            {
            }
        };
    } // namespace Metal
} // namespace AZ