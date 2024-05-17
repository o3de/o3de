/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Tests/FrameGraph.h>
#include <Tests/Factory.h>
#include <Tests/Device.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <AzCore/Math/Random.h>

namespace UnitTest
{
    using namespace AZ;

    class FrameGraphTests
        : public RHITestFixture
    {
    public:
        FrameGraphTests()
            : RHITestFixture()
        {
        }

        void ValidateBinding(
            const RHI::Scope* scope,
            const RHI::BufferScopeAttachment* scopeAttachment,
            const RHI::SingleDeviceBuffer* buffer)
        {
            ASSERT_TRUE(scopeAttachment->GetPrevious() == nullptr);
            ASSERT_TRUE(scopeAttachment->GetNext() == nullptr);
            ASSERT_TRUE(&scopeAttachment->GetScope() == scope);

            const RHI::BufferFrameAttachment& attachment = scopeAttachment->GetFrameAttachment();
            ASSERT_TRUE(attachment.GetFirstScope() == scope);
            ASSERT_TRUE(attachment.GetLastScope() == scope);
            ASSERT_TRUE(attachment.GetFirstScopeAttachment() == scopeAttachment);
            ASSERT_TRUE(attachment.GetLastScopeAttachment() == scopeAttachment);

            if (buffer)
            {
                ASSERT_TRUE(buffer == attachment.GetBuffer());
            }
        }

        void ValidateBinding(
            const RHI::Scope* scope,
            const RHI::ImageScopeAttachment* scopeAttachment,
            const RHI::SingleDeviceImage* image)
        {
            ASSERT_TRUE(scopeAttachment->GetPrevious() == nullptr);
            ASSERT_TRUE(scopeAttachment->GetNext() == nullptr);
            ASSERT_TRUE(&scopeAttachment->GetScope() == scope);

            const RHI::ImageFrameAttachment& attachment = scopeAttachment->GetFrameAttachment();
            ASSERT_TRUE(attachment.GetFirstScope() == scope);
            ASSERT_TRUE(attachment.GetLastScope() == scope);
            ASSERT_TRUE(attachment.GetFirstScopeAttachment() == scopeAttachment);
            ASSERT_TRUE(attachment.GetLastScopeAttachment() == scopeAttachment);

            if (image)
            {
                ASSERT_TRUE(image == attachment.GetImage());
            }
        }

        void SetUp() override
        {
            RHITestFixture::SetUp();

            m_rootFactory.reset(aznew Factory());

            RHI::Ptr<RHI::Device> device = MakeTestDevice();

            m_state.reset(new State);

            {
                m_state->m_bufferPool = RHI::Factory::Get().CreateBufferPool();

                RHI::BufferPoolDescriptor desc;
                desc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
                m_state->m_bufferPool->Init(*device, desc);
            }

            for (uint32_t i = 0; i < BufferCount; ++i)
            {
                RHI::Ptr<RHI::SingleDeviceBuffer> buffer;
                buffer = RHI::Factory::Get().CreateBuffer();

                RHI::BufferDescriptor desc;
                desc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
                desc.m_byteCount = BufferSize;

                RHI::SingleDeviceBufferInitRequest request;
                request.m_descriptor = desc;
                request.m_buffer = buffer.get();
                m_state->m_bufferPool->InitBuffer(request);

                AZStd::string name = AZStd::string::format("B%d", i);
                m_state->m_bufferAttachments[i].m_id = RHI::AttachmentId(name);
                m_state->m_bufferAttachments[i].m_buffer = AZStd::move(buffer);
            }

            {
                m_state->m_imagePool = RHI::Factory::Get().CreateImagePool();

                RHI::ImagePoolDescriptor desc;
                desc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite;
                m_state->m_imagePool->Init(*device, desc);
            }

            for (uint32_t i = 0; i < ImageCount; ++i)
            {
                RHI::Ptr<RHI::SingleDeviceImage> image;
                image = RHI::Factory::Get().CreateImage();

                RHI::ImageDescriptor desc = RHI::ImageDescriptor::Create2D(
                    RHI::ImageBindFlags::ShaderReadWrite,
                    ImageSize,
                    ImageSize,
                    RHI::Format::R8G8B8A8_UNORM);

                RHI::SingleDeviceImageInitRequest request;
                request.m_descriptor = desc;
                request.m_image = image.get();
                m_state->m_imagePool->InitImage(request);

                m_state->m_imageAttachments[i].m_id = RHI::AttachmentId(AZStd::string::format("I%d", i));
                m_state->m_imageAttachments[i].m_image = AZStd::move(image);
            }

            for (uint32_t i = 0; i < ScopeCount; ++i)
            {
                RHI::Ptr<RHI::Scope> scope;
                scope = RHI::Factory::Get().CreateScope();
                scope->Init(RHI::ScopeId{AZStd::string::format("S%d", i)});
                m_state->m_scopes[i] = AZStd::move(scope);
            }

            m_state->m_frameGraphCompiler = RHI::Factory::Get().CreateFrameGraphCompiler();
            m_state->m_frameGraphCompiler->Init(*device);
        }

        void TearDown() override
        {
            m_state.reset();
            m_rootFactory.reset();
            RHITestFixture::TearDown();
        }

        void TestEmptyGraph()
        {
            RHI::FrameGraph frameGraph;

            for (uint32_t frameIdx = 0; frameIdx < FrameIterationCount; ++frameIdx)
            {
                /// Test an empty graph.
                frameGraph.Begin();

                frameGraph.End();

                {
                    RHI::FrameGraphCompileRequest request;
                    request.m_frameGraph = &frameGraph;
                    m_state->m_frameGraphCompiler->Compile(request);
                }

                ASSERT_TRUE(frameGraph.GetScopes().empty());
            }
        }

        void TestSingleEmptyScope()
        {
            RHI::FrameGraph frameGraph;

            for (uint32_t frameIdx = 0; frameIdx < FrameIterationCount; ++frameIdx)
            {
                frameGraph.Begin();

                frameGraph.BeginScope(*m_state->m_scopes[0]);

                frameGraph.EndScope();

                frameGraph.End();

                {
                    RHI::FrameGraphCompileRequest request;
                    request.m_frameGraph = &frameGraph;
                    m_state->m_frameGraphCompiler->Compile(request);
                }

                ASSERT_TRUE(frameGraph.GetScopes().size() == 1);
                ASSERT_TRUE(frameGraph.GetScopes()[0] == m_state->m_scopes[0]);
            }
        }

        void TestSingleScope()
        {
            RHI::FrameGraph frameGraph;

            for (uint32_t frameIdx = 0; frameIdx < FrameIterationCount; ++frameIdx)
            {
                frameGraph.Begin();

                frameGraph.BeginScope(*m_state->m_scopes[0]);
                frameGraph.SetHardwareQueueClass(RHI::HardwareQueueClass::Graphics);

                frameGraph.GetAttachmentDatabase().ImportBuffer(m_state->m_bufferAttachments[0].m_id, m_state->m_bufferAttachments[0].m_buffer);
                frameGraph.GetAttachmentDatabase().ImportBuffer(m_state->m_bufferAttachments[1].m_id, m_state->m_bufferAttachments[1].m_buffer);

                {
                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = m_state->m_bufferAttachments[0].m_id;
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize);
                    desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Clear;
                    desc.m_loadStoreAction.m_clearValue = RHI::ClearValue::CreateVector4Float(1.0f, 0.0, 0.0, 0.0);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);

                    desc.m_attachmentId = m_state->m_bufferAttachments[1].m_id;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                frameGraph.GetAttachmentDatabase().ImportImage(m_state->m_imageAttachments[0].m_id, m_state->m_imageAttachments[0].m_image);
                frameGraph.GetAttachmentDatabase().ImportImage(m_state->m_imageAttachments[1].m_id, m_state->m_imageAttachments[1].m_image);

                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = m_state->m_imageAttachments[0].m_id;
                    desc.m_loadStoreAction.m_clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 1.0f, 0.0f, 1.0f);
                    desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Clear;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor();
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);

