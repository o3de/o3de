/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! Describes the primitive topology of an input assembly stream.
    enum class PrimitiveTopology : uint32_t
    {
        Undefined = 0,
        PointList,
        LineList,
        LineListAdj,
        LineStrip,
        LineStripAdj,
        TriangleList,
        TriangleListAdj,
        TriangleStrip,
        TriangleStripAdj,
        PatchList
    };

    //! Describes the rate at which an input assembly channel increments
    //! to the next element in the buffer stream. If Vertex, the value
    //! is assumed to be a per-vertex component. If Instance, the value
    //! is assumed to be a per-hardware-instance component.
    enum class StreamStepFunction : uint32_t
    {
        Constant = 0,
        PerVertex,
        PerInstance,
        PerPatch,
        PerPatchControlPoint
    };

    //! Describes an instance of a stream channel within a stream layout. A stream channel
    //! corresponds to a single data element on the input vertex, and it has a semantic associated
    //! with it which matches the element within the shader. The data in a stream channel can be
    //! offset from the base of the parent StreamBufferView to interleave multiple channels within 
    //! the same buffer.
    class StreamChannelDescriptor
    {
    public:
        AZ_TYPE_INFO(StreamChannelDescriptor, "{BF99DCBE-C30B-443A-A92C-B07EE6F0FB1D}");
        static void Reflect(AZ::ReflectContext* context);
        StreamChannelDescriptor() = default;

        StreamChannelDescriptor(
            ShaderSemantic semantic,
            Format format,
            uint32_t byteOffset,
            uint32_t bufferIndex);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        /// The semantic of the stream channel.
        ShaderSemantic m_semantic;

        /// The format of each element in the channel.
        Format m_format = Format::Unknown;

        /// The parent stream buffer index.
        uint32_t m_bufferIndex = 0;

        /// Byte offset from the base of the StreamBufferView to the first element in this channel. 
        /// Used to interleave multiple channels in a single buffer.
        uint32_t m_byteOffset = 0;
    };

    //! Describes an instance of a StreamBufferView within the stream layout. Each stream
    //! buffer provides new data to the shader at a specified step rate. The byte stride
    //! is the total width of a single element in the buffer stream.
    class StreamBufferDescriptor
    {
    public:
        AZ_TYPE_INFO(StreamBufferDescriptor, "{F1295422-9505-45EF-9E0D-47839B755F8C}");
        static void Reflect(AZ::ReflectContext* context);
        StreamBufferDescriptor() = default;

        StreamBufferDescriptor(
            StreamStepFunction stepFunction,
            uint32_t stepRate,
            uint32_t byteStride);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        /// The unit frequency at which data is presented to the vertex shader.
        StreamStepFunction m_stepFunction = StreamStepFunction::PerVertex;

        /// The interval (in per-frequency units) at which the data is pushed to the vertex shader.
        uint32_t m_stepRate = 1;

        //! The distance in bytes between consecutive vertex entries in the buffer.
        //! This will match the stride value in StreamBufferView. It is provided separately here
        //! because some platforms may require this information at PSO compile time, while the 
        //! StreamBufferView is not available until draw time.
        uint32_t m_byteStride = 0;
    };

    //! Describes the input assembly stream buffer layout for the pipeline state.
    //! This is provided to the RHI back-end at PSO compile time as part of the PipelineStateDescriptor.
    //!
    //! See InputStreamLayoutBuilder for a convenient way to construct InputStreamLayout objects.
    class InputStreamLayout
    {
    public:
        AZ_TYPE_INFO(InputStreamLayout, "{2F628C89-76F7-458C-9DCE-2A2FFD4530E1}");
        static void Reflect(AZ::ReflectContext* context);
        InputStreamLayout() = default;

        //! Clears the descriptor back to an empty state.
        void Clear();

        //! Finalizes the descriptor and builds the hash.
        bool Finalize();

        //! Returns whether Finalize() was called.
        bool IsFinalized() const;

        //! Sets the primitive topology to the descriptor.
        void SetTopology(PrimitiveTopology topology);

        //! Adds a stream channel to the descriptor.
        void AddStreamChannel(const StreamChannelDescriptor& descriptor);

        //! Adds a stream buffer to the descriptor.
        void AddStreamBuffer(const StreamBufferDescriptor& descriptor);

        //! Returns the primitive topology.
        const PrimitiveTopology GetTopology() const;

        //! Returns the list of stream channels.
        AZStd::span<const StreamChannelDescriptor> GetStreamChannels() const;

        //! Returns the list of stream buffers.
        AZStd::span<const StreamBufferDescriptor> GetStreamBuffers() const;

        //! Returns the hash computed in Finalize(), which must be called first.
        HashValue64 GetHash() const;

        bool operator == (const InputStreamLayout& rhs) const;

    private:
        /// The primitive supported by the stream layout.
        PrimitiveTopology m_topology = PrimitiveTopology::Undefined;

        /// An array of state data describing how vertex channels are stored in memory with respect
        /// to their parent stream buffers.
        AZStd::fixed_vector<StreamChannelDescriptor, RHI::Limits::Pipeline::StreamChannelCountMax> m_streamChannels;

        /// An array of state data describing how stream buffers are fetched by the vertex hardware.
        AZStd::fixed_vector<StreamBufferDescriptor, RHI::Limits::Pipeline::StreamCountMax> m_streamBuffers;

        /// Computed hash created at initialization time.
        HashValue64 m_hash = HashValue64{ 0 };
    };
}
