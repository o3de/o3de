/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceRayTracingShaderTable.h>

namespace AZ
{
    namespace RHI
    {
        class DeviceBufferPool;

        //! This class needs to be passed to the command list when submitting an indirect raytracing command
        //! The class is only relavant for DX12, other RHIs have dummy implementations
        //! For more information, see the DX12 implementation of this class
        class DeviceDispatchRaysIndirectBuffer : public Object
        {
        public:
            AZ_RTTI(AZ::RHI::DeviceDispatchRaysIndirectBuffer, "{CA9BC0E5-E43E-455B-9AFA-9C12B6107FD9}", Object)
            DeviceDispatchRaysIndirectBuffer() = default;
            virtual ~DeviceDispatchRaysIndirectBuffer() = default;
            DeviceDispatchRaysIndirectBuffer(const DeviceDispatchRaysIndirectBuffer&) = delete;
            DeviceDispatchRaysIndirectBuffer(DeviceDispatchRaysIndirectBuffer&&) = delete;
            DeviceDispatchRaysIndirectBuffer& operator=(const DeviceDispatchRaysIndirectBuffer&) = delete;
            DeviceDispatchRaysIndirectBuffer& operator=(const DeviceDispatchRaysIndirectBuffer&&) = delete;

            virtual void Init(RHI::DeviceBufferPool* bufferPool) = 0;
            // This needs to be called every time the shader table changes
            virtual void Build(DeviceRayTracingShaderTable* shaderTable) = 0;
        };
    } // namespace RHI
} // namespace AZ