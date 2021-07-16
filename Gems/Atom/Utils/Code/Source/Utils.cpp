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
        AZ::Outcome<void> WriteToBuffer(RHI::Ptr<RHI::Buffer> buffer, const void* data, size_t dataSize)
        {
            // Map the buffer, copy data to the mapped pointer, and unmap it.

            if (dataSize > buffer->GetDescriptor().m_byteCount)
            {
                AZ_Error("AZ::Render::WriteToBuffer", false, "Unable to map buffer %s, provided data is too big.", buffer->GetName().GetCStr());
                return AZ::Failure();
            }

            if (dataSize == 0)
            {
                AZ_Warning("AZ::Render::WriteToBuffer", false, "Provided data was empty, no data was written to the buffer.");
                return AZ::Failure();
            }

            RHI::BufferMapRequest mapRequest(*buffer, 0, dataSize);
            RHI::BufferMapResponse response;
            RHI::BufferPool* bufferPool = static_cast<RHI::BufferPool*>(buffer->GetPool());
            bufferPool->MapBuffer(mapRequest, response);

            if (response.m_data)
            {
                memcpy(response.m_data, data, dataSize);
                bufferPool->UnmapBuffer(*buffer);
            }
            else
            {
                AZ_Error("AZ::Render::WriteToBuffer", false, "Unable to map buffer: %s, map request failed.", buffer->GetName().GetCStr());
                return AZ::Failure();
            }
            return AZ::Success();
        }
    } // namespace Render
} // namespace AZ
