/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"

#include <Tests/Device.h>
#include <Tests/ThreadTester.h>

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI/PipelineLibrary.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/PipelineStateCache.h>

#include <AzCore/Math/Random.h>

namespace UnitTest
{
    using namespace AZ;

    class MultiDevicePipelineStateTests : public MultiDeviceRHITestFixture
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
            renderAttachmentLayout.m_subpassLayouts[0].m_rendertargetDescriptors[0] =
                RHI::RenderAttachmentDescriptor{ 1, RHI::InvalidRenderAttachmentIndex, RHI::AttachmentLoadStoreAction() };
            renderAttachmentLayout.m_subpassLayouts[0].m_depthStencilDescriptor =
                RHI::RenderAttachmentDescriptor{ 0, RHI::InvalidRenderAttachmentIndex, RHI::AttachmentLoadStoreAction() };
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
            MultiDeviceRHITestFixture::SetUp();

            m_pipelineLayout = RHI::PipelineLayoutDescriptor::Create();
            m_pipelineLayout->Finalize();
        }

        void TearDown() override
        {
            m_pipelineLayout = nullptr;
            MultiDeviceRHITestFixture::TearDown();
        }

        RHI::Ptr<RHI::PipelineLayoutDescriptor> m_pipelineLayout;
    };

    TEST_F(MultiDevicePipelineStateTests, PipelineState_CreateEmpty_Test)
    {
        RHI::Ptr<RHI::PipelineState> empty = aznew RHI::PipelineState;
        EXPECT_NE(empty.get(), nullptr);
        EXPECT_FALSE(empty->IsInitialized());
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineState_Init_Test)
    {
        RHI::Ptr<RHI::PipelineState> pipelineState = aznew RHI::PipelineState;
        RHI::ResultCode resultCode = pipelineState->Init(DeviceMask, CreatePipelineStateDescriptor(0));
        EXPECT_EQ(resultCode, RHI::ResultCode::Success);

        // Second init should fail and throw validation error.
        AZ_TEST_START_ASSERTTEST;
        resultCode = pipelineState->Init(DeviceMask, CreatePipelineStateDescriptor(0));
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineState_Init_Subpass)
    {
        RHI::Ptr<RHI::PipelineState> pipelineState = aznew RHI::PipelineState;
        auto descriptor = CreatePipelineStateDescriptor(0);
        descriptor.m_renderAttachmentConfiguration.m_subpassIndex = 1337;
        AZ_TEST_START_ASSERTTEST;
        RHI::ResultCode resultCode = pipelineState->Init(DeviceMask, descriptor);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineState_Init_SubpassInput)
    {
        RHI::Ptr<RHI::PipelineState> pipelineState = aznew RHI::PipelineState;
        auto descriptor = CreatePipelineStateDescriptor(0);
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0].m_subpassInputCount = 1;
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0]
            .m_subpassInputDescriptors[0]
            .m_attachmentIndex = 1;
        RHI::ResultCode resultCode = pipelineState->Init(DeviceMask, descriptor);
        EXPECT_EQ(resultCode, RHI::ResultCode::Success);

        AZ_TEST_START_ASSERTTEST;
        pipelineState = aznew RHI::PipelineState;
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0]
            .m_subpassInputDescriptors[0]
            .m_attachmentIndex = 3;
        resultCode = pipelineState->Init(DeviceMask, descriptor);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineState_Init_Resolve)
    {
        RHI::Ptr<RHI::PipelineState> pipelineState = aznew RHI::PipelineState;
        auto descriptor = CreatePipelineStateDescriptor(0);
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0]
            .m_rendertargetDescriptors[0]
            .m_resolveAttachmentIndex = 1;
        RHI::ResultCode resultCode = pipelineState->Init(DeviceMask, descriptor);
        EXPECT_EQ(resultCode, RHI::ResultCode::Success);

        AZ_TEST_START_ASSERTTEST;
        pipelineState = aznew RHI::PipelineState;
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0]
            .m_rendertargetDescriptors[0]
            .m_resolveAttachmentIndex = 5;
        resultCode = pipelineState->Init(DeviceMask, descriptor);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);

        AZ_TEST_START_ASSERTTEST;
        pipelineState = aznew RHI::PipelineState;
        descriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_subpassLayouts[0]
            .m_rendertargetDescriptors[0]
            .m_resolveAttachmentIndex = 0;
        resultCode = pipelineState->Init(DeviceMask, descriptor);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineLibrary_CreateEmpty_Test)
    {
        RHI::Ptr<RHI::PipelineLibrary> empty = aznew RHI::PipelineLibrary;
        EXPECT_NE(empty.get(), nullptr);
        EXPECT_FALSE(empty->IsInitialized());

        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(empty->MergeInto({}), RHI::ResultCode::InvalidOperation);
        EXPECT_TRUE(empty->GetSerializedDataMap().empty());
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineLibrary_Init_Test)
    {
        RHI::Ptr<RHI::PipelineLibrary> pipelineLibrary = aznew RHI::PipelineLibrary;
        AZ_TEST_START_ASSERTTEST; // Suppress asserts from default constructed library descriptor
        RHI::ResultCode resultCode = pipelineLibrary->Init(DeviceMask, RHI::PipelineLibraryDescriptor{});
        AZ_TEST_STOP_ASSERTTEST(DeviceCount);
        EXPECT_EQ(resultCode, RHI::ResultCode::Success);

        // Second init should fail and throw validation error.
        AZ_TEST_START_ASSERTTEST;
        resultCode = pipelineLibrary->Init(DeviceMask, RHI::PipelineLibraryDescriptor{});
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(resultCode, RHI::ResultCode::InvalidOperation);
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineStateCache_Init_Test)
    {
        RHI::Ptr<RHI::PipelineStateCache> pipelineStateCache =
        RHI::PipelineStateCache::Create(DeviceMask);

        AZStd::array<RHI::PipelineLibraryHandle, RHI::PipelineStateCache::LibraryCountMax> handles;
        for (size_t i = 0; i < handles.size(); ++i)
        {
            handles[i] = pipelineStateCache->CreateLibrary({}, {});

            EXPECT_TRUE(handles[i].IsValid());

            for (size_t j = 0; j < i; ++j)
            {
                EXPECT_NE(handles[i], handles[j]);
            }
        }

        // Creating more than the maximum number of libraries should assert but still function.
        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(pipelineStateCache->CreateLibrary({}, {}), RHI::PipelineLibraryHandle{});
        AZ_TEST_STOP_ASSERTTEST(1);

        // Reset should no-op.
        pipelineStateCache->Reset();

        for (size_t i = 0; i < handles.size(); ++i)
        {
            pipelineStateCache->ResetLibrary(handles[i]);
            pipelineStateCache->ReleaseLibrary(handles[i]);
        }

        // Test free-list by allocating another set of libraries.

        for (size_t i = 0; i < handles.size(); ++i)
        {
            handles[i] = pipelineStateCache->CreateLibrary({}, {});
            EXPECT_FALSE(handles[i].IsNull());
        }
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineStateCache_NullHandle_Test)
    {
        RHI::Ptr<RHI::PipelineStateCache> pipelineStateCache = RHI::PipelineStateCache::Create(DeviceMask);

        // Calling library methods with a null handle should early out.
        pipelineStateCache->ResetLibrary({});
        pipelineStateCache->ReleaseLibrary({});
        EXPECT_EQ(pipelineStateCache->GetMergedLibrary({}), nullptr);
        EXPECT_EQ(pipelineStateCache->AcquirePipelineState({}, CreatePipelineStateDescriptor(0)), nullptr);
        pipelineStateCache->Compact();
        ValidateCacheIntegrity(pipelineStateCache);
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineStateCache_PipelineStateThreading_Same_Test)
    {
        RHI::Ptr<RHI::PipelineStateCache> pipelineStateCache = RHI::PipelineStateCache::Create(DeviceMask);

        static const size_t IterationCountMax = 10000;
        static const size_t ThreadCountMax = 8;

        RHI::PipelineStateDescriptorForDraw descriptor = CreatePipelineStateDescriptor(0);

        RHI::PipelineLibraryHandle libraryHandle = pipelineStateCache->CreateLibrary({}, {});

        AZStd::mutex mutex;
        AZStd::unordered_set<const RHI::PipelineState*> pipelineStatesMerged;

        ThreadTester::Dispatch(
            ThreadCountMax,
            [&]([[maybe_unused]] size_t threadIndex)
            {
                AZStd::unordered_set<const RHI::PipelineState*> pipelineStates;

                for (size_t i = 0; i < IterationCountMax; ++i)
                {
                    pipelineStates.insert(pipelineStateCache->AcquirePipelineState(libraryHandle, descriptor));
                }

                EXPECT_EQ(pipelineStates.size(), 1);
                EXPECT_NE(*pipelineStates.begin(), nullptr);

                mutex.lock();
                pipelineStatesMerged.insert(*pipelineStates.begin());
                mutex.unlock();
            });

        pipelineStateCache->Compact();
        ValidateCacheIntegrity(pipelineStateCache);

        EXPECT_EQ(pipelineStatesMerged.size(), 1);
    }

    TEST_F(MultiDevicePipelineStateTests, PipelineStateCache_PipelineStateThreading_Fuzz_Test)
    {
        RHI::Ptr<RHI::PipelineStateCache> pipelineStateCache = RHI::PipelineStateCache::Create(DeviceMask);

        static const size_t CycleIterationCountMax = 4;
        static const size_t AcquireIterationCountMax = 2000;
        static const size_t ThreadCountMax = 4;
        static const size_t PipelineStateCountMax = 128;
        static const size_t LibraryCountMax = 2;

        AZStd::vector<RHI::PipelineStateDescriptorForDraw> descriptors;
        descriptors.reserve(PipelineStateCountMax);
        for (size_t i = 0; i < PipelineStateCountMax; ++i)
        {
            descriptors.push_back(CreatePipelineStateDescriptor(static_cast<uint32_t>(i)));
        }

        AZStd::vector<RHI::PipelineLibraryHandle> libraryHandles;
        for (size_t i = 0; i < LibraryCountMax; ++i)
        {
            libraryHandles.push_back(pipelineStateCache->CreateLibrary({}, {}));
        }

        AZStd::mutex mutex;

        for (size_t cycleIndex = 0; cycleIndex < CycleIterationCountMax; ++cycleIndex)
        {
            for (size_t libraryIndex = 0; libraryIndex < LibraryCountMax; ++libraryIndex)
            {
                RHI::PipelineLibraryHandle libraryHandle = libraryHandles[libraryIndex];

                AZStd::unordered_set<const RHI::PipelineState*> pipelineStatesMerged;

                ThreadTester::Dispatch(
                    ThreadCountMax,
                    [&](size_t threadIndex)
                    {
                        SimpleLcgRandom random(threadIndex);

                        AZStd::unordered_set<const RHI::PipelineState*> pipelineStates;

                        for (size_t i = 0; i < AcquireIterationCountMax; ++i)
                        {
                            size_t descriptorIndex = random.GetRandom() % descriptors.size();

                            pipelineStates.emplace(pipelineStateCache->AcquirePipelineState(libraryHandle,
                            descriptors[descriptorIndex]));
                        }

                        mutex.lock();
                        for (const RHI::PipelineState* pipelineState : pipelineStates)
                        {
                            pipelineStatesMerged.emplace(pipelineState);
                        }
                        mutex.unlock();
                    });

                EXPECT_EQ(pipelineStatesMerged.size(), PipelineStateCountMax);
            }

            pipelineStateCache->Compact();
            ValidateCacheIntegrity(pipelineStateCache);

            // Halfway through, reset the caches.

            if (cycleIndex == (CycleIterationCountMax / 2))
            {
                pipelineStateCache->Reset();
            }
        }
    }
} // namespace UnitTest
