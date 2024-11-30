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
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/BufferPool.h>
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
            const RHI::Buffer* buffer)
        {
            ASSERT_TRUE(scopeAttachment->GetPrevious() == nullptr);
            ASSERT_TRUE(scopeAttachment->GetNext() == nullptr);
            ASSERT_TRUE(&scopeAttachment->GetScope() == scope);

            const RHI::BufferFrameAttachment& attachment = scopeAttachment->GetFrameAttachment();
            ASSERT_TRUE(attachment.GetFirstScope(scope->GetDeviceIndex()) == scope);
            ASSERT_TRUE(attachment.GetLastScope(scope->GetDeviceIndex()) == scope);
            ASSERT_TRUE(attachment.GetFirstScopeAttachment(scope->GetDeviceIndex()) == scopeAttachment);
            ASSERT_TRUE(attachment.GetLastScopeAttachment(scope->GetDeviceIndex()) == scopeAttachment);

            if (buffer)
            {
                ASSERT_TRUE(buffer == attachment.GetBuffer());
            }
        }

        void ValidateBinding(
            const RHI::Scope* scope,
            const RHI::ImageScopeAttachment* scopeAttachment,
            const RHI::Image* image)
        {
            ASSERT_TRUE(scopeAttachment->GetPrevious() == nullptr);
            ASSERT_TRUE(scopeAttachment->GetNext() == nullptr);
            ASSERT_TRUE(&scopeAttachment->GetScope() == scope);

            const RHI::ImageFrameAttachment& attachment = scopeAttachment->GetFrameAttachment();
            ASSERT_TRUE(attachment.GetFirstScope(scope->GetDeviceIndex()) == scope);
            ASSERT_TRUE(attachment.GetLastScope(scope->GetDeviceIndex()) == scope);
            ASSERT_TRUE(attachment.GetFirstScopeAttachment(scope->GetDeviceIndex()) == scopeAttachment);
            ASSERT_TRUE(attachment.GetLastScopeAttachment(scope->GetDeviceIndex()) == scopeAttachment);

            if (image)
            {
                ASSERT_TRUE(image == attachment.GetImage());
            }
        }

        void SetUp() override
        {
            RHITestFixture::SetUp();

            m_rootFactory.reset(aznew Factory());

            m_rhiSystem.reset(aznew AZ::RHI::RHISystem);
            m_rhiSystem->InitDevices();
            m_rhiSystem->Init();

            m_state.reset(new State);

            {
                m_state->m_bufferPool = aznew RHI::BufferPool;

                RHI::BufferPoolDescriptor desc;
                desc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
                desc.m_deviceMask = RHI::MultiDevice::DefaultDevice;
                m_state->m_bufferPool->Init(desc);
            }

            for (uint32_t i = 0; i < BufferCount; ++i)
            {
                RHI::Ptr<RHI::Buffer> buffer;
                buffer = aznew RHI::Buffer;

                RHI::BufferDescriptor desc;
                desc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
                desc.m_byteCount = BufferSize;

                RHI::BufferInitRequest request;
                request.m_descriptor = desc;
                request.m_buffer = buffer.get();
                m_state->m_bufferPool->InitBuffer(request);

                AZStd::string name = AZStd::string::format("B%d", i);
                m_state->m_bufferAttachments[i].m_id = RHI::AttachmentId(name);
                m_state->m_bufferAttachments[i].m_buffer = AZStd::move(buffer);
            }

            {
                m_state->m_imagePool = aznew RHI::ImagePool;

                RHI::ImagePoolDescriptor desc;
                desc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite;
                desc.m_deviceMask = RHI::MultiDevice::DefaultDevice;
                m_state->m_imagePool->Init(desc);
            }

            for (uint32_t i = 0; i < ImageCount; ++i)
            {
                RHI::Ptr<RHI::Image> image;
                image = aznew RHI::Image;

                RHI::ImageDescriptor desc = RHI::ImageDescriptor::Create2D(
                    RHI::ImageBindFlags::ShaderReadWrite,
                    ImageSize,
                    ImageSize,
                    RHI::Format::R8G8B8A8_UNORM);

                desc.m_mipLevels = ImageMipCount;
                desc.m_arraySize = ImageArrayCount;

                RHI::ImageInitRequest request;
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
            m_state->m_frameGraphCompiler->Init();
        }

        void TearDown() override
        {
            m_state.reset();
            m_rhiSystem->Shutdown();
            m_rhiSystem.reset();
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
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);

                    desc.m_attachmentId = m_state->m_bufferAttachments[1].m_id;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                }

                frameGraph.GetAttachmentDatabase().ImportImage(m_state->m_imageAttachments[0].m_id, m_state->m_imageAttachments[0].m_image);
                frameGraph.GetAttachmentDatabase().ImportImage(m_state->m_imageAttachments[1].m_id, m_state->m_imageAttachments[1].m_image);
                frameGraph.GetAttachmentDatabase().ImportImage(m_state->m_imageAttachments[2].m_id, m_state->m_imageAttachments[2].m_image);

                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = m_state->m_imageAttachments[0].m_id;
                    desc.m_loadStoreAction.m_clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 1.0f, 0.0f, 1.0f);
                    desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Clear;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor();
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);

                    desc.m_attachmentId = m_state->m_imageAttachments[1].m_id;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);

                    desc.m_attachmentId = m_state->m_imageAttachments[2].m_id;
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::Uninitialized);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                }

                const RHI::FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();
                ASSERT_TRUE(attachmentDatabase.GetAttachments().size() == 5);
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

                ASSERT_TRUE(scope->GetImageAttachments().size() == 3);
                ASSERT_TRUE(scope->GetAttachments().size() == 5);

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
                            frameGraph.UseShaderAttachment(
                                bufferBindingDescs[0], RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                        }
                        else if (scopeIdx == bufferScopeIntervals[i].m_end)
                        {
                            bufferBindingDescs[1].m_attachmentId = m_state->m_bufferAttachments[i].m_id;
                            frameGraph.UseShaderAttachment(
                                bufferBindingDescs[1], RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::AnyGraphics);
                        }
                    }

                    for (uint32_t i = 0; i < ImageCount; ++i)
                    {
                        if (scopeIdx == imageScopeIntervals[i].m_begin)
                        {
                            imageBindingDescs[0].m_attachmentId = m_state->m_imageAttachments[i].m_id;
                            frameGraph.UseShaderAttachment(
                                imageBindingDescs[0], RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                        }
                        else if (scopeIdx == imageScopeIntervals[i].m_end)
                        {
                            imageBindingDescs[1].m_attachmentId = m_state->m_imageAttachments[i].m_id;
                            frameGraph.UseShaderAttachment(
                                imageBindingDescs[1], RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::AnyGraphics);
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
                    for (const RHI::ScopeAttachment* scopeAttachment =
                             attachment->GetFirstScopeAttachment(RHI::MultiDevice::DefaultDeviceIndex);
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

        void TestOverlappingAttachments()
        {
            RHI::FrameGraph frameGraph;

            for (uint32_t frameIdx = 0; frameIdx < FrameIterationCount; ++frameIdx)
            {
                frameGraph.Begin();

                frameGraph.BeginScope(*m_state->m_scopes[0]);
                frameGraph.SetHardwareQueueClass(RHI::HardwareQueueClass::Graphics);

                constexpr uint32_t numBufferImports = 6;
                for (uint32_t i = 0; i < numBufferImports; ++i)
                {
                    frameGraph.GetAttachmentDatabase().ImportBuffer(
                        m_state->m_bufferAttachments[i].m_id, m_state->m_bufferAttachments[i].m_buffer);
                }

                {
                    // Same attachment added twice
                    RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = m_state->m_bufferAttachments[0].m_id;
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize);
                    desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::DontCare;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    AZ_TEST_STOP_ASSERTTEST(1);

                    // Partial overlap
                    desc.m_attachmentId = m_state->m_bufferAttachments[1].m_id;
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_bufferViewDescriptor.m_elementOffset = 0;
                    desc.m_bufferViewDescriptor.m_elementCount = 1;
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    AZ_TEST_STOP_ASSERTTEST(1);

                    // Edge overlap
                    desc.m_attachmentId = m_state->m_bufferAttachments[2].m_id;
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize/2);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw((BufferSize / 2) - 1, BufferSize);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    AZ_TEST_STOP_ASSERTTEST(1);

                    // No overlap
                    desc.m_attachmentId = m_state->m_bufferAttachments[3].m_id;
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize/2);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw((BufferSize / 2) + 1, BufferSize);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);

                    // Overlap read only
                    desc.m_attachmentId = m_state->m_bufferAttachments[4].m_id;
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize / 2);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw((BufferSize / 2) - 1, BufferSize);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::AnyGraphics);

                    // Overlap with invalid usage
                    desc.m_attachmentId = m_state->m_bufferAttachments[5].m_id;
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, BufferSize);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseAttachment(
                        desc,
                        RHI::ScopeAttachmentAccess::Read,
                        RHI::ScopeAttachmentUsage::InputAssembly,
                        RHI::ScopeAttachmentStage::VertexInput);
                    AZ_TEST_STOP_ASSERTTEST(1);
                }

                constexpr uint32_t numImageImports = 9;
                for (uint32_t i = 0; i < numImageImports; ++i)
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(
                        m_state->m_imageAttachments[i].m_id, m_state->m_imageAttachments[i].m_image);
                }

                {
                    // Same attachment twice
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = m_state->m_imageAttachments[0].m_id;
                    desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::DontCare;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor();
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    AZ_TEST_STOP_ASSERTTEST(1);

                    // Mipmap overlap
                    desc.m_attachmentId = m_state->m_imageAttachments[1].m_id;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 1);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 1, 2);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    AZ_TEST_STOP_ASSERTTEST(1);

                    // Mipmap overlap, Slice Overlap
                    desc.m_attachmentId = m_state->m_imageAttachments[2].m_id;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 1, 0, 1);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 1, 2, 1, 2);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    AZ_TEST_STOP_ASSERTTEST(1);

                    // No overlap, different aspect mask
                    desc.m_attachmentId = m_state->m_imageAttachments[3].m_id;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor();
                    desc.m_imageViewDescriptor.m_aspectFlags = RHI::ImageAspectFlags::Depth;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_imageViewDescriptor.m_aspectFlags = RHI::ImageAspectFlags::Stencil;
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);

                    // No overlap on mimap
                    desc.m_attachmentId = m_state->m_imageAttachments[4].m_id;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 1, 0, 1);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 2, 3, 0, 1);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);

                    // No overlap on slice
                    desc.m_attachmentId = m_state->m_imageAttachments[5].m_id;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 1, 0, 1);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 1, 2, 3);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);

                    // No overlap on mipmap and slice
                    desc.m_attachmentId = m_state->m_imageAttachments[6].m_id;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 1, 1, 2);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 2, 3, 3, 4);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::AnyGraphics);

                    // Overlap read only
                    desc.m_attachmentId = m_state->m_imageAttachments[7].m_id;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor();
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::AnyGraphics);
                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::AnyGraphics);

                    // Overlap invalid usage
                    desc.m_attachmentId = m_state->m_imageAttachments[8].m_id;
                    desc.m_imageViewDescriptor = RHI::ImageViewDescriptor();
                    frameGraph.UseDepthStencilAttachment(
                        desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::EarlyFragmentTest);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    frameGraph.UseDepthStencilAttachment(
                        desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::EarlyFragmentTest);
                    AZ_TEST_STOP_ASSERTTEST(1);
                }

                frameGraph.EndScope();

                frameGraph.End();
            }
        }

    private:
        static const uint32_t FrameIterationCount = 32;
        static const uint32_t ImageCount = 256;
        static const uint32_t BufferCount = 256;
        static const uint32_t BufferSize = 64;
        static const uint32_t ImageSize = 16;
        static const uint32_t ImageMipCount = 5;
        static const uint32_t ImageArrayCount = 3;
        static const uint32_t ScopeCount = 128;

        AZStd::unique_ptr<AZ::RHI::RHISystem> m_rhiSystem;
        AZStd::unique_ptr<Factory> m_rootFactory;

        struct ImageAttachment
        {
            RHI::Ptr<RHI::Image> m_image;
            RHI::AttachmentId m_id;
        };

        struct BufferAttachment
        {
            RHI::Ptr<RHI::Buffer> m_buffer;
            RHI::AttachmentId m_id;
        };

        struct State
        {
            RHI::Ptr<RHI::BufferPool> m_bufferPool;
            RHI::Ptr<RHI::ImagePool> m_imagePool;
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

    TEST_F(FrameGraphTests, TestOverlappingAttachments)
    {
        TestOverlappingAttachments();
    }
}
