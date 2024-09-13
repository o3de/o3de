/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Tests/Factory.h>
#include <Tests/Device.h>
#include <Tests/Buffer.h>
#include <Tests/IndirectBuffer.h>

#include <Atom/RHI/FrameEventBus.h>

#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

namespace UnitTest
{
    using namespace AZ;

    class IndirectBufferTests
        : public RHITestFixture
    {
    public:
        IndirectBufferTests()
            : RHITestFixture()
        {
        }

        ~IndirectBufferTests()
        {
        }

    private:
        void SetUp() override
        {
            RHITestFixture::SetUp();
            m_factory.reset(aznew Factory());
            m_device = MakeTestDevice();
            m_serializeContext = AZStd::make_unique<SerializeContext>();
            RHI::ReflectSystemComponent::Reflect(m_serializeContext.get());
            AZ::Name::Reflect(m_serializeContext.get());

            m_commands.clear();
            m_commands.push_back(RHI::IndirectCommandType::RootConstants);
            m_commands.push_back(RHI::IndirectBufferViewArguments{ s_vertexSlotIndex });
            m_commands.push_back(RHI::IndirectCommandType::IndexBufferView);
            m_commands.push_back(RHI::IndirectCommandType::DrawIndexed);

            m_bufferPool = static_cast<BufferPool*>(RHI::Factory::Get().CreateBufferPool().get());
            RHI::BufferPoolDescriptor poolDesc;
            poolDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
            m_bufferPool->Init(*m_device, poolDesc);

            m_buffer = static_cast<Buffer*>(RHI::Factory::Get().CreateBuffer().get());
            RHI::DeviceBufferInitRequest initRequest;
            initRequest.m_buffer = m_buffer.get();
            initRequest.m_descriptor.m_byteCount = m_writerCommandStride * m_writerNumCommands;
            initRequest.m_descriptor.m_bindFlags = poolDesc.m_bindFlags;
            m_bufferPool->InitBuffer(initRequest);

            m_writerSignature = CreateInitializedSignature();
            EXPECT_CALL(*m_writerSignature, GetByteStrideInternal())
                .WillRepeatedly(
                    testing::Return(m_writerCommandStride));
        }

        void TearDown() override
        {
            m_buffer.reset();
            m_bufferPool.reset();
            m_writerSignature.reset();
            m_factory.reset();
            m_device.reset();
            m_serializeContext.reset();
            RHITestFixture::TearDown();
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
            AZ::IO::ByteContainerStream<AZStd::vector<char, AZ::OSStdAllocator> > outStream(&buffer);

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
            bool deserializedOK = AZ::Utils::LoadObjectFromStreamInPlace<RHI::IndirectBufferLayout>(outStream, deserializedLayout, m_serializeContext.get(), filterDesc);
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

        RHI::Ptr<IndirectBufferSignature> CreateInitializedSignature()
        {
            using namespace ::testing;
            auto signature = aznew NiceIndirectBufferSignature;
            m_signatureDescriptor.m_layout = CreateFinalizedLayout();
            EXPECT_CALL(*signature, InitInternal(_, _))
                .Times(1)
                .WillOnce(
                    Return(AZ::RHI::ResultCode::Success));
            EXPECT_EQ(signature->Init(*m_device, m_signatureDescriptor), RHI::ResultCode::Success);            
            return signature;
        }

        RHI::Ptr<IndirectBufferSignature> CreateUnInitializedSignature()
        {
            auto signature = aznew NiceIndirectBufferSignature;
            return signature;
        }

        RHI::Ptr<IndirectBufferWriter> CreateInitializedWriter()
        {
            auto writer = aznew NiceIndirectBufferWriter;
            EXPECT_EQ(writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride, m_writerNumCommands, *m_writerSignature), RHI::ResultCode::Success);
            return writer;
        }

        void ValidateSignature(const RHI::DeviceIndirectBufferSignature& signature)
        {
            ValidateLayout(signature.GetLayout());
            EXPECT_TRUE(signature.IsInitialized());
        }

        void ValidateWriter(const IndirectBufferWriter& writer)
        {
            EXPECT_EQ(writer.GetData(), static_cast<const uint8_t*>(m_buffer->GetData().data()));
            EXPECT_EQ(writer.GetCurrentSequenceIndex(), 0);
            EXPECT_TRUE(m_buffer->IsMapped());
        }

        AZStd::unique_ptr<Factory> m_factory;
        RHI::Ptr<RHI::Device> m_device;

        static const uint32_t s_vertexSlotIndex = 3;
        AZStd::vector<RHI::IndirectCommandDescriptor> m_commands;

        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        RHI::DeviceIndirectBufferSignatureDescriptor m_signatureDescriptor;

        RHI::Ptr<BufferPool> m_bufferPool;
        RHI::Ptr<Buffer> m_buffer;

        size_t m_writerOffset = 0;
        uint32_t m_writerCommandStride = 2;
        uint32_t m_writerNumCommands = 1024;

        RHI::Ptr<IndirectBufferSignature> m_writerSignature;
    };

