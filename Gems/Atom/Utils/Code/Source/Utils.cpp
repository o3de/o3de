/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/Utils.h>

namespace AZ
{
    namespace Render
    {
        AZ::Outcome<void> WriteToBuffer(RHI::Ptr<RHI::DeviceBuffer> buffer, const void* data, size_t dataSize)
        {
            // Map the buffer, copy data to the mapped pointer, and unmap it.
            RHI::DeviceBufferMapRequest mapRequest(*buffer, 0, dataSize);
            RHI::DeviceBufferMapResponse response;
            RHI::DeviceBufferPool* bufferPool = static_cast<RHI::DeviceBufferPool*>(buffer->GetPool());
            RHI::ResultCode resultCode = bufferPool->MapBuffer(mapRequest, response);

            if (resultCode != RHI::ResultCode::Success)
            {
                return AZ::Failure();
            }

            if (response.m_data)
            {
                memcpy(response.m_data, data, dataSize);
                bufferPool->UnmapBuffer(*buffer);
            }

            return AZ::Success();
        }
    } // namespace Render
} // namespace AZ
