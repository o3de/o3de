/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Buffer/RingBuffer.h>

namespace AZ::RPI
{
    RingBuffer::RingBuffer(const AZStd::string& bufferName, CommonBufferPoolType bufferPoolType, RHI::Format bufferFormat)
        : m_bufferName{ bufferName }
        , m_bufferPoolType{ bufferPoolType }
        , m_bufferFormat{ bufferFormat }
        , m_elementSize{ RHI::GetFormatSize(bufferFormat) }
    {
    }

    RingBuffer::RingBuffer(const AZStd::string& bufferName, CommonBufferPoolType bufferPoolType, uint32_t elementSize)
        : m_bufferName{ bufferName }
        , m_bufferPoolType{ bufferPoolType }
        , m_elementSize{ elementSize }
    {
    }

    bool RingBuffer::IsCurrentBufferValid() const
    {
        return GetCurrentBuffer();
    }

    const Data::Instance<Buffer>& RingBuffer::GetCurrentBuffer() const
    {
        return GetCurrentElement();
    }

    const RHI::BufferView* RingBuffer::GetCurrentBufferView() const
    {
        return GetCurrentBuffer()->GetBufferView();
    }

    void RingBuffer::AdvanceCurrentBufferAndUpdateData(const void* data, u64 dataSizeInBytes)
    {
        AdvanceCurrentElement();
        CreateOrResizeCurrentBuffer(dataSizeInBytes);
        UpdateCurrentBufferData(data, dataSizeInBytes, 0);
    }

    void RingBuffer::AdvanceCurrentBufferAndUpdateData(const AZStd::unordered_map<int, const void *> &data, u64 dataSizeInBytes)
    {
        AdvanceCurrentElement();
        CreateOrResizeCurrentBuffer(dataSizeInBytes);
        UpdateCurrentBufferData(data, dataSizeInBytes, 0);
    }

    void RingBuffer::CreateOrResizeCurrentBuffer(u64 bufferSizeInBytes)
    {
        auto& currentBuffer{ GetCurrentElement() };
        if (!currentBuffer)
        {
            CommonBufferDescriptor desc;
            desc.m_bufferName = m_bufferName;
            desc.m_poolType = m_bufferPoolType;
            desc.m_elementSize = m_elementSize;
            desc.m_elementFormat = m_bufferFormat;
            desc.m_byteCount = bufferSizeInBytes;
            currentBuffer = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }
        else if (currentBuffer->GetBufferSize() < bufferSizeInBytes)
        {
            currentBuffer->Resize(bufferSizeInBytes);
        }
    }

    void RingBuffer::UpdateCurrentBufferData(const void* data, u64 dataSizeInBytes, u64 bufferOffsetInBytes)
    {
        auto& currentBuffer{ GetCurrentBuffer() };
        currentBuffer->UpdateData(data, dataSizeInBytes, bufferOffsetInBytes);
    }

    void RingBuffer::UpdateCurrentBufferData(const AZStd::unordered_map<int, const void*>& data, u64 dataSizeInBytes, u64 bufferOffsetInBytes)
    {
        auto& currentBuffer{ GetCurrentBuffer() };
        currentBuffer->UpdateData(data, dataSizeInBytes, bufferOffsetInBytes);
    }
} // namespace AZ::RPI