    TEST_F(IndirectBufferTests, TestLayout)
    {
        // Normal layout initialization
        {
            auto layout = CreateFinalizedLayout();
            ValidateLayout(layout);
        }

        // Double finalize.
        {
            auto layout = CreateFinalizedLayout();
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(layout.Finalize());
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Add a command to a finalized layout
        {
            auto layout = CreateFinalizedLayout();
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(layout.AddIndirectCommand(RHI::IndirectBufferViewArguments{ 1337 }));
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Get list of commands of non finalized layout
        {
            auto layout = CreateUnfinalizedLayout();
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(layout.GetCommands().size(), 0);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Same hash
        {
            auto layout1 = CreateFinalizedLayout();
            auto layout2 = CreateFinalizedLayout();
            EXPECT_EQ(layout1.GetHash(), layout2.GetHash());
        }

        // Different hash
        {
            auto layout1 = CreateUnfinalizedLayout();
            auto layout2 = layout1;
            EXPECT_TRUE(layout2.AddIndirectCommand(RHI::IndirectBufferViewArguments{ 1337 }));
            EXPECT_TRUE(layout1.Finalize());
            EXPECT_TRUE(layout2.Finalize());
            EXPECT_NE(layout1.GetHash(), layout2.GetHash());
        }

        // Duplicate command
        {
            auto layout = CreateUnfinalizedLayout();
            AZ_TEST_START_TRACE_SUPPRESSION;
            for (const auto& desc : m_commands)
            {
                EXPECT_FALSE(layout.AddIndirectCommand(desc));
            }            
            AZ_TEST_STOP_TRACE_SUPPRESSION(m_commands.size());
        }

        // Duplicate main command (only one draw, drawIndexed or dispatch is allowed)
        {
            auto layout = CreateUnfinalizedLayout();
            EXPECT_TRUE(layout.AddIndirectCommand(RHI::IndirectCommandType::Dispatch));
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_FALSE(layout.Finalize());
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Find invalid command
        {
            auto layout = CreateFinalizedLayout();
            auto index = layout.FindCommandIndex(RHI::IndirectCommandType::Draw);
            EXPECT_TRUE(index.IsNull());
        }

        // Test serialization
        {
            auto layout = CreateFinalizedLayout();
            auto serializedLayout = CreateSerializedLayout(layout);
            ValidateLayout(serializedLayout);
            EXPECT_EQ(layout.GetHash(), serializedLayout.GetHash());
        }

        // Layout type
        {
            auto layout = CreateFinalizedLayout();
            EXPECT_EQ(layout.GetType(), RHI::IndirectBufferLayoutType::IndexedDraw);
        }
    }

    TEST_F(IndirectBufferTests, TestSignature)
    {        
        // Normal initialization
        {
            auto signature = CreateInitializedSignature();
            EXPECT_TRUE(signature != nullptr);
            ValidateSignature(*signature);
        }

        // Failure initializing.
        {
            auto signature = CreateUnInitializedSignature();
            EXPECT_CALL(*signature, InitInternal(testing::_, testing::_))
                .Times(1)
                .WillOnce(
                    testing::Return(RHI::ResultCode::InvalidOperation));
            RHI::DeviceIndirectBufferSignatureDescriptor descriptor;
            EXPECT_TRUE(signature->Init(*m_device, descriptor) == RHI::ResultCode::InvalidOperation);
            EXPECT_FALSE(signature->IsInitialized());
        }

        // GetByteStride()
        {
            auto signature = CreateInitializedSignature();
            uint32_t byteStride = 1337;
            EXPECT_CALL(*signature, GetByteStrideInternal())
                .Times(1)
                .WillOnce(
                    testing::Return(byteStride));
            EXPECT_EQ(signature->GetByteStride(), byteStride);
        }

        // GetByteStride() on uninitialized signature.
        {
            auto signature = CreateUnInitializedSignature();
            EXPECT_CALL(*signature, GetByteStrideInternal())
                .Times(1)
                .WillOnce(
                    testing::Return(0));
            AZ_TEST_START_TRACE_SUPPRESSION;
            signature->GetByteStride();
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // GetOffset()
        {
            auto signature = CreateInitializedSignature();
            uint32_t offset = 1337;
            RHI::IndirectCommandIndex index(m_commands.size() - 1);
            EXPECT_CALL(*signature, GetOffsetInternal(index))
                .Times(1)
                .WillOnce(
                    testing::Return(offset));
            EXPECT_EQ(signature->GetOffset(index), offset);
        }

        // GetOffset with null index
        {
            auto signature = CreateInitializedSignature();
            RHI::IndirectCommandIndex index =  RHI::IndirectCommandIndex::Null;
            AZ_TEST_START_TRACE_SUPPRESSION;
            signature->GetOffset(index);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // GetOffset with invalid index
        {
            auto signature = CreateInitializedSignature();
            RHI::IndirectCommandIndex index(m_commands.size());
            AZ_TEST_START_TRACE_SUPPRESSION;
            signature->GetOffset(index);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Shutdown
        {
            auto signature = CreateInitializedSignature();
            EXPECT_CALL(*signature, ShutdownInternal())
                .Times(1);
        }
    }

    TEST_F(IndirectBufferTests, TestWriter)
    {
        // Normal Initialization
        {
            auto writer = CreateInitializedWriter();
            EXPECT_TRUE(writer != nullptr);
            ValidateWriter(*writer);
        }

        // Initialization with invalid size
        {
            RHI::Ptr<IndirectBufferWriter> writer = aznew NiceIndirectBufferWriter;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(writer->Init(*m_buffer, 1, m_writerCommandStride, m_writerNumCommands, *m_writerSignature), RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with invalid stride
        {
            RHI::Ptr<IndirectBufferWriter> writer = aznew NiceIndirectBufferWriter;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(writer->Init(*m_buffer, m_writerOffset, 0, m_writerNumCommands, *m_writerSignature), RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with invalid max num sequences
        {
            RHI::Ptr<IndirectBufferWriter> writer = aznew NiceIndirectBufferWriter;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride, 0, *m_writerSignature), RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with small invalid stride
        {
            RHI::Ptr<IndirectBufferWriter> writer = aznew NiceIndirectBufferWriter;
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride - 1, m_writerNumCommands, *m_writerSignature), RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with invalid signature
        {
            RHI::Ptr<IndirectBufferWriter> writer = aznew NiceIndirectBufferWriter;
            auto signature = CreateUnInitializedSignature();
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride, m_writerNumCommands, *signature), RHI::ResultCode::InvalidArgument);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Initialization with offset
        {
            RHI::Ptr<IndirectBufferWriter> writer = aznew NiceIndirectBufferWriter;
            size_t offset = 16;
            EXPECT_EQ(writer->Init(*m_buffer, offset, m_writerCommandStride, 5, *m_writerSignature), RHI::ResultCode::Success);
            EXPECT_EQ(writer->GetData(), m_buffer->GetData().data() + offset);
        }

        // Initialization with memory pointer
        {
            RHI::Ptr<IndirectBufferWriter> writer = aznew NiceIndirectBufferWriter;
            EXPECT_EQ(writer->Init(const_cast<uint8_t*>(m_buffer->GetData().data()), m_writerCommandStride, m_writerNumCommands, *m_writerSignature), RHI::ResultCode::Success);
            EXPECT_EQ(writer->GetData(), m_buffer->GetData().data());
        }

        // Double Init
        {
            auto writer = CreateInitializedWriter();
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(writer->Init(*m_buffer, m_writerOffset, m_writerCommandStride, m_writerNumCommands, *m_writerSignature), RHI::ResultCode::InvalidOperation);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Valid Seek
        {
            auto writer = CreateInitializedWriter();
            uint32_t seekPos = 2;
            EXPECT_TRUE(writer->Seek(seekPos));
            EXPECT_EQ(writer->GetCurrentSequenceIndex(), seekPos);
            seekPos += 6;
            EXPECT_TRUE(writer->Seek(seekPos));
            EXPECT_EQ(writer->GetCurrentSequenceIndex(), seekPos);
        }

        // Invalid Seek
        {
            auto writer = CreateInitializedWriter();
            EXPECT_FALSE(writer->Seek(m_writerNumCommands + 1));
            EXPECT_EQ(writer->GetCurrentSequenceIndex(), 0);
        }

        // Valid NextSequence
        {
            auto writer = CreateInitializedWriter();
            EXPECT_TRUE(writer->NextSequence());
            EXPECT_EQ(writer->GetCurrentSequenceIndex(), 1);
        }

        // Invalid NextSequence
        {
            auto writer = CreateInitializedWriter();
            EXPECT_TRUE(writer->Seek(m_writerNumCommands - 1));
            EXPECT_FALSE(writer->NextSequence());
            EXPECT_EQ(writer->GetCurrentSequenceIndex(), m_writerNumCommands - 1);
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
                    AZ::RHI::DeviceStreamBufferView bufferView(*m_buffer, 0, 12, 10);
                    EXPECT_CALL(*writer, SetVertexViewInternal(index, testing::_)).Times(1);
                    writer->SetVertexView(s_vertexSlotIndex, bufferView);
                    break;
                }
                case RHI::IndirectCommandType::IndexBufferView:
                {
                    auto index = m_signatureDescriptor.m_layout.FindCommandIndex(command.m_type);
                    EXPECT_FALSE(index.IsNull());
                    AZ::RHI::DeviceIndexBufferView indexView(*m_buffer, 0, 12, RHI::IndexFormat::Uint16);
                    EXPECT_CALL(*writer, SetIndexViewInternal(index, testing::_)).Times(1);
                    writer->SetIndexView(indexView);
                    break;
                }
                case RHI::IndirectCommandType::DrawIndexed:
                {
                    auto index = m_signatureDescriptor.m_layout.FindCommandIndex(command.m_type);
                    EXPECT_FALSE(index.IsNull());
                    AZ::RHI::DrawInstanceArguments drawInstanceArgs(1, 2);
                    AZ::RHI::DrawIndexed arguments(3, 4, 5);
                    EXPECT_CALL(*writer, DrawIndexedInternal(index, testing::_, testing::_)).Times(1);
                    writer->DrawIndexed(arguments, drawInstanceArgs);
                    break;
                }
                case RHI::IndirectCommandType::RootConstants:
                {
                    auto index = m_signatureDescriptor.m_layout.FindCommandIndex(command.m_type);
                    EXPECT_FALSE(index.IsNull());
                    size_t rootConstant;
                    EXPECT_CALL(*m_writerSignature, GetOffsetInternal(index)).
                        Times(1)
                        .WillOnce(testing::Return(0));
                    auto nextIndex = RHI::IndirectCommandIndex(index.GetIndex() + 1);
                    EXPECT_CALL(*m_writerSignature, GetOffsetInternal(nextIndex)).
                        Times(1)
                        .WillOnce(testing::Return(static_cast<uint32_t>(sizeof(rootConstant))));
                    EXPECT_CALL(*writer, SetRootConstantsInternal(index, reinterpret_cast<uint8_t*>(&rootConstant), sizeof(rootConstant))).Times(1);
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
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Write command on uninitialized writer
        {
            RHI::Ptr<IndirectBufferWriter> writer = aznew NiceIndirectBufferWriter;
            AZ::RHI::DrawInstanceArguments drawInstanceArgs(1, 2);
            AZ::RHI::DrawIndexed arguments(3, 4, 5);
            EXPECT_CALL(*writer, DrawIndexedInternal(testing::_, testing::_, testing::_)).Times(0);
            AZ_TEST_START_TRACE_SUPPRESSION;
            writer->DrawIndexed(arguments, drawInstanceArgs);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Flush
        {
            auto writer = CreateInitializedWriter();
            writer->Flush();
            EXPECT_FALSE(m_buffer->IsMapped());
            AZ::RHI::DeviceIndexBufferView indexView(*m_buffer, 0, 12, RHI::IndexFormat::Uint16);
            EXPECT_CALL(*writer, SetIndexViewInternal(testing::_, testing::_)).Times(1);
            writer->SetIndexView(indexView);
            EXPECT_TRUE(m_buffer->IsMapped());
        }

        // Inline Constants Command with incorrect size
        {
            auto writer = CreateInitializedWriter();
            auto findIt = AZStd::find_if(m_commands.begin(), m_commands.end(), [](const auto& element) { return element.m_type == RHI::IndirectCommandType::RootConstants; });
            EXPECT_NE(findIt, m_commands.end());
            auto commandIndex = m_writerSignature->GetLayout().FindCommandIndex(*findIt);
            EXPECT_FALSE(commandIndex.IsNull());
            auto nextCommandIndex = RHI::IndirectCommandIndex(commandIndex.GetIndex() + 1);
            uint32_t commandOffsett = 12;
            uint32_t nextCommandOffset = 16; 
            EXPECT_CALL(*m_writerSignature, GetOffsetInternal(commandIndex))
                .Times(1)
                .WillOnce(
                    testing::Return(commandOffsett));

            EXPECT_CALL(*m_writerSignature, GetOffsetInternal(nextCommandIndex))
                .Times(1)
                .WillOnce(
                    testing::Return(nextCommandOffset));

            AZ_TEST_START_TRACE_SUPPRESSION;
            uint64_t data;
            writer->SetRootConstants(reinterpret_cast<uint8_t*>(&data), sizeof(data));
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }

        // Shutdown
        {
            auto writer = CreateInitializedWriter();
            writer->Shutdown();
            EXPECT_FALSE(m_buffer->IsMapped());
            EXPECT_TRUE(writer->GetData() == nullptr);
        }
    }
}
