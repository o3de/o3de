/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/InputStreamLayout.h>

namespace AZ::RHI
{
    //! Provides a convenient way to construct InputStreamLayout objects, which describes
    //! the input assembly stream buffer layout for the pipeline state.
    //!
    //! The general usage includes adding one or more stream buffer descriptors, and 
    //! adding one or more channels descriptors to each buffer. Examples are shown below.
    //!
    //! 1) Individual Stream Buffers - each stream channel is contained in a separate buffers
    //!    @code
    //!      RHI::InputStreamLayoutBuilder layoutBuilder;
    //!      layoutBuilder.AddBuffer()->Channel("POSITION", RHI::Format::R32G32B32_FLOAT);
    //!      layoutBuilder.AddBuffer()->Channel("COLOR", RHI::Format::R32G32B32A32_FLOAT);
    //!      layoutBuilder.AddBuffer()->Channel("UV", RHI::Format::R32G32_FLOAT);
    //!      layout = layoutBuilder.End();
    //!    @endcode
    //!
    //! 2) Interleaved Stream Buffers - a single buffer contains all stream channels
    //!    @code
    //!      RHI::InputStreamLayoutBuilder layoutBuilder;
    //!      layoutBuilder.AddBuffer()
    //!          ->Channel("POSITION", RHI::Format::R32G32B32_FLOAT)
    //!          ->Channel("COLOR", RHI::Format::R8G8B8A8_UNORM)
    //!          ->Channel("UV", RHI::Format::R32G32_FLOAT);
    //!      layout = layoutBuilder.End();
    //!    @endcode
    //!
    //! 3) Multiple Interleaved Stream Buffers - multiple buffers with multiple channels
    //!    @code
    //!      RHI::InputStreamLayoutBuilder layoutBuilder;
    //!      layoutBuilder.AddBuffer()
    //!          ->Channel("POSITION", RHI::Format::R32G32B32_FLOAT)
    //!          ->Channel("COLOR", RHI::Format::R32G32B32A32_FLOAT);
    //!      layoutBuilder.AddBuffer()
    //!          ->Channel("UV0", RHI::Format::R32G32_FLOAT)
    //!          ->Channel("UV1", RHI::Format::R32G32_FLOAT);
    //!      layout = layoutBuilder.End();
    //!    @endcode
    class InputStreamLayoutBuilder
    {
    public:

        //! Used to build the stream channel content of a stream buffer. 
        //! The byte-position of each channel and the total number of stride bytes for the buffer descriptor are
        //! calculated automatically based on the Channels and Padding that are registered. Note that all padding
        //! in the structure must be registered including at the end of the structure, not just between channels, 
        //! in order to calculate the correct stride.
        class BufferDescriptorBuilder
        {
            friend class InputStreamLayoutBuilder;

        public:
            /// Registers the next stream channel in the current buffer
            BufferDescriptorBuilder * Channel(AZStd::string_view semantic, Format format);

            /// Registers the next stream channel in the current buffer
            BufferDescriptorBuilder* Channel(const ShaderSemantic& semantic, Format format);

            /// Registers padding bytes that are unused in the current buffer.
            BufferDescriptorBuilder* Padding(uint32_t byteCount);

        private:
            uint32_t m_bufferIndex = 0;
            uint32_t m_byteOffset = 0;
            AZStd::fixed_vector<StreamChannelDescriptor, RHI::Limits::Pipeline::StreamChannelCountMax> m_channelDescriptors;
            StreamBufferDescriptor m_bufferDescriptor;
        };

        InputStreamLayoutBuilder();

        /// Begin building a new InputStreamLayout. This is optional for the first InputStreamLayout.
        void Begin();

        /// Set the PrimitiveTopology used by the InputStreamLayout. The default is TriangleList.
        void SetTopology(PrimitiveTopology topology);

        /// Add a buffer descriptor to the InputStreamLayout. Use the returned BufferBuilder to register Channels and Padding.
        BufferDescriptorBuilder* AddBuffer(StreamStepFunction stepFunction = StreamStepFunction::PerVertex, uint32_t stepRate = 1);

        /// Finalize and return the InputStreamLayout
        InputStreamLayout End();

    private:
        PrimitiveTopology m_topology = PrimitiveTopology::TriangleList;

        AZStd::fixed_vector<BufferDescriptorBuilder, RHI::Limits::Pipeline::StreamCountMax> m_bufferDescriptorBuilders;
        BufferDescriptorBuilder m_dummyBufferDescriptorBuilder;
    };
}
