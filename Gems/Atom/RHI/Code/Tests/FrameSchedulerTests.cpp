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
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI/FrameScheduler.h>
#include <AzCore/Math/Random.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/ImagePool.h>

#include <Atom/RHI/RHISystemInterface.h>

namespace UnitTest
{
    using namespace AZ;

    struct ImportedImage
    {
        RHI::AttachmentId m_id;
        RHI::Ptr<RHI::Image> m_image;
    };

    struct ImportedBuffer
    {
        RHI::AttachmentId m_id;
        RHI::Ptr<RHI::Buffer> m_buffer;
    };

    struct TransientImage
    {
        RHI::AttachmentId m_id;
        RHI::ImageDescriptor m_descriptor;
    };

    struct TransientBuffer
    {
        RHI::AttachmentId m_id;
        RHI::BufferDescriptor m_descriptor;
    };

    class ScopeProducer
        : public RHI::ScopeProducer
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopeProducer, SystemAllocator);

        ScopeProducer(const RHI::ScopeId& scopeId)
            : RHI::ScopeProducer(scopeId)
        {}

        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override
        {
            RHI::FrameGraphAttachmentInterface attachmentDatabase = frameGraph.GetAttachmentDatabase();

            for (ImportedImage& image : m_imageImports)
            {
                ASSERT_FALSE(attachmentDatabase.IsAttachmentValid(image.m_id));
                attachmentDatabase.ImportImage(image.m_id, image.m_image);
                ASSERT_TRUE(attachmentDatabase.IsAttachmentValid(image.m_id));
            }

            for (ImportedBuffer& buffer : m_bufferImports)
            {
                ASSERT_FALSE(attachmentDatabase.IsAttachmentValid(buffer.m_id));
                attachmentDatabase.ImportBuffer(buffer.m_id, buffer.m_buffer);
                ASSERT_TRUE(attachmentDatabase.IsAttachmentValid(buffer.m_id));
            }

            for (const TransientImage& image : m_transientImages)
            {
                ASSERT_FALSE(attachmentDatabase.IsAttachmentValid(image.m_id));
                attachmentDatabase.CreateTransientImage(RHI::TransientImageDescriptor{image.m_id, image.m_descriptor});
                ASSERT_TRUE(attachmentDatabase.IsAttachmentValid(image.m_id));
            }

            for (const TransientBuffer& buffer : m_transientBuffers)
            {
                ASSERT_FALSE(attachmentDatabase.IsAttachmentValid(buffer.m_id));
                attachmentDatabase.CreateTransientBuffer(RHI::TransientBufferDescriptor{buffer.m_id, buffer.m_descriptor});
                ASSERT_TRUE(attachmentDatabase.IsAttachmentValid(buffer.m_id));
            }

            for (const ImageUsage& usage : m_imageUsages)
            {
                frameGraph.UseShaderAttachment(usage.m_descriptor, usage.m_access, RHI::ScopeAttachmentStage::AnyGraphics);
            }

            for (const BufferUsage& usage : m_bufferUsages)
            {
                frameGraph.UseShaderAttachment(usage.m_descriptor, usage.m_access, RHI::ScopeAttachmentStage::AnyGraphics);
            }

            frameGraph.SetEstimatedItemCount(0);
        }

        void CompileResources(const RHI::FrameGraphCompileContext& context) override
        {
            ASSERT_TRUE(context.GetScopeId() == GetScopeId());

            for (const ImageUsage& usage : m_imageUsages)
            {
                ASSERT_TRUE(context.GetImageView(usage.m_descriptor.m_attachmentId) != nullptr);
            }

            for (const BufferUsage& usage : m_bufferUsages)
            {
                ASSERT_TRUE(context.GetBufferView(usage.m_descriptor.m_attachmentId) != nullptr);
            }
        }

        void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override
        {
            ASSERT_TRUE(context.GetScopeId() == GetScopeId());
            ASSERT_TRUE(context.GetCommandListIndex() == 0);
            ASSERT_TRUE(context.GetCommandListCount() == 1);
        }

        AZStd::vector<ImportedImage> m_imageImports;
        AZStd::vector<ImportedBuffer> m_bufferImports;
        AZStd::vector<TransientImage> m_transientImages;
        AZStd::vector<TransientBuffer> m_transientBuffers;

        struct ImageUsage
        {
            RHI::ImageScopeAttachmentDescriptor m_descriptor;
            RHI::ScopeAttachmentAccess m_access;
        };

        struct BufferUsage
        {
            RHI::BufferScopeAttachmentDescriptor m_descriptor;
            RHI::ScopeAttachmentAccess m_access;
        };

        AZStd::vector<ImageUsage> m_imageUsages;
        AZStd::vector<BufferUsage> m_bufferUsages;
    };

    class FrameSchedulerTests
        : public RHITestFixture
    {
    public:
        FrameSchedulerTests()
            : RHITestFixture()
        {
        }

        void SetUp() override
        {
            UnitTest::RHITestFixture::SetUp();

            m_rootFactory.reset(aznew Factory());

            m_rhiSystem.reset(aznew AZ::RHI::RHISystem);
            m_rhiSystem->InitDevices();
            m_rhiSystem->Init();

            m_device = AZ::RHI::RHISystemInterface::Get()->GetDevice(RHI::MultiDevice::DefaultDeviceIndex);

            m_state.reset(new State);

            {
                m_state->m_bufferPool = aznew RHI::BufferPool;

                RHI::BufferPoolDescriptor desc;
                desc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
                desc.m_deviceMask = RHI::MultiDevice::DefaultDevice;
                m_state->m_bufferPool->Init(desc);
            }

            for (uint32_t i = 0; i < ImportedBufferCount; ++i)
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

                m_state->m_bufferAttachments[i].m_id = RHI::AttachmentId{AZStd::string::format("B%d", i)};
                m_state->m_bufferAttachments[i].m_buffer = AZStd::move(buffer);
            }

            {
                m_state->m_imagePool = aznew RHI::ImagePool();

                RHI::ImagePoolDescriptor desc;
                desc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite;
                m_state->m_imagePool->Init(desc);
            }

            for (uint32_t i = 0; i < ImportedImageCount; ++i)
            {
                RHI::Ptr<RHI::Image> image;
                image = aznew RHI::Image();

                RHI::ImageDescriptor desc = RHI::ImageDescriptor::Create2D(
                    RHI::ImageBindFlags::ShaderReadWrite,
                    ImageSize,
                    ImageSize,
                    RHI::Format::R8G8B8A8_UNORM);

                RHI::ImageInitRequest request;
                request.m_descriptor = desc;
                request.m_image = image.get();
                m_state->m_imagePool->InitImage(request);

                m_state->m_imageAttachments[i].m_id = RHI::AttachmentId{AZStd::string::format("I%d", i)};
                m_state->m_imageAttachments[i].m_image = AZStd::move(image);
            }

            for (uint32_t i = 0; i < ScopeCount; ++i)
            {
                m_state->m_producers.emplace_back(aznew ScopeProducer(RHI::ScopeId{AZStd::string::format("S%d", i)}));
            }
        }

        void TearDown() override
        {
            m_state.reset();
            m_device = nullptr;
            m_rhiSystem->Shutdown();
            m_rhiSystem.reset();
            m_rootFactory.reset();
            RHITestFixture::TearDown();
        }

        void Test()
        {
            RHI::FrameScheduler frameScheduler;

            RHI::FrameSchedulerDescriptor descriptor;
            descriptor.m_transientAttachmentPoolDescriptors[RHI::MultiDevice::DefaultDeviceIndex].m_bufferBudgetInBytes = 80 * 1024 * 1024;
            frameScheduler.Init(RHI::MultiDevice::DefaultDevice, descriptor);

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
                ScopeProducer& producer = *m_state->m_producers[scopeIdx];

                //
                // IMPORTS
                //

                for (uint32_t i = 0; i < ImportedBufferCount; ++i)
                {
                    if (scopeIdx == bufferScopeIntervals[i].m_begin)
                    {
                        producer.m_bufferImports.push_back(m_state->m_bufferAttachments[i]);
                        bufferBindingDescs[0].m_attachmentId = m_state->m_bufferAttachments[i].m_id;
                        producer.m_bufferUsages.push_back(ScopeProducer::BufferUsage{ bufferBindingDescs[0], RHI::ScopeAttachmentAccess::ReadWrite });
                    }
                    else if (scopeIdx == bufferScopeIntervals[i].m_end)
                    {
                        bufferBindingDescs[1].m_attachmentId = m_state->m_bufferAttachments[i].m_id;
                        producer.m_bufferUsages.push_back(ScopeProducer::BufferUsage{ bufferBindingDescs[1], RHI::ScopeAttachmentAccess::Read });
                    }
                }

                for (uint32_t i = 0; i < ImportedImageCount; ++i)
                {
                    if (scopeIdx == imageScopeIntervals[i].m_begin)
                    {
                        producer.m_imageImports.push_back(m_state->m_imageAttachments[i]);
                        imageBindingDescs[0].m_attachmentId = m_state->m_imageAttachments[i].m_id;
                        producer.m_imageUsages.push_back(ScopeProducer::ImageUsage{ imageBindingDescs[0], RHI::ScopeAttachmentAccess::ReadWrite });
                    }
                    else if (scopeIdx == imageScopeIntervals[i].m_end)
                    {
                        imageBindingDescs[1].m_attachmentId = m_state->m_imageAttachments[i].m_id;
                        producer.m_imageUsages.push_back(ScopeProducer::ImageUsage{ imageBindingDescs[1], RHI::ScopeAttachmentAccess::Read });
                    }
                }

                //
                // TRANSIENTS
                //

                for (uint32_t i = 0; i < TransientBufferCount; ++i)
                {
                    const uint32_t adjustedIndex = i + ImportedBufferCount;

                    TransientBuffer transientBuffer =
                    {
                        RHI::AttachmentId{AZStd::string::format("B%d", adjustedIndex)},
                        RHI::BufferDescriptor(RHI::BufferBindFlags::ShaderReadWrite, BufferSize)
                    };

                    bufferBindingDescs[0].m_attachmentId = transientBuffer.m_id;
                    bufferBindingDescs[1].m_attachmentId = transientBuffer.m_id;

                    if (scopeIdx == bufferScopeIntervals[adjustedIndex].m_begin)
                    {
                        producer.m_transientBuffers.push_back(transientBuffer);
                        producer.m_bufferUsages.push_back(ScopeProducer::BufferUsage{ bufferBindingDescs[0], RHI::ScopeAttachmentAccess::ReadWrite });
                    }
                    else if (scopeIdx == bufferScopeIntervals[adjustedIndex].m_end)
                    {
                        producer.m_bufferUsages.push_back(ScopeProducer::BufferUsage{ bufferBindingDescs[1], RHI::ScopeAttachmentAccess::Read });
                    }
                }

                for (uint32_t i = 0; i < TransientImageCount; ++i)
                {
                    const uint32_t adjustedIndex = i + ImportedImageCount;

                    TransientImage transientImage =
                    {
                        RHI::AttachmentId{AZStd::string::format("I%d", adjustedIndex)},
                        RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite, ImageSize, ImageSize, RHI::Format::R8G8B8A8_UNORM)
                    };

                    imageBindingDescs[0].m_attachmentId = transientImage.m_id;
                    imageBindingDescs[1].m_attachmentId = transientImage.m_id;

                    if (scopeIdx == imageScopeIntervals[adjustedIndex].m_begin)
                    {
                        producer.m_transientImages.push_back(transientImage);
                        producer.m_imageUsages.push_back(ScopeProducer::ImageUsage{ imageBindingDescs[0], RHI::ScopeAttachmentAccess::ReadWrite });
                    }
                    else if (scopeIdx == imageScopeIntervals[adjustedIndex].m_end)
                    {
                        producer.m_imageUsages.push_back(ScopeProducer::ImageUsage{ imageBindingDescs[1], RHI::ScopeAttachmentAccess::Read });
                    }
                }
            }

            for (uint32_t frameIdx = 0; frameIdx < FrameIterationCount; ++frameIdx)
            {
                frameScheduler.BeginFrame();

                for (AZStd::unique_ptr<ScopeProducer>& producer : m_state->m_producers)
                {
                    frameScheduler.ImportScopeProducer(*producer);
                }

                RHI::FrameSchedulerCompileRequest compileRequest;
                compileRequest.m_jobPolicy = RHI::JobPolicy::Serial;
                frameScheduler.Compile(compileRequest);

                frameScheduler.Execute(RHI::JobPolicy::Serial);

                frameScheduler.EndFrame();
            }

            frameScheduler.Shutdown();
        }

    private:
        static const uint32_t FrameIterationCount = 128;
        static const uint32_t ImportedImageCount = 16;
        static const uint32_t ImportedBufferCount = 16;
        static const uint32_t TransientBufferCount = 16;
        static const uint32_t TransientImageCount = 16;
        static const uint32_t BufferCount = ImportedBufferCount + TransientBufferCount;
        static const uint32_t ImageCount = ImportedImageCount + TransientImageCount;
        static const uint32_t BufferSize = 64;
        static const uint32_t ImageSize = 16;
        static const uint32_t ScopeCount = 16;

        AZStd::unique_ptr<Factory> m_rootFactory;
        AZStd::unique_ptr<AZ::RHI::RHISystem> m_rhiSystem; //! Needed for the TransientAttachmentPool in the FrameScheduler
        RHI::Ptr<RHI::Device> m_device;

        struct State
        {
            RHI::Ptr<RHI::BufferPool> m_bufferPool;
            RHI::Ptr<RHI::ImagePool> m_imagePool;
            ImportedImage m_imageAttachments[ImportedImageCount];
            ImportedBuffer m_bufferAttachments[ImportedBufferCount];
            AZStd::vector<AZStd::unique_ptr<ScopeProducer>> m_producers;
        };

        AZStd::unique_ptr<State> m_state;
    };

    TEST_F(FrameSchedulerTests, Test)
    {
        Test();
    }
}
