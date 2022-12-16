/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace RHI
    {
        ResultCode BufferView::Init(const Buffer& buffer, const BufferViewDescriptor& viewDescriptor)
        {
            if (!ValidateForInit(buffer, viewDescriptor))
            {
                return ResultCode::InvalidOperation;
            }
            if (Validation::IsEnabled())
            {
                // Check buffer view does not reach outside buffer's memory
                if (buffer.GetDescriptor().m_byteCount < (viewDescriptor.m_elementOffset + viewDescriptor.m_elementCount) * viewDescriptor.m_elementSize)
                {
                    AZ_Warning("BufferView", false, "Buffer view out of boundaries of buffer's memory.");
                    return ResultCode::OutOfMemory;
                }
            }

            m_descriptor = viewDescriptor;
            m_hash = buffer.GetHash();
            m_hash = TypeHash64(m_descriptor.GetHash(), m_hash);
            return ResourceView::Init(buffer);
        }

        bool BufferView::ValidateForInit(const Buffer& buffer, const BufferViewDescriptor& viewDescriptor) const
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Warning("BufferView", false, "Buffer view already initialized");
                    return false;
                }

                if (!buffer.IsInitialized())
                {
                    AZ_Warning("BufferView", false, "Attempting to create view from uninitialized buffer '%s'.", buffer.GetName().GetCStr());
                    return false;
                }

                if (!CheckBitsAll(buffer.GetDescriptor().m_bindFlags, viewDescriptor.m_overrideBindFlags))
                {
                    AZ_Warning("BufferView", false, "Buffer view has bind flags that are incompatible with the underlying buffer.");
            
                    return false;
                }
            }

            return true;
        }

        const BufferViewDescriptor& BufferView::GetDescriptor() const
        {
            return m_descriptor;
        }

        const Buffer& BufferView::GetBuffer() const
        {
            return static_cast<const Buffer&>(GetResource());
        }

        Ptr<DeviceResourceView> BufferView::GetDeviceResourceView(int deviceIndex) const
        {
            return GetDeviceBufferView(deviceIndex);
        }

        bool BufferView::IsFullView() const
        {
            const BufferDescriptor& bufferDescriptor = GetBuffer().GetDescriptor();
            const uint32_t bufferViewSize = m_descriptor.m_elementCount * m_descriptor.m_elementSize;
            return m_descriptor.m_elementOffset == 0 && bufferViewSize >= bufferDescriptor.m_byteCount;
        }

        HashValue64 BufferView::GetHash() const
        {
            return m_hash;
        }

        ResultCode BufferView::InitInternal(const Resource& resource)
        {
            const Buffer& buffer = static_cast<const Buffer&>(resource);

            IterateDevices(
                [this, &buffer](int deviceIndex)
                {
                    m_deviceBufferViews[deviceIndex] = Factory::Get().CreateBufferView();
                    m_deviceBufferViews[deviceIndex]->Init(*buffer.m_deviceBuffers.at(deviceIndex), m_descriptor);
                    return true;
                });
            return ResultCode::Success;
        }

        void BufferView::ShutdownInternal()
        {
            InvalidateInternal();
        }

        ResultCode BufferView::InvalidateInternal()
        {
            m_deviceBufferViews.clear();
            return ResultCode::Success;
        }
    }
}
