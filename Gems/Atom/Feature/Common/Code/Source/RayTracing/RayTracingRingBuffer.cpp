/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RayTracingRingBuffer.h"

namespace AZ::Render
{
    RayTracingRingBuffer::RayTracingRingBuffer(
        const AZStd::string& bufferName, RPI::CommonBufferPoolType bufferPoolType, RHI::Format bufferFormat)
        : m_bufferName{ bufferName }
        , m_bufferPoolType{ bufferPoolType }
        , m_bufferFormat{ bufferFormat }
        , m_elementSize{ RHI::GetFormatSize(bufferFormat) }
    {
    }

    RayTracingRingBuffer::RayTracingRingBuffer(
        const AZStd::string& bufferName, RPI::CommonBufferPoolType bufferPoolType, uint32_t elementSize)
        : m_bufferName{ bufferName }
        , m_bufferPoolType{ bufferPoolType }
        , m_elementSize{ elementSize }
    {
    }

    bool RayTracingRingBuffer::IsCurrentBufferValid() const
    {
        return GetCurrentBuffer();
    }

    const Data::Instance<RPI::Buffer>& RayTracingRingBuffer::GetCurrentBuffer() const
    {
        return GetCurrentElement();
    }

    const RHI::BufferView* RayTracingRingBuffer::GetCurrentBufferView() const
    {
        return GetCurrentBuffer()->GetBufferView();
    }

    void RayTracingRingBuffer::AdvanceCurrentBufferAndUpdateData(const void* data, u64 dataSizeInBytes)
    {
        AdvanceCurrentElement();
        CreateOrResizeBuffer(dataSizeInBytes);
        UpdateCurrentBufferData(data, dataSizeInBytes, 0);
    }

    void RayTracingRingBuffer::CreateOrResizeBuffer(u64 bufferSizeInBytes)
    {
        auto& currentBuffer{ GetCurrentElement() };
        if (!currentBuffer)
        {
            RPI::CommonBufferDescriptor desc;
            desc.m_bufferName = m_bufferName;
            desc.m_poolType = m_bufferPoolType;
            desc.m_elementSize = m_elementSize;
            desc.m_elementFormat = m_bufferFormat;
            desc.m_byteCount = bufferSizeInBytes;
            currentBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }
        else if (currentBuffer->GetBufferSize() < bufferSizeInBytes)
        {
            currentBuffer->Resize(bufferSizeInBytes);
        }
    }

    void RayTracingRingBuffer::UpdateCurrentBufferData(const void* data, u64 dataSizeInBytes, u64 bufferOffsetInBytes)
    {
        auto& currentBuffer{ GetCurrentBuffer() };
        currentBuffer->UpdateData(data, dataSizeInBytes, bufferOffsetInBytes);
    }
} // namespace AZ::Render
