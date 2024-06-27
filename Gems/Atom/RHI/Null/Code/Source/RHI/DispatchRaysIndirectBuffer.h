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
    namespace Null
    {
        class DispatchRaysIndirectBuffer : public RHI::DeviceDispatchRaysIndirectBuffer
        {
        public:
            AZ_RTTI(AZ::Null::DispatchRaysIndirectBuffer, "{BE747193-DF9E-49ED-8EC8-2348044F11B2}", RHI::DeviceDispatchRaysIndirectBuffer)
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
    } // namespace Null
} // namespace AZ