                    desc.m_attachmentId = m_state->m_imageAttachments[1].m_id;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                const RHI::FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();
                ASSERT_TRUE(attachmentDatabase.GetAttachments().size() == 4);
                ASSERT_TRUE(attachmentDatabase.GetBufferDescriptor(m_state->m_bufferAttachments[0].m_id).GetHash() == m_state->m_bufferAttachments[0].m_buffer->GetDescriptor().GetHash());
                ASSERT_TRUE(attachmentDatabase.GetBufferDescriptor(m_state->m_bufferAttachments[1].m_id).GetHash() == m_state->m_bufferAttachments[1].m_buffer->GetDescriptor().GetHash());
                ASSERT_TRUE(attachmentDatabase.GetImageDescriptor(m_state->m_imageAttachments[0].m_id).GetHash() == m_state->m_imageAttachments[0].m_image->GetDescriptor().GetHash());
                ASSERT_TRUE(attachmentDatabase.GetImageDescriptor(m_state->m_imageAttachments[1].m_id).GetHash() == m_state->m_imageAttachments[1].m_image->GetDescriptor().GetHash());

                frameGraph.EndScope();

                frameGraph.End();

                {
                    RHI::FrameGraphCompileRequest request;
                    request.m_frameGraph = &frameGraph;
                    m_state->m_frameGraphCompiler->Compile(request);
                }

