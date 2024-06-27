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
#include <Tests/ThreadTester.h>

#include <Atom/RHI/PipelineStateCache.h>

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>

#include <AzCore/Math/Random.h>

namespace UnitTest
{
    using namespace AZ;

    class PipelineStateTests
        : public RHITestFixture
    {
        static const uint32_t s_heapSizeMB = 64;

    protected:
        void ScrambleMemory(void* memory, size_t size, uint32_t seed)
        {
            SimpleLcgRandom random(seed);

            uint8_t* bytes = reinterpret_cast<uint8_t*>(memory);

            for (size_t i = 0; i < size; ++i)
            {
                bytes[i] = static_cast<uint8_t>(random.GetRandom());
            }
        }

        // Generates random render state. Everything else is left empty or default as much as possible,
        // but we do touch-up the data to make sure we don't end up with something that will fail assertions.
        // The point here is to create a unique descriptor that will have a unique hash value.
        RHI::PipelineStateDescriptorForDraw CreatePipelineStateDescriptor(uint32_t randomSeed)
        {
            RHI::PipelineStateDescriptorForDraw desc;
            desc.m_inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleList);
            desc.m_inputStreamLayout.Finalize();
            desc.m_pipelineLayoutDescriptor = m_pipelineLayout;

            ScrambleMemory(&desc.m_renderStates, sizeof(RHI::RenderStates), randomSeed++);

            desc.m_renderStates.m_depthStencilState.m_depth.m_enable = true;
            auto& renderAttachmentLayout = desc.m_renderAttachmentConfiguration.m_renderAttachmentLayout;
            renderAttachmentLayout.m_attachmentCount = 2;
            renderAttachmentLayout.m_attachmentFormats[0] = RHI::Format::R32_FLOAT;
            renderAttachmentLayout.m_attachmentFormats[1] = RHI::Format::R8G8B8A8_SNORM;
            renderAttachmentLayout.m_subpassCount = 1;
            renderAttachmentLayout.m_subpassLayouts[0].m_rendertargetCount = 1;
            renderAttachmentLayout.m_subpassLayouts[0].m_rendertargetDescriptors[0] = RHI::RenderAttachmentDescriptor{ 1, RHI::InvalidRenderAttachmentIndex, RHI::AttachmentLoadStoreAction() };
            renderAttachmentLayout.m_subpassLayouts[0].m_depthStencilDescriptor = RHI::RenderAttachmentDescriptor{ 0, RHI::InvalidRenderAttachmentIndex, RHI::AttachmentLoadStoreAction() };
            desc.m_renderAttachmentConfiguration.m_subpassIndex = 0;
            return desc;
        }

        void ValidateCacheIntegrity(RHI::Ptr<RHI::PipelineStateCache>& cache) const
        {
            cache->ValidateCacheIntegrity();
        }

    private:

        void SetUp() override
        {
            RHITestFixture::SetUp();
            m_factory.reset(aznew Factory());

            m_pipelineLayout = RHI::PipelineLayoutDescriptor::Create();
            m_pipelineLayout->Finalize();
        }

        void TearDown() override
        {
            m_pipelineLayout = nullptr;

            m_factory.reset();
            RHITestFixture::TearDown();
        }

        RHI::Ptr<RHI::PipelineLayoutDescriptor> m_pipelineLayout;
        AZStd::unique_ptr<Factory> m_factory;
    };

    TEST_F(PipelineStateTests, PipelineState_CreateEmpty_Test)
    {
        RHI::Ptr<RHI::DevicePipelineState> empty = RHI::Factory::Get().CreatePipelineState();
        EXPECT_NE(empty.get(), nullptr);
        EXPECT_FALSE(empty->IsInitialized());
    }

    TEST_F(PipelineStateTests, PipelineState_Init_Test)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::DevicePipelineState> pipelineState = RHI::Factory::Get().CreatePipelineState();
        RHI::ResultCode resultCode = pipelineState->Init(*device, CreatePipelineStateDescriptor(0));
        EXPECT_EQ(resultCode, RHI::ResultCode::Success);

        // Second init should fail and throw validation error.
        AZ_TEST_START_ASSERTTEST;
        resultCode = pipelineState->Init(*device, CreatePipelineStateDescriptor(0));
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(PipelineStateTests, PipelineState_Init_Subpass)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::DevicePipelineState> pipelineState = RHI::Factory::Get().CreatePipelineState();
        auto descriptor = CreatePipelineStateDescriptor(0);
        descriptor.m_renderAttachmentConfiguration.m_subpassIndex = 1337;
        AZ_TEST_START_ASSERTTEST;
        RHI::ResultCode resultCode = pipelineState->Init(*device, descriptor);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(PipelineStateTests, PipelineState_Init_SubpassInput)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::DevicePipelineState> pipelineState = RHI::Factory::Get().CreatePipelineState();
        auto descriptor = CreatePipelineStateDescriptor(0);
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0].m_subpassInputCount = 1;
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0].m_subpassInputDescriptors[0].m_attachmentIndex = 1;
        RHI::ResultCode resultCode = pipelineState->Init(*device, descriptor);
        EXPECT_EQ(resultCode, RHI::ResultCode::Success);

        AZ_TEST_START_ASSERTTEST;
        pipelineState = RHI::Factory::Get().CreatePipelineState();
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0]
            .m_subpassInputDescriptors[0]
            .m_attachmentIndex = 3;
        resultCode = pipelineState->Init(*device, descriptor);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(PipelineStateTests, PipelineState_Init_Resolve)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::DevicePipelineState> pipelineState = RHI::Factory::Get().CreatePipelineState();
        auto descriptor = CreatePipelineStateDescriptor(0);
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0].m_rendertargetDescriptors[0].m_resolveAttachmentIndex = 1;
        RHI::ResultCode resultCode = pipelineState->Init(*device, descriptor);
        EXPECT_EQ(resultCode, RHI::ResultCode::Success);

        AZ_TEST_START_ASSERTTEST;
        pipelineState = RHI::Factory::Get().CreatePipelineState();
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0].m_rendertargetDescriptors[0].m_resolveAttachmentIndex = 5;
        resultCode = pipelineState->Init(*device, descriptor);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);

        AZ_TEST_START_ASSERTTEST;
        pipelineState = RHI::Factory::Get().CreatePipelineState();
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0].m_rendertargetDescriptors[0].m_resolveAttachmentIndex = 0;
        resultCode = pipelineState->Init(*device, descriptor);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(PipelineStateTests, PipelineLibrary_CreateEmpty_Test)
    {
        RHI::Ptr<RHI::DevicePipelineLibrary> empty = RHI::Factory::Get().CreatePipelineLibrary();
        EXPECT_NE(empty.get(), nullptr);
        EXPECT_FALSE(empty->IsInitialized());

        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(empty->MergeInto({}), RHI::ResultCode::InvalidOperation);
        EXPECT_EQ(empty->GetSerializedData(), nullptr);
        AZ_TEST_STOP_ASSERTTEST(2);
    }

    TEST_F(PipelineStateTests, PipelineLibrary_Init_Test)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::DevicePipelineLibrary> pipelineLibrary = RHI::Factory::Get().CreatePipelineLibrary();
        RHI::ResultCode resultCode = pipelineLibrary->Init(*device, RHI::DevicePipelineLibraryDescriptor{});
        EXPECT_EQ(resultCode, RHI::ResultCode::Success);

        // Second init should fail and throw validation error.
        AZ_TEST_START_ASSERTTEST;
        resultCode = pipelineLibrary->Init(*device, RHI::DevicePipelineLibraryDescriptor{});
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }
}
