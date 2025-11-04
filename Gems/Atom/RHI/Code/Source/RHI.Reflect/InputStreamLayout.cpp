/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    StreamChannelDescriptor::StreamChannelDescriptor(
        ShaderSemantic semantic,
        Format format,
        uint32_t byteOffset,
        uint32_t bufferIndex)
        : m_semantic{AZStd::move(semantic)}
        , m_format{format}
        , m_byteOffset{byteOffset}
        , m_bufferIndex{bufferIndex}
    {}

    HashValue64 StreamChannelDescriptor::GetHash(HashValue64 seed) const
    {
        seed = m_semantic.GetHash(seed);
        seed = TypeHash64(m_format, seed);
        seed = TypeHash64(m_byteOffset, seed);
        seed = TypeHash64(m_bufferIndex, seed);
        return seed;
    }

    void StreamChannelDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<StreamChannelDescriptor>()
                ->Version(1)
                ->Field("m_semantic", &StreamChannelDescriptor::m_semantic)
                ->Field("m_format", &StreamChannelDescriptor::m_format)
                ->Field("m_byteOffset", &StreamChannelDescriptor::m_byteOffset)
                ->Field("m_bufferIndex", &StreamChannelDescriptor::m_bufferIndex);
        }
    }

    StreamBufferDescriptor::StreamBufferDescriptor(
        StreamStepFunction stepFunction,
        uint32_t stepRate,
        uint32_t byteStride)
        : m_stepFunction{stepFunction}
        , m_stepRate{stepRate}
        , m_byteStride{byteStride}
    {}

    HashValue64 StreamBufferDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_stepFunction, seed);
        seed = TypeHash64(m_stepRate, seed);
        seed = TypeHash64(m_byteStride, seed);
        return seed;
    }

    void StreamBufferDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<StreamBufferDescriptor>()
                ->Version(1)
                ->Field("m_stepFunction", &StreamBufferDescriptor::m_stepFunction)
                ->Field("m_stepRate", &StreamBufferDescriptor::m_stepRate)
                ->Field("m_byteStride", &StreamBufferDescriptor::m_byteStride);
        }
    }

    void InputStreamLayout::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<InputStreamLayout>()
                ->Version(3)
                ->Field("m_topology", &InputStreamLayout::m_topology)
                ->Field("m_streamChannels", &InputStreamLayout::m_streamChannels)
                ->Field("m_streamBuffers", &InputStreamLayout::m_streamBuffers)
                ->Field("m_hash", &InputStreamLayout::m_hash);
        }
    }

    void InputStreamLayout::Clear()
    {
        m_topology = PrimitiveTopology::Undefined;
        m_streamChannels.clear();
        m_streamBuffers.clear();
        m_hash = HashValue64{ 0 };
    }

    bool InputStreamLayout::Finalize()
    {
        if (Validation::IsEnabled())
        {
            for (const auto& channelDescriptor : m_streamChannels)
            {
                if (channelDescriptor.m_bufferIndex >= m_streamBuffers.size())
                {
                    AZ_Error("InputStreamLayout", false, "Channel %s[%d] references buffer index %d which does not exist.",
                        channelDescriptor.m_semantic.m_name.GetCStr(), channelDescriptor.m_semantic.m_index, channelDescriptor.m_bufferIndex);
                    return false;
                }
            }

            if (m_topology == PrimitiveTopology::Undefined)
            {
                AZ_Error("InputStreamLayout", false, "Topology is undefined.");
                return false;
            }
        }

        HashValue64 seed = TypeHash64(m_topology, HashValue64{ 0 });

        for (const StreamChannelDescriptor& channel : m_streamChannels)
        {
            seed = channel.GetHash(seed);
        }

        for (const StreamBufferDescriptor& buffer : m_streamBuffers)
        {
            seed = buffer.GetHash(seed);
        }

        m_hash = seed;

        return true;
    }

    bool InputStreamLayout::IsFinalized() const
    {
        return m_hash != HashValue64{ 0 };
    }

    void InputStreamLayout::SetTopology(PrimitiveTopology topology)
    {
        m_topology = topology;
    }

    void InputStreamLayout::AddStreamChannel(const StreamChannelDescriptor& descriptor)
    {
        m_streamChannels.push_back(descriptor);
    }

    void InputStreamLayout::AddStreamBuffer(const StreamBufferDescriptor& descriptor)
    {
        m_streamBuffers.push_back(descriptor);
    }

    const PrimitiveTopology InputStreamLayout::GetTopology() const
    {
        return m_topology;
    }

    AZStd::span<const StreamChannelDescriptor> InputStreamLayout::GetStreamChannels() const
    {
        return m_streamChannels;
    }

    AZStd::span<const StreamBufferDescriptor> InputStreamLayout::GetStreamBuffers() const
    {
        return m_streamBuffers;
    }

    HashValue64 InputStreamLayout::GetHash() const
    {
        return m_hash;
    }
   
    bool InputStreamLayout::operator == (const InputStreamLayout& rhs) const
    {
        bool same = (m_streamChannels.size() == rhs.m_streamChannels.size() && m_streamBuffers.size() == rhs.m_streamBuffers.size());
        if (same)
        {
            for (size_t index = 0; index < m_streamChannels.size(); index++)
            {
                if (m_streamChannels[index].GetHash() != rhs.m_streamChannels[index].GetHash())
                {
                    same = false;
                    break;
                }
            }

            if (same)
            {
                for (size_t index = 0; index < m_streamBuffers.size(); index++)
                {
                    if (m_streamBuffers[index].GetHash() != rhs.m_streamBuffers[index].GetHash())
                    {
                        same = false;
                        break;
                    }
                }
            }
        }

        return same && m_hash == rhs.m_hash && m_topology == rhs.m_topology;
    }
}
