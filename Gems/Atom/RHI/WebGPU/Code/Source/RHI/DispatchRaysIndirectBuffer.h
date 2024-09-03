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
    namespace WebGPU
    {
        class DispatchRaysIndirectBuffer : public RHI::DeviceDispatchRaysIndirectBuffer
        {
        public:
            AZ_RTTI(AZ::WebGPU::DispatchRaysIndirectBuffer, "{70741F5F-14F2-41BC-A779-3D6947A50BDC}", RHI::DeviceDispatchRaysIndirectBuffer)
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

            void Init(RHI::DeviceBufferPool*) override
            {
            }
            void Build(RHI::DeviceRayTracingShaderTable*) override
            {
            }
        };
    } // namespace WebGPU
} // namespace AZ