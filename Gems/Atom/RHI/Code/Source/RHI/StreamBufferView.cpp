/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>

namespace AZ
{
    namespace RHI
    {
        StreamBufferView::StreamBufferView(
            const Buffer& buffer,
            uint32_t byteOffset,
            uint32_t byteCount,
            uint32_t byteStride)
            : m_buffer{&buffer}
            , m_byteOffset{byteOffset}
            , m_byteCount{byteCount}
            , m_byteStride{byteStride}
        {
            size_t seed = 0;
            AZStd::hash_combine(seed, m_buffer);
            AZStd::hash_combine(seed, m_byteOffset);
            AZStd::hash_combine(seed, m_byteCount);
            AZStd::hash_combine(seed, m_byteStride);
            m_hash = static_cast<HashValue64>(seed);
        }

        HashValue64 StreamBufferView::GetHash() const
        {
            return m_hash;
        }

        const Buffer* StreamBufferView::GetBuffer() const
        {
            return m_buffer;
        }

        uint32_t StreamBufferView::GetByteOffset() const
        {
            return m_byteOffset;
        }

        uint32_t StreamBufferView::GetByteCount() const
        {
            return m_byteCount;
        }

        uint32_t StreamBufferView::GetByteStride() const
        {
            return m_byteStride;
        }

        bool ValidateStreamBufferViews(const RHI::InputStreamLayout& inputStreamLayout, AZStd::array_view<RHI::StreamBufferView> streamBufferViews)
        {
            bool ok = true;

            if (Validation::IsEnabled())
            {
                if (!inputStreamLayout.IsFinalized())
                {
                    AZ_Error("InputStreamLayout", false, "InputStreamLayout is not finalized.");
                    ok = false;
                }

                if (inputStreamLayout.GetStreamBuffers().size() != streamBufferViews.size())
                {
                    AZ_Error("InputStreamLayout", false, "InputStreamLayout references %d stream buffers but %d StreamBufferViews were provided.",
                        inputStreamLayout.GetStreamBuffers().size(), streamBufferViews.size());
                    ok = false;
                }

                for (int i = 0; i < inputStreamLayout.GetStreamBuffers().size() && i < streamBufferViews.size(); ++i)
                {
                    auto& bufferDescriptor = inputStreamLayout.GetStreamBuffers()[i];
                    auto& bufferView = streamBufferViews[i];

                    // It can be valid to have a null buffer if this stream is not actually used by the shader, which can be the case for streams marked optional.
                    if (bufferView.GetBuffer() == nullptr)
                    {
                        continue;
                    }

                    if (bufferDescriptor.m_byteStride != bufferView.GetByteStride())
                    {
                        AZ_Error("InputStreamLayout", false, "InputStreamLayout's buffer[%d] has stride=%d but StreamBufferView[%d] has stride=%d.",
                            i, bufferDescriptor.m_byteStride, i, bufferView.GetByteStride());
                        ok = false;
                    }
                }
            }

            return ok;
        }
    }
}
