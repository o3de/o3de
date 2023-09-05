/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/MultiDeviceBufferPool.h>
#include <Atom/RHI/MultiDeviceIndirectBufferSignature.h>
#include <Atom/RHI/MultiDeviceIndirectBufferWriter.h>
#include <Atom/RHI/MultiDeviceStreamBufferView.h>
#include <Atom/RHI/MultiDeviceIndexBufferView.h>
#include <Tests/Buffer.h>
#include <Tests/Device.h>
#include <Tests/IndirectBuffer.h>

#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

namespace UnitTest
{
    using namespace AZ;

    class MultiDeviceIndirectBufferTests : public MultiDeviceRHITestFixture
    {
    public:
        MultiDeviceIndirectBufferTests()
            : MultiDeviceRHITestFixture()
        {
        }

        ~MultiDeviceIndirectBufferTests()
        {
        }

    private:
        void SetUp() override
        {
            MultiDeviceRHITestFixture::SetUp();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            RHI::ReflectSystemComponent::Reflect(m_serializeContext.get());
            AZ::Name::Reflect(m_serializeContext.get());

            m_commands.clear();
            m_commands.push_back(RHI::IndirectCommandType::RootConstants);
            m_commands.push_back(RHI::IndirectBufferViewArguments{ s_vertexSlotIndex });
            m_commands.push_back(RHI::IndirectCommandType::IndexBufferView);
            m_commands.push_back(RHI::IndirectCommandType::DrawIndexed);

            m_bufferPool = aznew AZ::RHI::MultiDeviceBufferPool;
            RHI::BufferPoolDescriptor poolDesc;
            poolDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
            m_bufferPool->Init(DeviceMask, poolDesc);

            m_buffer = aznew AZ::RHI::MultiDeviceBuffer;
            RHI::MultiDeviceBufferInitRequest initRequest;
            initRequest.m_buffer = m_buffer.get();
            initRequest.m_descriptor.m_byteCount = m_writerCommandStride * m_writerNumCommands;
            initRequest.m_descriptor.m_bindFlags = poolDesc.m_bindFlags;
            m_bufferPool->InitBuffer(initRequest);

            AZ_TEST_START_TRACE_SUPPRESSION;
            m_writerSignature = CreateInitializedSignature();
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_CALL(
                    *static_cast<IndirectBufferSignature*>(m_writerSignature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                    GetByteStrideInternal())
                    .WillRepeatedly(testing::Return(m_writerCommandStride));
            }
        }

        void TearDown() override
        {
            m_buffer.reset();
            m_bufferPool.reset();
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_CALL(
                    *static_cast<IndirectBufferSignature*>(m_writerSignature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                    ShutdownInternal())
                    .Times(1);
            }

            m_writerSignature.reset();

            m_serializeContext.reset();
            MultiDeviceRHITestFixture::TearDown();
        }

    protected:
        RHI::IndirectBufferLayout CreateUnfinalizedLayout()
        {
            RHI::IndirectBufferLayout layout;
            for (const auto& descriptor : m_commands)
            {
                EXPECT_TRUE(layout.AddIndirectCommand(descriptor));
            }
            return layout;
        }

        RHI::IndirectBufferLayout CreateFinalizedLayout()
        {
            auto layout = CreateUnfinalizedLayout();
            EXPECT_TRUE(layout.Finalize());
            return layout;
        }

        RHI::IndirectBufferLayout CreateSerializedLayout(const RHI::IndirectBufferLayout& layout)
        {
            AZStd::vector<char, AZ::OSStdAllocator> buffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char, AZ::OSStdAllocator>> outStream(&buffer);

            {
                AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&outStream, *m_serializeContext.get(), AZ::ObjectStream::ST_BINARY);

                bool writeOK = objStream->WriteClass(&layout);
                EXPECT_TRUE(writeOK);

                bool finalizeOK = objStream->Finalize();
                EXPECT_TRUE(finalizeOK);
            }

            outStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

            AZ::ObjectStream::FilterDescriptor filterDesc;
            RHI::IndirectBufferLayout deserializedLayout;
            bool deserializedOK = AZ::Utils::LoadObjectFromStreamInPlace<RHI::IndirectBufferLayout>(
                outStream, deserializedLayout, m_serializeContext.get(), filterDesc);
            EXPECT_TRUE(deserializedOK);
            return deserializedLayout;
        }

