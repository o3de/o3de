/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>

namespace AZ::RHI
{

    bool ValidateStreamBufferViews(const InputStreamLayout& inputStreamLayout, RHI::GeometryView& geometryView,
        const RHI::StreamBufferIndices& streamIndices)
    {
        bool validStreams = true;

        if (Validation::IsEnabled())
        {
            if (!inputStreamLayout.IsFinalized())
            {
                AZ_Assert(false, "InputStreamLayout is not finalized.");
                validStreams = false;
            }

            size_t streamCount = 0;
            auto streamIter = geometryView.CreateStreamIterator(streamIndices);

            for (; !streamIter.HasEnded(); ++streamIter, ++streamCount)
            {
                auto bufferDescriptors = inputStreamLayout.GetStreamBuffers();
                auto& bufferDescriptor = bufferDescriptors[streamCount];
                auto& bufferView = *streamIter;

                // It can be valid to have a null buffer if this stream is not actually used by the shader, which can be the case for streams marked optional.
                if (bufferView.GetBuffer() == nullptr)
                {
                    continue;
                }

                if (bufferDescriptor.m_byteStride != bufferView.GetByteStride())
                {
                    AZ_Assert(false, "InputStreamLayout's buffer[%d] has stride=%d but StreamBufferView[%d] has stride=%d.",
                        streamCount, bufferDescriptor.m_byteStride, streamCount, bufferView.GetByteStride());
                    validStreams = false;
                }
            }

            if (streamCount != inputStreamLayout.GetStreamBuffers().size())
            {
                AZ_Assert(false, "InputStreamLayout references %d stream buffers but %d StreamBufferViews passed the mask check.",
                    inputStreamLayout.GetStreamBuffers().size(), streamCount);
                validStreams = false;
            }
        }

        return validStreams;
    }

}
