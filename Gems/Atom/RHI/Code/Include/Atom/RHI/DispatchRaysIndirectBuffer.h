/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceRayTracingShaderTable.h>

namespace AZ
{
    namespace RHI
    {
        class SingleDeviceBufferPool;

        //! This class needs to be passed to the command list when submitting an indirect raytracing command
        //! The class is only relavant for DX12, other RHIs have dummy implementations
        //! For more information, see the DX12 implementation of this class
        class DispatchRaysIndirectBuffer : public Object
        {
        public:
            AZ_RTTI(AZ::RHI::DispatchRaysIndirectBuffer, "{CA9BC0E5-E43E-455B-9AFA-9C12B6107FD9}", Object)
            DispatchRaysIndirectBuffer() = default;
            virtual ~DispatchRaysIndirectBuffer() = default;
            DispatchRaysIndirectBuffer(const DispatchRaysIndirectBuffer&) = delete;
            DispatchRaysIndirectBuffer(DispatchRaysIndirectBuffer&&) = delete;
            DispatchRaysIndirectBuffer& operator=(const DispatchRaysIndirectBuffer&) = delete;
            DispatchRaysIndirectBuffer& operator=(const DispatchRaysIndirectBuffer&&) = delete;

            virtual void Init(RHI::SingleDeviceBufferPool* bufferPool) = 0;
            // This needs to be called every time the shader table changes
            virtual void Build(SingleDeviceRayTracingShaderTable* shaderTable) = 0;
        };
    } // namespace RHI
} // namespace AZ