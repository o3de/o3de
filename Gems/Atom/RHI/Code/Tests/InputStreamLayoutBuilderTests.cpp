/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <AzCore/Name/NameDictionary.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RHI;

    class InputStreamLayoutBuilderTests
        : public RHITestFixture
    {
    protected:

        void ExpectEq(AZStd::array_view<StreamBufferDescriptor> expected, AZStd::array_view<StreamBufferDescriptor> actual)
        {
            EXPECT_EQ(expected.size(), actual.size());
            for (int i = 0; i < expected.size() && i < actual.size(); ++i)
            {
                EXPECT_EQ(expected[i].m_stepRate, actual[i].m_stepRate);
                EXPECT_EQ(expected[i].m_stepFunction, actual[i].m_stepFunction);
                EXPECT_EQ(expected[i].m_byteStride, actual[i].m_byteStride);
            }
        }

        void ExpectEq(AZStd::array_view<StreamChannelDescriptor> expected, AZStd::array_view<StreamChannelDescriptor> actual)
        {
            EXPECT_EQ(expected.size(), actual.size());
            for (int i = 0; i < expected.size() && i < actual.size(); ++i)
            {
                EXPECT_EQ(expected[i].m_bufferIndex, actual[i].m_bufferIndex);
                EXPECT_EQ(expected[i].m_byteOffset, actual[i].m_byteOffset);
                EXPECT_EQ(expected[i].m_format, actual[i].m_format);
                EXPECT_EQ(expected[i].m_semantic, actual[i].m_semantic);
            }
        }

        void ExpectEq(const InputStreamLayout& expected, const InputStreamLayout& actual)
        {
            EXPECT_EQ(expected.IsFinalized(), actual.IsFinalized());
            EXPECT_EQ(expected.GetTopology(), actual.GetTopology());
            ExpectEq(expected.GetStreamBuffers(), actual.GetStreamBuffers());
            ExpectEq(expected.GetStreamChannels(), actual.GetStreamChannels());
        }
    };

    TEST_F(InputStreamLayoutBuilderTests, TestDefault)
    {
        InputStreamLayout expected;
        expected.SetTopology(PrimitiveTopology::TriangleList);
        expected.Finalize();

        InputStreamLayout actual = InputStreamLayoutBuilder().End();

        ExpectEq(expected, actual);
    }

    TEST_F(InputStreamLayoutBuilderTests, TestInterleavedBuffer)
    {
        InputStreamLayout expected;
        {
            expected.SetTopology(RHI::PrimitiveTopology::TriangleList);

            RHI::StreamChannelDescriptor positionDescriptor;
            positionDescriptor.m_bufferIndex = 0;
            positionDescriptor.m_byteOffset = 0;
            positionDescriptor.m_format = RHI::Format::R32G32_FLOAT;
            positionDescriptor.m_semantic.m_name = Name{ "POSITION" };
            expected.AddStreamChannel(positionDescriptor);

            RHI::StreamChannelDescriptor uvDescriptor;
            uvDescriptor.m_bufferIndex = 0;
            uvDescriptor.m_byteOffset = sizeof(float) * 2;
            uvDescriptor.m_format = RHI::Format::R32G32_FLOAT;
            uvDescriptor.m_semantic.m_name = Name{ "UV" };
            expected.AddStreamChannel(uvDescriptor);

            RHI::StreamChannelDescriptor colorDescriptor;
            colorDescriptor.m_bufferIndex = 0;
            colorDescriptor.m_byteOffset = sizeof(float) * 4;
            colorDescriptor.m_format = RHI::Format::R8G8B8A8_UNORM;
            colorDescriptor.m_semantic.m_name = Name{ "COLOR" };
            expected.AddStreamChannel(colorDescriptor);

            RHI::StreamBufferDescriptor bufferDescriptor;
            bufferDescriptor.m_byteStride = sizeof(float) * 4 + 4;
            expected.AddStreamBuffer(bufferDescriptor);

            expected.Finalize();
        }
        
        InputStreamLayout actual;
        {
            RHI::InputStreamLayoutBuilder layoutBuilder;

            layoutBuilder.AddBuffer()
                ->Channel("POSITION", RHI::Format::R32G32_FLOAT)
                ->Channel("UV", RHI::Format::R32G32_FLOAT)
                ->Channel("COLOR", RHI::Format::R8G8B8A8_UNORM);

            actual = layoutBuilder.End();
        }

        ExpectEq(expected, actual);
    }

    TEST_F(InputStreamLayoutBuilderTests, TestIndependentBuffers)
    {
        InputStreamLayout expected;
        {            
            expected.SetTopology(RHI::PrimitiveTopology::TriangleList);

            RHI::StreamChannelDescriptor positionDescriptor;
            positionDescriptor.m_bufferIndex = 0;
            positionDescriptor.m_byteOffset = 0;
            positionDescriptor.m_format = RHI::Format::R32G32B32_FLOAT;
            positionDescriptor.m_semantic.m_name = Name{ "POSITION" };
            expected.AddStreamChannel(positionDescriptor);

            RHI::StreamChannelDescriptor colorDescriptor;
            colorDescriptor.m_bufferIndex = 1;
            colorDescriptor.m_byteOffset = 0;
            colorDescriptor.m_format = RHI::Format::R32G32B32A32_FLOAT;
            colorDescriptor.m_semantic.m_name = Name{ "COLOR" };
            expected.AddStreamChannel(colorDescriptor);

            RHI::StreamChannelDescriptor uvDescriptor;
            uvDescriptor.m_bufferIndex = 2;
            uvDescriptor.m_byteOffset = 0;
            uvDescriptor.m_format = RHI::Format::R32G32_FLOAT;
            uvDescriptor.m_semantic.m_name = Name{ "UV" };
            expected.AddStreamChannel(uvDescriptor);

            RHI::StreamBufferDescriptor bufferDescriptor;
            bufferDescriptor.m_byteStride = 3 * sizeof(float);
            expected.AddStreamBuffer(bufferDescriptor);

            bufferDescriptor.m_byteStride = 4 * sizeof(float);
            expected.AddStreamBuffer(bufferDescriptor);

            bufferDescriptor.m_byteStride = 2 * sizeof(float);
            expected.AddStreamBuffer(bufferDescriptor);

            expected.Finalize();
        }

        InputStreamLayout actual;
        {  
            RHI::InputStreamLayoutBuilder layoutBuilder;
            layoutBuilder.AddBuffer()->Channel("POSITION", RHI::Format::R32G32B32_FLOAT);
            layoutBuilder.AddBuffer()->Channel("COLOR", RHI::Format::R32G32B32A32_FLOAT);
            layoutBuilder.AddBuffer()->Channel("UV", RHI::Format::R32G32_FLOAT);
            actual = layoutBuilder.End();
        }

        ExpectEq(expected, actual);
    }

    TEST_F(InputStreamLayoutBuilderTests, TestMultipleInterleavedBuffersWithPadding)
    {
        InputStreamLayout expected;
        {
            expected.SetTopology(RHI::PrimitiveTopology::TriangleList);

            // Buffer 0 ...

            RHI::StreamChannelDescriptor positionDescriptor;
            positionDescriptor.m_bufferIndex = 0;
            positionDescriptor.m_byteOffset = 0;
            positionDescriptor.m_format = RHI::Format::R32G32B32_FLOAT;
            positionDescriptor.m_semantic.m_name = Name{ "POSITION" };
            expected.AddStreamChannel(positionDescriptor);

            RHI::StreamChannelDescriptor colorDescriptor;
            colorDescriptor.m_bufferIndex = 0;
            colorDescriptor.m_byteOffset = sizeof(float) * 4; // Includes 4 bytes of padding between channels
            colorDescriptor.m_format = RHI::Format::R32G32B32A32_FLOAT;
            colorDescriptor.m_semantic.m_name = Name{ "COLOR" };
            expected.AddStreamChannel(colorDescriptor);

            RHI::StreamBufferDescriptor bufferDescriptor;
            bufferDescriptor.m_byteStride = 8 * sizeof(float);
            expected.AddStreamBuffer(bufferDescriptor);

            // Buffer 1 ...

            RHI::StreamChannelDescriptor uvDescriptor;
            uvDescriptor.m_bufferIndex = 1;
            uvDescriptor.m_byteOffset = 0;
            uvDescriptor.m_format = RHI::Format::R32G32_FLOAT;
            uvDescriptor.m_semantic = ShaderSemantic{ "UV", 0 };
            expected.AddStreamChannel(uvDescriptor);

            uvDescriptor.m_byteOffset = sizeof(float) * 2;
            uvDescriptor.m_format = RHI::Format::R32G32_FLOAT;
            uvDescriptor.m_semantic = ShaderSemantic{ "UV", 1 };
            expected.AddStreamChannel(uvDescriptor);

            // UV1 is present in the buffer but not used for this shader

            uvDescriptor.m_byteOffset = sizeof(float) * 6;
            uvDescriptor.m_format = RHI::Format::R32G32_FLOAT;
            uvDescriptor.m_semantic = ShaderSemantic{ "UV", 3 };
            expected.AddStreamChannel(uvDescriptor);

            uvDescriptor.m_byteOffset = sizeof(float) * 8;
            uvDescriptor.m_format = RHI::Format::R32G32_FLOAT;
            uvDescriptor.m_semantic = ShaderSemantic{ "UV", 4 };
            expected.AddStreamChannel(uvDescriptor);

            bufferDescriptor.m_byteStride = 10 * sizeof(float);
            expected.AddStreamBuffer(bufferDescriptor);

            expected.Finalize();
        }

        InputStreamLayout actual;
        {
            RHI::InputStreamLayoutBuilder layoutBuilder;

            layoutBuilder.AddBuffer()
                ->Channel("POSITION", RHI::Format::R32G32B32_FLOAT)
                ->Padding(sizeof(float))
                ->Channel("COLOR", RHI::Format::R32G32B32A32_FLOAT);

            layoutBuilder.AddBuffer()
                ->Channel("UV0", RHI::Format::R32G32_FLOAT)
                ->Channel("UV1", RHI::Format::R32G32_FLOAT)
                ->Padding(sizeof(float) * 2)
                ->Channel("UV3", RHI::Format::R32G32_FLOAT)
                ->Channel("UV4", RHI::Format::R32G32_FLOAT);

            actual = layoutBuilder.End();
        }

        ExpectEq(expected, actual);
    }

    TEST_F(InputStreamLayoutBuilderTests, TestTooManyBuffers)
    {
        const uint32_t maxBuffers = RHI::Limits::Pipeline::StreamCountMax;

        // The expected layout will have exactly the max number of buffers, which demonstrates that
        // InputStreamLayoutBuilder attempts to recover from the error.
        InputStreamLayout expected;
        {
            expected.SetTopology(RHI::PrimitiveTopology::TriangleList);

            for (uint32_t i = 0; i < maxBuffers; ++i)
            {
                RHI::StreamChannelDescriptor positionDescriptor;
                positionDescriptor.m_bufferIndex = i;
                positionDescriptor.m_byteOffset = 0;
                positionDescriptor.m_format = RHI::Format::R32G32_FLOAT;
                positionDescriptor.m_semantic = ShaderSemantic{ "UV", i };
                expected.AddStreamChannel(positionDescriptor);

                RHI::StreamBufferDescriptor bufferDescriptor;
                bufferDescriptor.m_byteStride = 2 * sizeof(float);
                expected.AddStreamBuffer(bufferDescriptor);
            }

            expected.Finalize();
        }

        InputStreamLayout actual;
        {
            RHI::InputStreamLayoutBuilder layoutBuilder;

            for (uint32_t i = 0; i < maxBuffers; ++i)
            {
                layoutBuilder.AddBuffer()->Channel(ShaderSemantic{ "UV", i }, RHI::Format::R32G32_FLOAT);
            }

            AZ_TEST_START_ASSERTTEST;
            // Registering a channel on the failed buffer should not crash, is ignored.
            layoutBuilder.AddBuffer()->Channel(ShaderSemantic{ "UV", maxBuffers }, RHI::Format::R32G32_FLOAT);
            AZ_TEST_STOP_ASSERTTEST(1);

            actual = layoutBuilder.End();
        }

        ExpectEq(expected, actual);
    }
}
