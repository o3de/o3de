/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/SingleDeviceBufferView.h>
#include <Atom/RHI/SingleDeviceBuffer.h>

namespace AZ::RHI
{
    ResultCode SingleDeviceBufferView::Init(const SingleDeviceBuffer& buffer, const BufferViewDescriptor& viewDescriptor)
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
                AZ_Warning("SingleDeviceBufferView", false, "Buffer view out of boundaries of buffer's memory.");
                return ResultCode::OutOfMemory;
            }
        }

        m_descriptor = viewDescriptor;
        m_hash = buffer.GetHash();
        m_hash = TypeHash64(m_descriptor.GetHash(), m_hash);
        return SingleDeviceResourceView::Init(buffer);
    }

    bool SingleDeviceBufferView::ValidateForInit(const SingleDeviceBuffer& buffer, const BufferViewDescriptor& viewDescriptor) const
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Warning("SingleDeviceBufferView", false, "Buffer view already initialized");
                return false;
            }

            if (!buffer.IsInitialized())
            {
                AZ_Warning("SingleDeviceBufferView", false, "Attempting to create view from uninitialized buffer '%s'.", buffer.GetName().GetCStr());
                return false;
            }

            if (!CheckBitsAll(buffer.GetDescriptor().m_bindFlags, viewDescriptor.m_overrideBindFlags))
            {
                AZ_Warning("SingleDeviceBufferView", false, "Buffer view has bind flags that are incompatible with the underlying buffer.");
            
                return false;
            }
        }

        return true;
    }

    const BufferViewDescriptor& SingleDeviceBufferView::GetDescriptor() const
    {
        return m_descriptor;
    }

    const SingleDeviceBuffer& SingleDeviceBufferView::GetBuffer() const
    {
        return static_cast<const SingleDeviceBuffer&>(GetResource());
    }

    bool SingleDeviceBufferView::IsFullView() const
    {
        const BufferDescriptor& bufferDescriptor = GetBuffer().GetDescriptor();
        const uint32_t bufferViewSize = m_descriptor.m_elementCount * m_descriptor.m_elementSize;
        return m_descriptor.m_elementOffset == 0 && bufferViewSize >= bufferDescriptor.m_byteCount;
    }

    HashValue64 SingleDeviceBufferView::GetHash() const
    {
        return m_hash;
    }
}
