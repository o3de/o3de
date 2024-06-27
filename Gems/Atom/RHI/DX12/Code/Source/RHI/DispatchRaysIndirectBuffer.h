/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "RHI/MemoryView.h"
#include <Atom/RHI/DeviceDispatchRaysIndirectBuffer.h>
#include <Atom/RHI/DeviceIndirectBufferView.h>

struct ID3D12GraphicsCommandList;
namespace AZ
{
    namespace DX12
    {
        // This class exists, because the buffer for an indirect raytracing call, in D3D12, has a different layout than e.g. the Vulkan one.
        // In D3D12, the buffer contains the shader table for the raytracing call.
        // This means, we can't use the indirect ray dispatch buffer, usually passed using a slot of the pass.
        // This class is reponsible for copying the shader table, and the actual indirect raytracing arguments to a DX12 specific buffer
        // The buffer is then used by the commandlist as the argument buffer
        class DispatchRaysIndirectBuffer : public RHI::DeviceDispatchRaysIndirectBuffer
        {
        public:
            AZ_RTTI(AZ::DX12::DispatchRaysIndirectBuffer, "{623567A8-5A73-46FE-BAE6-6B8F59BDEFB3}", RHI::DeviceDispatchRaysIndirectBuffer)
            DispatchRaysIndirectBuffer() = default;
            virtual ~DispatchRaysIndirectBuffer() = default;
            DispatchRaysIndirectBuffer(const DispatchRaysIndirectBuffer&) = delete;
            DispatchRaysIndirectBuffer(DispatchRaysIndirectBuffer&&) = delete;
            DispatchRaysIndirectBuffer& operator=(const DispatchRaysIndirectBuffer&) = delete;
            DispatchRaysIndirectBuffer& operator=(const DispatchRaysIndirectBuffer&&) = delete;

            static RHI::Ptr<DispatchRaysIndirectBuffer> Create();

            void Init(RHI::DeviceBufferPool* bufferPool) override;
            void Build(RHI::DeviceRayTracingShaderTable* shaderTable) override;

            RHI::Ptr<RHI::DeviceBuffer> m_buffer;
            MemoryView m_shaderTableStagingMemory;
            bool m_shaderTableNeedsCopy = false;
        };
    } // namespace DX12
} // namespace AZ