                ASSERT_TRUE(frameGraph.GetScopes().size() == 1);
                ASSERT_TRUE(frameGraph.GetScopes()[0] == m_state->m_scopes[0]);

                const RHI::Scope* scope = frameGraph.FindScope(m_state->m_scopes[0]->GetId());
                ASSERT_TRUE(scope == m_state->m_scopes[0].get());
                ASSERT_TRUE(scope->GetIndex() == 0);

                ASSERT_TRUE(scope->GetBufferAttachments().size() == 2);

                for (uint32_t i = 0; i < 2; ++i)
                {
                    ValidateBinding(scope, scope->GetBufferAttachments()[i], m_state->m_bufferAttachments[i].m_buffer.get());
                }

                ASSERT_TRUE(scope->GetImageAttachments().size() == 2);
                ASSERT_TRUE(scope->GetAttachments().size() == 4);

                for (uint32_t i = 0; i < 2; ++i)
                {
                    ValidateBinding(scope, scope->GetImageAttachments()[i], m_state->m_imageAttachments[i].m_image.get());
                }
            }
        }

        void TestScopeGraph()
        {
            RHI::FrameGraph frameGraph;

            RHI::ImageScopeAttachmentDescriptor imageBindingDescs[2];
            imageBindingDescs[0].m_imageViewDescriptor = RHI::ImageViewDescriptor();
            imageBindingDescs[0].m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Clear;
            imageBindingDescs[0].m_loadStoreAction.m_clearValue = RHI::ClearValue::CreateVector4Float(1.0f, 0.0, 0.0, 0.0);
            imageBindingDescs[1] = imageBindingDescs[0];
            imageBindingDescs[1].m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;

            RHI::BufferScopeAttachmentDescriptor bufferBindingDescs[2];
            bufferBindingDescs[0].m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize);
            bufferBindingDescs[0].m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Clear;
            bufferBindingDescs[0].m_loadStoreAction.m_clearValue = RHI::ClearValue::CreateVector4Float(1.0f, 0.0, 0.0, 0.0);
            bufferBindingDescs[1] = bufferBindingDescs[0];
            bufferBindingDescs[1].m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;

            AZ::SimpleLcgRandom random;

            struct Interval
            {
                uint32_t m_begin;
                uint32_t m_end;
            };

            for (uint32_t frameIdx = 0; frameIdx < FrameIterationCount; ++frameIdx)
            {
                frameGraph.Begin();

                Interval bufferScopeIntervals[BufferCount];
                for (uint32_t i = 0; i < BufferCount; ++i)
                {
                    uint32_t b = random.GetRandom() % ScopeCount;
                    uint32_t e = random.GetRandom() % ScopeCount;
                    if (b > e)
                    {
                        AZStd::swap(b, e);
                    }

                    bufferScopeIntervals[i].m_begin = b;
                    bufferScopeIntervals[i].m_end = e;
                }

                Interval imageScopeIntervals[ImageCount];
                for (uint32_t i = 0; i < ImageCount; ++i)
                {
                    uint32_t b = random.GetRandom() % ScopeCount;
                    uint32_t e = random.GetRandom() % ScopeCount;
                    if (b > e)
                    {
                        AZStd::swap(b, e);
                    }

                    imageScopeIntervals[i].m_begin = b;
                    imageScopeIntervals[i].m_end = e;
                }

                for (uint32_t scopeIdx = 0; scopeIdx < ScopeCount; ++scopeIdx)
                {
                    if (scopeIdx == 0)
                    {
                        for (uint32_t i = 0; i < BufferCount; ++i)
                        {
                            frameGraph.GetAttachmentDatabase().ImportBuffer(m_state->m_bufferAttachments[i].m_id, m_state->m_bufferAttachments[i].m_buffer);
                        }

                        for (uint32_t i = 0; i < ImageCount; ++i)
                        {
                            frameGraph.GetAttachmentDatabase().ImportImage(m_state->m_imageAttachments[i].m_id, m_state->m_imageAttachments[i].m_image);
                        }
                    }

                    frameGraph.BeginScope(*m_state->m_scopes[scopeIdx]);

                    for (uint32_t i = 0; i < BufferCount; ++i)
                    {
                        if (scopeIdx == bufferScopeIntervals[i].m_begin)
                        {
                            bufferBindingDescs[0].m_attachmentId = m_state->m_bufferAttachments[i].m_id;
                            frameGraph.UseShaderAttachment(bufferBindingDescs[0], RHI::ScopeAttachmentAccess::ReadWrite);
                        }
                        else if (scopeIdx == bufferScopeIntervals[i].m_end)
                        {
                            bufferBindingDescs[1].m_attachmentId = m_state->m_bufferAttachments[i].m_id;
                            frameGraph.UseShaderAttachment(bufferBindingDescs[1], RHI::ScopeAttachmentAccess::Read);
                        }
                    }

                    for (uint32_t i = 0; i < ImageCount; ++i)
                    {
                        if (scopeIdx == imageScopeIntervals[i].m_begin)
                        {
                            imageBindingDescs[0].m_attachmentId = m_state->m_imageAttachments[i].m_id;
                            frameGraph.UseShaderAttachment(imageBindingDescs[0], RHI::ScopeAttachmentAccess::ReadWrite);
                        }
                        else if (scopeIdx == imageScopeIntervals[i].m_end)
                        {
                            imageBindingDescs[1].m_attachmentId = m_state->m_imageAttachments[i].m_id;
                            frameGraph.UseShaderAttachment(imageBindingDescs[1], RHI::ScopeAttachmentAccess::Read);
                        }
                    }

                    frameGraph.EndScope();
                }

                frameGraph.End();

                {
                    RHI::FrameGraphCompileRequest request;
                    request.m_frameGraph = &frameGraph;
                    m_state->m_frameGraphCompiler->Compile(request);
                }

                const RHI::FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();

                ASSERT_TRUE(frameGraph.GetScopes().size() == ScopeCount);
                ASSERT_TRUE(attachmentDatabase.GetAttachments().size() == BufferCount + ImageCount);
                ASSERT_TRUE(attachmentDatabase.GetBufferAttachments().size() == BufferCount);
                ASSERT_TRUE(attachmentDatabase.GetImageAttachments().size() == ImageCount);
                ASSERT_TRUE(attachmentDatabase.GetImportedImageAttachments().size() == ImageCount);
                ASSERT_TRUE(attachmentDatabase.GetImportedBufferAttachments().size() == BufferCount);

                for (uint32_t i = 0; i < ImageCount; ++i)
                {
                    ASSERT_TRUE(attachmentDatabase.FindAttachment(m_state->m_imageAttachments[i].m_id) != nullptr);
                }

                for (uint32_t i = 0; i < BufferCount; ++i)
                {
                    ASSERT_TRUE(attachmentDatabase.FindAttachment(m_state->m_bufferAttachments[i].m_id) != nullptr);
                }

                for (const RHI::FrameAttachment* attachment : attachmentDatabase.GetAttachments())
                {
                    const RHI::ScopeAttachment* scopeAttachmentPrev = nullptr;
                    for (const RHI::ScopeAttachment* scopeAttachment = attachment->GetFirstScopeAttachment();
                        scopeAttachment;
                        scopeAttachment = scopeAttachment->GetNext())
                    {
                        ASSERT_TRUE(&scopeAttachment->GetFrameAttachment() == attachment);
                        ASSERT_TRUE(scopeAttachment->GetPrevious() == scopeAttachmentPrev);

                        if (scopeAttachmentPrev)
                        {
                            ASSERT_TRUE(scopeAttachmentPrev->GetScope().GetIndex() < scopeAttachment->GetScope().GetIndex());
                        }

                        scopeAttachmentPrev = scopeAttachment;
                    }
                }
            }
        }

    private:
        static const uint32_t FrameIterationCount = 32;
        static const uint32_t ImageCount = 256;
        static const uint32_t BufferCount = 256;
        static const uint32_t BufferSize = 64;
        static const uint32_t ImageSize = 16;
        static const uint32_t ScopeCount = 128;

        AZStd::unique_ptr<Factory> m_rootFactory;

        struct ImageAttachment
        {
            RHI::Ptr<RHI::SingleDeviceImage> m_image;
            RHI::AttachmentId m_id;
        };

        struct BufferAttachment
        {
            RHI::Ptr<RHI::SingleDeviceBuffer> m_buffer;
            RHI::AttachmentId m_id;
        };

        struct State
        {
            RHI::Ptr<RHI::SingleDeviceBufferPool> m_bufferPool;
            RHI::Ptr<RHI::SingleDeviceImagePool> m_imagePool;
            RHI::Ptr<RHI::FrameGraphCompiler> m_frameGraphCompiler;

            ImageAttachment m_imageAttachments[ImageCount];
            BufferAttachment m_bufferAttachments[BufferCount];
            RHI::Ptr<RHI::Scope> m_scopes[ScopeCount];

        };

        AZStd::unique_ptr<State> m_state;
    };

    TEST_F(FrameGraphTests, TestEmptyGraph)
    {
        TestEmptyGraph();
    }

    TEST_F(FrameGraphTests, TestSingleEmptyScope)
    {
        TestSingleEmptyScope();
    }

    TEST_F(FrameGraphTests, TestSingleScope)
    {
        TestSingleScope();
    }

    TEST_F(FrameGraphTests, TestScopeGraph)
    {
        TestScopeGraph();
    }
}
