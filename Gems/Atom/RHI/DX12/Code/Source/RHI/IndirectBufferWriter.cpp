/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Buffer.h>
#include <RHI/IndirectBufferWriter.h>
#include <RHI/IndirectBufferSignature.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<IndirectBufferWriter> IndirectBufferWriter::Create()
        {
            return aznew IndirectBufferWriter();
        }

        void IndirectBufferWriter::SetVertexViewInternal(RHI::IndirectCommandIndex index, const RHI::DeviceStreamBufferView& view)
        {
            const Buffer* buffer = static_cast<const Buffer*>(view.GetBuffer());
            D3D12_VERTEX_BUFFER_VIEW* command = reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW*>(GetCommandTargetMemory(index));
            command->BufferLocation = buffer->GetMemoryView().GetGpuAddress() + view.GetByteOffset();
            command->SizeInBytes = view.GetByteCount();
            command->StrideInBytes = view.GetByteStride();
        }

        void IndirectBufferWriter::SetIndexViewInternal(RHI::IndirectCommandIndex index, const RHI::DeviceIndexBufferView& indexBufferView)
        {
            const Buffer* indexBuffer = static_cast<const Buffer*>(indexBufferView.GetBuffer());
            D3D12_INDEX_BUFFER_VIEW* command = reinterpret_cast<D3D12_INDEX_BUFFER_VIEW*>(GetCommandTargetMemory(index));
            command->BufferLocation = indexBuffer->GetMemoryView().GetGpuAddress() + indexBufferView.GetByteOffset();
            command->Format = (indexBufferView.GetIndexFormat() == RHI::IndexFormat::Uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            command->SizeInBytes = indexBufferView.GetByteCount();
        }

        void IndirectBufferWriter::DrawInternal(RHI::IndirectCommandIndex index, const RHI::DrawLinear& arguments, const RHI::DrawInstanceArguments& m_drawInstanceArgs)
        {
            D3D12_DRAW_ARGUMENTS* command = reinterpret_cast<D3D12_DRAW_ARGUMENTS*>(GetCommandTargetMemory(index));
            command->VertexCountPerInstance = arguments.m_vertexCount;
            command->InstanceCount = m_drawInstanceArgs.m_instanceCount;
            command->StartVertexLocation = arguments.m_vertexOffset;
            command->StartInstanceLocation = m_drawInstanceArgs.m_instanceOffset;
        }

        void IndirectBufferWriter::DrawIndexedInternal(RHI::IndirectCommandIndex index, const RHI::DrawIndexed& arguments, const RHI::DrawInstanceArguments& m_drawInstanceArgs)
        {
            D3D12_DRAW_INDEXED_ARGUMENTS * command = reinterpret_cast<D3D12_DRAW_INDEXED_ARGUMENTS *>(GetCommandTargetMemory(index));
            command->IndexCountPerInstance = arguments.m_indexCount;
            command->InstanceCount = m_drawInstanceArgs.m_instanceCount;
            command->StartIndexLocation = arguments.m_indexOffset;
            command->BaseVertexLocation = arguments.m_vertexOffset;
            command->StartInstanceLocation = m_drawInstanceArgs.m_instanceOffset;
        }

        void IndirectBufferWriter::DispatchInternal(RHI::IndirectCommandIndex index, const RHI::DispatchDirect& arguments)
        {
            D3D12_DISPATCH_ARGUMENTS* command = reinterpret_cast<D3D12_DISPATCH_ARGUMENTS*>(GetCommandTargetMemory(index));
            command->ThreadGroupCountX = arguments.GetNumberOfGroupsX();
            command->ThreadGroupCountY = arguments.GetNumberOfGroupsY();
            command->ThreadGroupCountZ = arguments.GetNumberOfGroupsZ();
        }

        void IndirectBufferWriter::SetRootConstantsInternal(
            RHI::IndirectCommandIndex index,
            const uint8_t* data,
            uint32_t byteSize)
        {
            ::memcpy(GetCommandTargetMemory(index), data, byteSize);
        }

        uint8_t* IndirectBufferWriter::GetCommandTargetMemory(RHI::IndirectCommandIndex index) const
        {
            return GetTargetMemory() + GetCurrentSequenceIndex() * m_sequenceStride + m_signature->GetOffset(index);
        }

    }
}