        void ValidateLayout(const RHI::IndirectBufferLayout& layout)
        {
            EXPECT_TRUE(layout.IsFinalized());
            auto layoutCommands = layout.GetCommands();
            EXPECT_EQ(m_commands.size(), layoutCommands.size());
            for (uint32_t i = 0; i < m_commands.size(); ++i)
            {
                EXPECT_EQ(m_commands[i], layoutCommands[i]);
                EXPECT_EQ(layout.FindCommandIndex(m_commands[i]), RHI::IndirectCommandIndex(i));
            }
        }

        RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferSignature> CreateInitializedSignature()
        {
            using namespace ::testing;
            auto signature = aznew AZ::RHI::MultiDeviceIndirectBufferSignature;
            m_signatureDescriptor.m_layout = CreateFinalizedLayout();
            EXPECT_EQ(signature->Init(DeviceMask, m_signatureDescriptor), RHI::ResultCode::Success);

            return signature;
        }

        RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferSignature> CreateUnInitializedSignature()
        {
            auto signature = aznew AZ::RHI::MultiDeviceIndirectBufferSignature;
            return signature;
        }

        RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> CreateInitializedWriter()
        {
            auto writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            EXPECT_EQ(
                writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride, m_writerNumCommands, *m_writerSignature),
                RHI::ResultCode::Success);
            return writer;
        }

        void ValidateSignature(const RHI::MultiDeviceIndirectBufferSignature& signature)
        {
            ValidateLayout(signature.GetLayout());
            EXPECT_TRUE(signature.IsInitialized());
        }

        void ValidateWriter(const AZ::RHI::MultiDeviceIndirectBufferWriter& writer)
        {
            auto currentSequenceIndices{ writer.GetCurrentSequenceIndex() };
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_EQ(
                    static_cast<IndirectBufferWriter*>(writer.GetDeviceIndirectBufferWriter(deviceIndex).get())->GetData(),
                    static_cast<const uint8_t*>(static_cast<Buffer*>(m_buffer->GetDeviceBuffer(deviceIndex).get())->GetData().data()));

                EXPECT_EQ(currentSequenceIndices[deviceIndex], 0);
                EXPECT_TRUE(static_cast<Buffer*>(m_buffer->GetDeviceBuffer(deviceIndex).get())->IsMapped());
            }
        }

        static const uint32_t s_vertexSlotIndex = 3;
        AZStd::vector<RHI::IndirectCommandDescriptor> m_commands;

        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        RHI::MultiDeviceIndirectBufferSignatureDescriptor m_signatureDescriptor;

        RHI::Ptr<AZ::RHI::MultiDeviceBufferPool> m_bufferPool;
        RHI::Ptr<AZ::RHI::MultiDeviceBuffer> m_buffer;

        size_t m_writerOffset = 0;
        uint32_t m_writerCommandStride = 2;
        uint32_t m_writerNumCommands = 1024;

        RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferSignature> m_writerSignature;
    };

    TEST_F(MultiDeviceIndirectBufferTests, TestSignature)
    {
        // Normal initialization
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto signature = CreateInitializedSignature();
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
            EXPECT_TRUE(signature != nullptr);
            ValidateSignature(*signature);
        }

        // ! Cannot tests this as we do not have access to device signatures here and cannot setup mock-call
        // // Failure initializing.
        // {
        //     auto signature = CreateUnInitializedSignature();
        //     RHI::MultiDeviceIndirectBufferSignatureDescriptor descriptor;
        //     EXPECT_TRUE(signature->Init(DeviceMask, descriptor) == RHI::ResultCode::InvalidOperation);
        //     EXPECT_FALSE(signature->IsInitialized());
        // }

        // GetByteStride()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto signature = CreateInitializedSignature();
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
            uint32_t byteStride = 1337;
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_CALL(
                    *static_cast<IndirectBufferSignature*>(signature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                    GetByteStrideInternal())
                    .Times(1)
                    .WillOnce(testing::Return(byteStride));
            }
            EXPECT_EQ(signature->GetByteStride(), byteStride);
        }

        // GetByteStride() on uninitialized signature.
        {
            auto signature = CreateUnInitializedSignature();
            // ! Do not have access to members, cannot setup mock-call
            // EXPECT_CALL(*signature, GetByteStrideInternal()).Times(1).WillOnce(testing::Return(0));
            AZ_TEST_START_TRACE_SUPPRESSION;
            signature->GetByteStride();
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // GetOffset()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto signature = CreateInitializedSignature();
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
            uint32_t offset = 1337;
            RHI::IndirectCommandIndex index(m_commands.size() - 1);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_CALL(
                    *static_cast<IndirectBufferSignature*>(signature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                    GetOffsetInternal(index))
                    .Times(1)
                    .WillOnce(testing::Return(offset));
            }
            EXPECT_EQ(signature->GetOffset(index), offset);
        }

        // GetOffset with null index
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto signature = CreateInitializedSignature();
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
            RHI::IndirectCommandIndex index = RHI::IndirectCommandIndex::Null;
            AZ_TEST_START_TRACE_SUPPRESSION;
            signature->GetOffset(index);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // GetOffset with invalid index
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto signature = CreateInitializedSignature();
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
            RHI::IndirectCommandIndex index(m_commands.size());
            AZ_TEST_START_TRACE_SUPPRESSION;
            signature->GetOffset(index);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Shutdown
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto signature = CreateInitializedSignature();
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_CALL(
                    *static_cast<IndirectBufferSignature*>(signature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                    ShutdownInternal())
                    .Times(1);
            }
        }
    }

    TEST_F(MultiDeviceIndirectBufferTests, TestWriter)
    {
        // Normal Initialization
        {
            auto writer = CreateInitializedWriter();
            EXPECT_TRUE(writer != nullptr);
            ValidateWriter(*writer);
        }

        // Initialization with invalid size
        {
            RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(
                writer->Init(*m_buffer, 1, m_writerCommandStride, m_writerNumCommands, *m_writerSignature),
                RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with invalid stride
        {
            RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(
                writer->Init(*m_buffer, m_writerOffset, 0, m_writerNumCommands, *m_writerSignature), RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with invalid max num sequences
        {
            RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(
                writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride, 0, *m_writerSignature), RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with small invalid stride
        {
            RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(
                writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride - 1, m_writerNumCommands, *m_writerSignature),
                RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with invalid signature
        {
            RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            auto signature = CreateUnInitializedSignature();
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(
                writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride, m_writerNumCommands, *signature),
                RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with offset
        {
            RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            size_t offset = 16;
            EXPECT_EQ(writer->Init(*m_buffer, offset, m_writerCommandStride, 5, *m_writerSignature), RHI::ResultCode::Success);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_EQ(
                    static_cast<IndirectBufferWriter*>(writer->GetDeviceIndirectBufferWriter(deviceIndex).get())->GetData(),
                    static_cast<Buffer*>(m_buffer->GetDeviceBuffer(deviceIndex).get())->GetData().data() + offset);
            }
        }

        // Initialization with memory pointer
        {
            RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            EXPECT_EQ(
                writer->Init(
                    const_cast<uint8_t*>(
                        static_cast<Buffer*>(m_buffer->GetDeviceBuffer(AZ::RHI::MultiDevice::DefaultDeviceIndex).get())->GetData().data()),
                    m_writerCommandStride,
                    m_writerNumCommands,
                    *m_writerSignature),
                RHI::ResultCode::Success);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_EQ(
                    static_cast<IndirectBufferWriter*>(writer->GetDeviceIndirectBufferWriter(deviceIndex).get())->GetData(),
                    static_cast<Buffer*>(m_buffer->GetDeviceBuffer(AZ::RHI::MultiDevice::DefaultDeviceIndex).get())->GetData().data());
            }
        }

        // Double Init
        {
            auto writer = CreateInitializedWriter();
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(
                writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride, m_writerNumCommands, *m_writerSignature),
                RHI::ResultCode::InvalidOperation);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Valid Seek
        {
            auto writer = CreateInitializedWriter();
            uint32_t seekPos = 2;
            EXPECT_TRUE(writer->Seek(seekPos));
            {
                auto currentSequenceIndex{ writer->GetCurrentSequenceIndex() };
                for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                {
                    EXPECT_EQ(currentSequenceIndex[deviceIndex], seekPos);
                }
            }

            seekPos += 6;
            EXPECT_TRUE(writer->Seek(seekPos));
            {
                auto currentSequenceIndex{ writer->GetCurrentSequenceIndex() };
                for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                {
                    EXPECT_EQ(currentSequenceIndex[deviceIndex], seekPos);
                }
            }
        }

        // Invalid Seek
        {
            auto writer = CreateInitializedWriter();
            EXPECT_FALSE(writer->Seek(m_writerNumCommands + 1));
            {
                auto currentSequenceIndex{ writer->GetCurrentSequenceIndex() };
                for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                {
                    EXPECT_EQ(currentSequenceIndex[deviceIndex], 0);
                }
            }
        }

        // Valid NextSequence
        {
            auto writer = CreateInitializedWriter();
            EXPECT_TRUE(writer->NextSequence());
            {
                auto currentSequenceIndex{ writer->GetCurrentSequenceIndex() };
                for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                {
                    EXPECT_EQ(currentSequenceIndex[deviceIndex], 1);
                }
            }
        }

        // Invalid NextSequence
        {
            auto writer = CreateInitializedWriter();
            EXPECT_TRUE(writer->Seek(m_writerNumCommands - 1));
            EXPECT_FALSE(writer->NextSequence());
            {
                auto currentSequenceIndex{ writer->GetCurrentSequenceIndex() };
                for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                {
                    EXPECT_EQ(currentSequenceIndex[deviceIndex], m_writerNumCommands - 1);
                }
            }
        }

        // Valid Command
        {
            auto writer = CreateInitializedWriter();
            for (const auto& command : m_commands)
            {
                switch (command.m_type)
                {
                case RHI::IndirectCommandType::VertexBufferView:
                    {
                        auto index = m_signatureDescriptor.m_layout.FindCommandIndex(RHI::IndirectBufferViewArguments{ s_vertexSlotIndex });
                        EXPECT_FALSE(index.IsNull());
                        AZ::RHI::MultiDeviceStreamBufferView bufferView(*m_buffer, 0, 12, 10);
                        for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                            EXPECT_CALL(
                                *static_cast<IndirectBufferWriter*>(writer->GetDeviceIndirectBufferWriter(deviceIndex).get()),
                                SetVertexViewInternal(index, testing::_))
                                .Times(1);
                        writer->SetVertexView(s_vertexSlotIndex, bufferView);
                        break;
                    }
                case RHI::IndirectCommandType::IndexBufferView:
                    {
                        auto index = m_signatureDescriptor.m_layout.FindCommandIndex(command.m_type);
                        EXPECT_FALSE(index.IsNull());
                        AZ::RHI::MultiDeviceIndexBufferView indexView(*m_buffer, 0, 12, RHI::IndexFormat::Uint16);
                        for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                            EXPECT_CALL(
                                *static_cast<IndirectBufferWriter*>(writer->GetDeviceIndirectBufferWriter(deviceIndex).get()),
                                SetIndexViewInternal(index, testing::_))
                                .Times(1);
                        writer->SetIndexView(indexView);
                        break;
                    }
                case RHI::IndirectCommandType::DrawIndexed:
                    {
                        auto index = m_signatureDescriptor.m_layout.FindCommandIndex(command.m_type);
                        EXPECT_FALSE(index.IsNull());
                        AZ::RHI::DrawIndexed arguments(1, 2, 3, 4, 5);
                        for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                            EXPECT_CALL(
                                *static_cast<IndirectBufferWriter*>(writer->GetDeviceIndirectBufferWriter(deviceIndex).get()),
                                DrawIndexedInternal(index, testing::_))
                                .Times(1);
                        writer->DrawIndexed(arguments);
                        break;
                    }
                case RHI::IndirectCommandType::RootConstants:
                    {
                        auto index = m_signatureDescriptor.m_layout.FindCommandIndex(command.m_type);
                        EXPECT_FALSE(index.IsNull());
                        size_t rootConstant;
                        for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                            EXPECT_CALL(
                                *static_cast<IndirectBufferSignature*>(
                                    m_writerSignature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                                GetOffsetInternal(index))
                                .Times(1)
                                .WillOnce(testing::Return(0));

                        auto nextIndex = RHI::IndirectCommandIndex(index.GetIndex() + 1);
                        for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                            EXPECT_CALL(
                                *static_cast<IndirectBufferSignature*>(
                                    m_writerSignature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                                GetOffsetInternal(nextIndex))
                                .Times(1)
                                .WillOnce(testing::Return(static_cast<uint32_t>(sizeof(rootConstant))));

                        for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                            EXPECT_CALL(
                                *static_cast<IndirectBufferWriter*>(writer->GetDeviceIndirectBufferWriter(deviceIndex).get()),
                                SetRootConstantsInternal(index, reinterpret_cast<uint8_t*>(&rootConstant), sizeof(rootConstant)))
                                .Times(1);
                        writer->SetRootConstants(reinterpret_cast<uint8_t*>(&rootConstant), sizeof(rootConstant));
                        break;
                    }
                default:
                    break;
                }
            }
        }

        // Invalid command
        {
            auto writer = CreateInitializedWriter();
            RHI::DispatchDirect args;
            AZ_TEST_START_TRACE_SUPPRESSION;
            writer->Dispatch(args);
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
        }

        // Write command on uninitialized writer
        {
            RHI::Ptr<AZ::RHI::MultiDeviceIndirectBufferWriter> writer = aznew AZ::RHI::MultiDeviceIndirectBufferWriter;
            ;
            AZ::RHI::DrawIndexed arguments(1, 2, 3, 4, 5);
            AZ_TEST_START_TRACE_SUPPRESSION;
            writer->DrawIndexed(arguments);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Flush
        {
            auto writer = CreateInitializedWriter();
            writer->Flush();
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                EXPECT_FALSE(static_cast<Buffer*>(m_buffer->GetDeviceBuffer(deviceIndex).get())->IsMapped());
            AZ::RHI::MultiDeviceIndexBufferView indexView(*m_buffer, 0, 12, RHI::IndexFormat::Uint16);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                EXPECT_CALL(
                    *static_cast<IndirectBufferWriter*>(writer->GetDeviceIndirectBufferWriter(deviceIndex).get()),
                    SetIndexViewInternal(testing::_, testing::_))
                    .Times(1);
            writer->SetIndexView(indexView);
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                EXPECT_TRUE(static_cast<Buffer*>(m_buffer->GetDeviceBuffer(deviceIndex).get())->IsMapped());
        }

        // Inline Constants Command with incorrect size
        {
            auto writer = CreateInitializedWriter();
            auto findIt = AZStd::find_if(
                m_commands.begin(),
                m_commands.end(),
                [](const auto& element)
                {
                    return element.m_type == RHI::IndirectCommandType::RootConstants;
                });
            EXPECT_NE(findIt, m_commands.end());
            auto commandIndex = m_writerSignature->GetLayout().FindCommandIndex(*findIt);
            EXPECT_FALSE(commandIndex.IsNull());
            auto nextCommandIndex = RHI::IndirectCommandIndex(commandIndex.GetIndex() + 1);
            uint32_t commandOffsett = 12;
            uint32_t nextCommandOffset = 16;

            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                EXPECT_CALL(
                    *static_cast<IndirectBufferSignature*>(m_writerSignature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                    GetOffsetInternal(commandIndex))
                    .Times(1)
                    .WillOnce(testing::Return(commandOffsett));

                EXPECT_CALL(
                    *static_cast<IndirectBufferSignature*>(m_writerSignature->GetDeviceIndirectBufferSignature(deviceIndex).get()),
                    GetOffsetInternal(nextCommandIndex))
                    .Times(1)
                    .WillOnce(testing::Return(nextCommandOffset));
            }

            AZ_TEST_START_TRACE_SUPPRESSION;
            uint64_t data;
            writer->SetRootConstants(reinterpret_cast<uint8_t*>(&data), sizeof(data));
            AZ_TEST_STOP_TRACE_SUPPRESSION(DeviceCount);
        }

        // Shutdown
        {
            auto writer = CreateInitializedWriter();
            writer->Shutdown();
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                EXPECT_FALSE(static_cast<Buffer*>(m_buffer->GetDeviceBuffer(deviceIndex).get())->IsMapped());
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
                EXPECT_TRUE(
                    static_cast<IndirectBufferWriter*>(writer->GetDeviceIndirectBufferWriter(deviceIndex).get())->GetData() == nullptr);
        }
    }
} // namespace UnitTest
