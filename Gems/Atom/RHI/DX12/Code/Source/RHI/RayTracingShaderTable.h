/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>


namespace AZ
{
    namespace DX12
    {
        class Buffer;

        class RayTracingShaderTable final
            : public RHI::DeviceRayTracingShaderTable
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingShaderTable, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingShaderTable> Create();

            struct ShaderTableBuffers
            {
                RHI::Ptr<Buffer> m_rayGenerationTable;
                uint32_t m_rayGenerationTableSize = 0;

                RHI::Ptr<Buffer> m_missTable;
                uint32_t m_missTableSize = 0;
                uint32_t m_missTableStride = 0;

                RHI::Ptr<Buffer> m_callableTable;
                uint32_t m_callableTableSize = 0;
                uint32_t m_callableTableStride = 0;

                RHI::Ptr<Buffer> m_hitGroupTable;
                uint32_t m_hitGroupTableSize = 0;
                uint32_t m_hitGroupTableStride = 0;
            };

            const ShaderTableBuffers& GetBuffers() const { return m_buffers.GetCurrentElement(); }

        private:

            RayTracingShaderTable() = default;

#ifdef AZ_DX12_DXR_SUPPORT
            uint32_t FindLargestRecordSize(const RHI::DeviceRayTracingShaderTableRecordList& recordList);
            RHI::Ptr<Buffer> BuildTable(RHI::Device& deviceBase,
                                        const RHI::DeviceRayTracingBufferPools& bufferPools,
                                        const RHI::DeviceRayTracingShaderTableRecordList& recordList,
                                        uint32_t shaderRecordSize,
                                        AZStd::wstring shaderTableName,
                                        Microsoft::WRL::ComPtr<ID3D12StateObjectProperties>& stateObjectProperties);
#endif

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingShaderTable
            RHI::ResultCode BuildInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::FrameCountMaxRingBuffer<ShaderTableBuffers> m_buffers;
        };
    }
}
