/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <AzCore/Debug/Timer.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <Tests/Device.h>

#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/ResourceInvalidateBus.h>

#include <Tests/Buffer.h>

namespace UnitTest
{
    using namespace AZ;

    class MultiDeviceBufferTests : public MultiDeviceRHITestFixture
    {
    public:
        MultiDeviceBufferTests()
            : MultiDeviceRHITestFixture()
        {
        }

        void SetUp() override
        {
            MultiDeviceRHITestFixture::SetUp();
        }

        void TearDown() override
        {
            MultiDeviceRHITestFixture::TearDown();
        }
    };

    TEST_F(MultiDeviceBufferTests, TestNoop)
    {
        RHI::Ptr<RHI::Buffer> noopBuffer;
        noopBuffer = aznew RHI::Buffer;
    }

    TEST_F(MultiDeviceBufferTests, TestAll)
    {
        RHI::Ptr<RHI::Buffer> bufferA;
        bufferA = aznew RHI::Buffer;

        bufferA->SetName(Name("BufferA"));
        AZ_TEST_ASSERT(bufferA->GetName().GetStringView() == "BufferA");
        AZ_TEST_ASSERT(bufferA->use_count() == 1);

        {
            RHI::Ptr<AZ::RHI::BufferPool> bufferPool;
            bufferPool = aznew AZ::RHI::BufferPool;

            AZ_TEST_ASSERT(bufferPool->use_count() == 1);

            RHI::Ptr<RHI::Buffer> bufferB;
            bufferB = aznew RHI::Buffer;
            AZ_TEST_ASSERT(bufferB->use_count() == 1);

            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::Constant;
            bufferPool->Init(bufferPoolDesc);

            AZStd::vector<uint8_t> testData(32);
            for (uint32_t i = 0; i < 32; ++i)
            {
                testData[i] = (uint8_t)i * 2;
            }

            AZ_TEST_ASSERT(bufferA->IsInitialized() == false);
            AZ_TEST_ASSERT(bufferB->IsInitialized() == false);

            RHI::BufferInitRequest initRequest;
            initRequest.m_buffer = bufferA.get();
            initRequest.m_descriptor = RHI::BufferDescriptor(RHI::BufferBindFlags::Constant, 32);
            initRequest.m_initialData = testData.data();
            bufferPool->InitBuffer(initRequest);

            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                RHI::Ptr<RHI::DeviceBufferView> bufferView;
                bufferView = bufferA->GetDeviceBuffer(deviceIndex)->GetBufferView(RHI::BufferViewDescriptor::CreateRaw(0, 32));

                AZ_TEST_ASSERT(bufferView->IsInitialized());
                ASSERT_FALSE(bufferView->IsStale());
                AZ_TEST_ASSERT(bufferView->IsFullView());
                AZ_TEST_ASSERT(bufferA->GetDeviceBuffer(deviceIndex)->use_count() == 3);
            }

            // MDBuffer still only used once
            AZ_TEST_ASSERT(bufferA->use_count() == 1);
            AZ_TEST_ASSERT(bufferA->IsInitialized());

            initRequest.m_buffer = bufferB.get();
            initRequest.m_descriptor = RHI::BufferDescriptor(RHI::BufferBindFlags::Constant, 16);
            initRequest.m_initialData = testData.data() + 16;
            bufferPool->InitBuffer(initRequest);

            AZ_TEST_ASSERT(bufferB->IsInitialized());

            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                const auto& bufferAData{static_cast<Buffer*>(bufferA->GetDeviceBuffer(deviceIndex).get())->GetData()};
                AZ_TEST_ASSERT(AZStd::equal(testData.begin(), testData.end(), bufferAData.begin()));

                const auto& bufferBData{static_cast<Buffer*>(bufferB->GetDeviceBuffer(deviceIndex).get())->GetData()};
                AZ_TEST_ASSERT(AZStd::equal(testData.begin() + 16, testData.end(), bufferBData.begin()));
            }

            AZ_TEST_ASSERT(bufferA->GetPool() == bufferPool.get());
            AZ_TEST_ASSERT(bufferB->GetPool() == bufferPool.get());
            AZ_TEST_ASSERT(bufferPool->GetResourceCount() == 2);

            {
                uint32_t bufferIndex = 0;

                const RHI::Buffer* buffers[] =
                {
                    bufferA.get(),
                    bufferB.get()
                };

                bufferPool->ForEach<RHI::Buffer>([&bufferIndex, &buffers]([[maybe_unused]] RHI::Buffer& buffer)
                {
                    AZ_UNUSED(buffers); // Prevent unused warning in release builds
                    AZ_Assert(buffers[bufferIndex] == &buffer, "buffers don't match");
                    bufferIndex++;
                });
            }

            bufferB->Shutdown();
            AZ_TEST_ASSERT(bufferB->GetPool() == nullptr);

            RHI::Ptr<AZ::RHI::BufferPool> bufferPoolB;
            bufferPoolB = aznew AZ::RHI::BufferPool;
            bufferPoolB->Init(bufferPoolDesc);

            initRequest.m_buffer = bufferB.get();
            initRequest.m_descriptor = RHI::BufferDescriptor(RHI::BufferBindFlags::Constant, 16);
            initRequest.m_initialData = testData.data() + 16;
            bufferPoolB->InitBuffer(initRequest);
            AZ_TEST_ASSERT(bufferB->GetPool() == bufferPoolB.get());

            //Since we are switching bufferpools for bufferB it adds a refcount and invalidates the views.
            //We need this to ensure the views are fully invalidated in order to release the refcount and avoid a leak.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

            bufferPoolB->Shutdown();
            AZ_TEST_ASSERT(bufferPoolB->GetResourceCount() == 0);
        }

        AZ_TEST_ASSERT(bufferA->GetPool() == nullptr);
        AZ_TEST_ASSERT(bufferA->use_count() == 1);
    }

    TEST_F(MultiDeviceBufferTests, TestViews)
    {
        AZStd::vector<RHI::Ptr<RHI::DeviceBufferView>> bufferViewsA(DeviceCount);

        {
            RHI::Ptr<RHI::BufferPool> bufferPool;
            bufferPool = aznew RHI::BufferPool;

            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::Constant;
            bufferPool->Init(bufferPoolDesc);

            RHI::Ptr<RHI::Buffer> buffer;
            buffer = aznew RHI::Buffer;

            RHI::BufferInitRequest initRequest;
            initRequest.m_buffer = buffer.get();
            initRequest.m_descriptor = RHI::BufferDescriptor(RHI::BufferBindFlags::Constant, 32);
            bufferPool->InitBuffer(initRequest);

            // Should report initialized and not stale.
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                bufferViewsA[deviceIndex] = buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(RHI::BufferViewDescriptor::CreateRaw(0, 32));
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsInitialized());
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsStale() == false);
            }

            // Should report as still initialized and also stale.
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                buffer->GetDeviceBuffer(deviceIndex)->Shutdown();
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsInitialized());
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsStale());
            }
            buffer->Shutdown();

            bufferPool->InitBuffer(initRequest);

            // Make sure that the buffer doesn't expect an invalidation event.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

            // We need to recreate device views since device buffers are recreated after Shutdown.
            for (auto deviceIndex{ 0 }; deviceIndex < DeviceCount; ++deviceIndex)
            {
                bufferViewsA[deviceIndex] =
                    buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(RHI::BufferViewDescriptor::CreateRaw(0, 32));
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsInitialized());
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsStale() == false);
            }

            // Explicit invalidation should mark it stale.
            buffer->InvalidateViews();
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsInitialized());
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsStale());
            }

            // This should re-initialize the views.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
            for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
            {
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsInitialized());
                AZ_TEST_ASSERT(bufferViewsA[deviceIndex]->IsStale() == false);
            }

            // Create an uninitialized bufferview and let it go out of scope
            RHI::Ptr<RHI::DeviceBufferView> uninitializedBufferViewPtr = RHI::Factory::Get().CreateBufferView();
        }
    }


    struct MultiDeviceBufferAndViewBindFlags
    {
        RHI::BufferBindFlags bufferBindFlags;
        RHI::BufferBindFlags viewBindFlags;
    };

    class MultiDeviceBufferBindFlagTests
        : public MultiDeviceBufferTests
        , public ::testing::WithParamInterface <MultiDeviceBufferAndViewBindFlags>
    {
    public:
        void SetUp() override
        {
            MultiDeviceBufferTests::SetUp();

            // Create a pool and buffer with the buffer bind flags from the parameterized test
            m_bufferPool = aznew AZ::RHI::BufferPool;
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_bindFlags = GetParam().bufferBindFlags;
            m_bufferPool->Init(bufferPoolDesc);

            m_buffer = aznew RHI::Buffer;
            RHI::BufferInitRequest initRequest;
            initRequest.m_buffer = m_buffer.get();
            initRequest.m_descriptor = RHI::BufferDescriptor(GetParam().bufferBindFlags, 32);
            m_bufferPool->InitBuffer(initRequest);
        }
    
        void TearDown() override
        {
            m_bufferPool.reset();
            m_buffer.reset();
            m_bufferView.reset();
            MultiDeviceBufferTests::TearDown();
        }

        RHI::Ptr<RHI::BufferPool> m_bufferPool;
        RHI::Ptr<RHI::Buffer> m_buffer;
        RHI::Ptr<RHI::DeviceBufferView> m_bufferView;
    };

    TEST_P(MultiDeviceBufferBindFlagTests, InitView_ViewIsCreated)
    {
        RHI::BufferViewDescriptor bufferViewDescriptor;
        bufferViewDescriptor.m_overrideBindFlags = GetParam().viewBindFlags;
        for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
        {
            m_bufferView = m_buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(bufferViewDescriptor);
            EXPECT_EQ(m_bufferView.get()!=nullptr, true);
        }
    }

    // This test fixture is the same as MultiDeviceBufferBindFlagTests, but exists separately so that
    // we can instantiate different test cases that are expected to fail
    class MultiDeviceBufferBindFlagFailureCases
        : public MultiDeviceBufferBindFlagTests
    {

    };

    TEST_P(MultiDeviceBufferBindFlagFailureCases, InitView_ViewIsNotCreated)
    {
        RHI::BufferViewDescriptor bufferViewDescriptor;
        bufferViewDescriptor.m_overrideBindFlags = GetParam().viewBindFlags;
        for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
        {
            m_bufferView = m_buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(bufferViewDescriptor);
            EXPECT_EQ(m_bufferView.get()==nullptr, true);
        }
    }

        // These combinations should result in a successful creation of the buffer view
    std::vector<MultiDeviceBufferAndViewBindFlags> GenerateCompatibleMultiDeviceBufferBindFlagCombinations()
    {
        std::vector<MultiDeviceBufferAndViewBindFlags> testCases;
        MultiDeviceBufferAndViewBindFlags flags;

        // When the buffer bind flags are equal to or a superset of the buffer view bind flags, the view is compatible with the buffer
        flags.bufferBindFlags = RHI::BufferBindFlags::Constant;
        flags.viewBindFlags = RHI::BufferBindFlags::Constant;
        testCases.push_back(flags); 
        
        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderReadWrite;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderReadWrite;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderWrite;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderReadWrite;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderReadWrite;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderRead;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderWrite;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderWrite;
        testCases.push_back(flags);

        // When the buffer view bind flags are None, they have no effect and should work with any bind flag used by the buffer
        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderRead;
        flags.viewBindFlags = RHI::BufferBindFlags::None;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderWrite;
        flags.viewBindFlags = RHI::BufferBindFlags::None;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderReadWrite;
        flags.viewBindFlags = RHI::BufferBindFlags::None;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::None;
        flags.viewBindFlags = RHI::BufferBindFlags::None;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::Constant;
        flags.viewBindFlags = RHI::BufferBindFlags::None;
        testCases.push_back(flags);

        return testCases;
    };

    // These combinations should fail during BufferView::Init
    std::vector<MultiDeviceBufferAndViewBindFlags> GenerateIncompatibleMultiDeviceBufferBindFlagCombinations()
    {
        std::vector<MultiDeviceBufferAndViewBindFlags> testCases;
        MultiDeviceBufferAndViewBindFlags flags;

        flags.bufferBindFlags = RHI::BufferBindFlags::Constant;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderRead;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderWrite;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderRead;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderReadWrite;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderWrite;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::ShaderWrite;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderReadWrite;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::None;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderRead;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::None;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderWrite;
        testCases.push_back(flags);

        flags.bufferBindFlags = RHI::BufferBindFlags::None;
        flags.viewBindFlags = RHI::BufferBindFlags::ShaderReadWrite;
        testCases.push_back(flags);

        return testCases;
    }

    std::string MultiDeviceBufferBindFlagsToString(RHI::BufferBindFlags bindFlags)
    {
        switch (bindFlags)
        {
        case RHI::BufferBindFlags::None:
            return "None";
        case RHI::BufferBindFlags::Constant:
            return "Constant";
        case RHI::BufferBindFlags::ShaderRead:
            return "ShaderRead";
        case RHI::BufferBindFlags::ShaderWrite:
            return "ShaderWrite";
        case RHI::BufferBindFlags::ShaderReadWrite:
            return "ShaderReadWrite";
        default:
            AZ_Assert(false, "No string conversion was created for this bind flag setting.")
            break;
        }

        return "";
    }

    std::string GenerateMultiDeviceBufferBindFlagTestCaseName(const ::testing::TestParamInfo<MultiDeviceBufferAndViewBindFlags>& info)
    {
        return MultiDeviceBufferBindFlagsToString(info.param.bufferBindFlags) + "BufferWith" + MultiDeviceBufferBindFlagsToString(info.param.viewBindFlags) + "BufferView";
    }

    INSTANTIATE_TEST_CASE_P(BufferView, MultiDeviceBufferBindFlagTests, ::testing::ValuesIn(GenerateCompatibleMultiDeviceBufferBindFlagCombinations()), GenerateMultiDeviceBufferBindFlagTestCaseName);
    INSTANTIATE_TEST_CASE_P(BufferView, MultiDeviceBufferBindFlagFailureCases, ::testing::ValuesIn(GenerateIncompatibleMultiDeviceBufferBindFlagCombinations()), GenerateMultiDeviceBufferBindFlagTestCaseName);

    enum class MultiDeviceParallelGetBufferViewTestCases
    {
        Get,
        GetAndDeferRemoval,
        GetCreateAndDeferRemoval
    };

    enum class MultiDeviceParallelGetBufferViewCurrentAction
    {
        Get,
        Create,
        DeferredRemoval
    };

    MultiDeviceParallelGetBufferViewCurrentAction ParallelBufferViewGetCurrentAction(const MultiDeviceParallelGetBufferViewTestCases& testCase)
    {
        switch (testCase)
        {
        case MultiDeviceParallelGetBufferViewTestCases::GetAndDeferRemoval:
            switch (rand() % 2)
            {
            case 0:
                return MultiDeviceParallelGetBufferViewCurrentAction::Get;
            case 1:
                return MultiDeviceParallelGetBufferViewCurrentAction::DeferredRemoval;
            }
        case MultiDeviceParallelGetBufferViewTestCases::GetCreateAndDeferRemoval:
            switch (rand() % 3)
            {
            case 0:
                return MultiDeviceParallelGetBufferViewCurrentAction::Get;
            case 1:
                return MultiDeviceParallelGetBufferViewCurrentAction::Create;
            case 2:
                return MultiDeviceParallelGetBufferViewCurrentAction::DeferredRemoval;
            }
        case MultiDeviceParallelGetBufferViewTestCases::Get:
        default:
            return MultiDeviceParallelGetBufferViewCurrentAction::Get;
        }
    }

    void ParallelGetBufferViewHelper(
        const size_t& threadCountMax,
        const uint32_t& bufferViewCount,
        const uint32_t& iterations,
        const MultiDeviceParallelGetBufferViewTestCases& testCase)
    {
        // printf("Testing threads=%zu assetIds=%zu ... ", threadCountMax, assetIdCount);

        AZ::Debug::Timer timer;
        timer.Stamp();

        // Create the buffer
        constexpr uint32_t viewSize = 32;
        constexpr uint32_t maxBufferViewCount = 100;
        constexpr uint32_t bufferSize = viewSize * maxBufferViewCount;
            
        AZ_Assert(
            maxBufferViewCount >= bufferViewCount,
            "This test uses offsets/sizes to create unique BufferViewDescriptors. Ensure the buffer size is large enough to handle the "
            "number of unique buffer views.");

        RHI::Ptr<RHI::BufferPool> bufferPool;
        bufferPool = aznew AZ::RHI::BufferPool;

        RHI::BufferPoolDescriptor bufferPoolDesc;
        bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::Constant;
        bufferPool->Init(bufferPoolDesc);

        RHI::Ptr<RHI::Buffer> buffer;
        buffer = aznew RHI::Buffer;

        RHI::BufferInitRequest initRequest;
        initRequest.m_buffer = buffer.get();
        initRequest.m_descriptor = RHI::BufferDescriptor(RHI::BufferBindFlags::Constant, bufferSize);
        bufferPool->InitBuffer(initRequest);

        AZStd::vector<RHI::BufferViewDescriptor> viewDescriptors;
        viewDescriptors.reserve(bufferViewCount);
        for (uint32_t i = 0; i < bufferViewCount; ++i)
        {
            viewDescriptors.push_back(RHI::BufferViewDescriptor::CreateRaw(i * viewSize, viewSize));
        }

        AZStd::vector<AZStd::vector<RHI::Ptr<RHI::DeviceBufferView>>> referenceTable(bufferViewCount);

        AZStd::vector<AZStd::thread> threads;
        AZStd::mutex mutex;
        AZStd::mutex referenceTableMutex;
        AZStd::atomic<int> threadCount((int)threadCountMax);
        AZStd::condition_variable cv;

        for (size_t i = 0; i < threadCountMax; ++i)
        {
            threads.emplace_back(
                [&threadCount, &cv, &buffer, &viewDescriptors, &referenceTable, &iterations, &testCase, &referenceTableMutex]()
                {
                    bool deferRemoval = testCase == MultiDeviceParallelGetBufferViewTestCases::GetAndDeferRemoval ||
                        testCase == MultiDeviceParallelGetBufferViewTestCases::GetCreateAndDeferRemoval;

                    for (uint32_t i = 0; i < iterations; ++i) // queue up a bunch of work
                    {
                        // Pick a random buffer view from a random device to deal with
                        const size_t index = rand() % viewDescriptors.size();
                        const int deviceIndex = rand() % DeviceCount;
                        const RHI::BufferViewDescriptor& viewDescriptor = viewDescriptors[index];

                        MultiDeviceParallelGetBufferViewCurrentAction currentAction = ParallelBufferViewGetCurrentAction(testCase);

                        if (currentAction == MultiDeviceParallelGetBufferViewCurrentAction::Get ||
                            currentAction == MultiDeviceParallelGetBufferViewCurrentAction::Create)
                        {
                            RHI::Ptr<RHI::DeviceBufferView> ptr = nullptr;
                            if (currentAction == MultiDeviceParallelGetBufferViewCurrentAction::Get)
                            {
                                ptr = buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(viewDescriptor);
                                EXPECT_EQ(ptr->GetDescriptor(), viewDescriptor);
                            }
                            else if (currentAction == MultiDeviceParallelGetBufferViewCurrentAction::Create)
                            {
                                ptr = RHI::Factory::Get().CreateBufferView();
                                // Only initialize half of the created references to validated
                                // that uninitialized views are also threadsafe
                                if (rand() % 2)
                                {
                                    RHI::ResultCode resultCode = ptr->Init(static_cast<const Buffer&>(*buffer->GetDeviceBuffer(deviceIndex)), viewDescriptor);
                                    EXPECT_EQ(resultCode, RHI::ResultCode::Success);
                                    EXPECT_EQ(ptr->GetDescriptor(), viewDescriptor);
                                }
                            }

                            // Validate the new reference
                            EXPECT_NE(ptr, nullptr);

                            if (deferRemoval)
                            {
                                // If this test case includes deferring the removal,
                                // keep a reference to the instance alive so it can be removed later
                                referenceTableMutex.lock();
                                referenceTable[index].push_back(ptr);
                                referenceTableMutex.unlock();
                            }
                        }
                        else if (currentAction == MultiDeviceParallelGetBufferViewCurrentAction::DeferredRemoval)
                        {
                            // Drop the refcount to zero so the instance will be released
                            referenceTableMutex.lock();
                            referenceTable[index].clear();
                            referenceTableMutex.unlock();
                        }
                    }

                    threadCount--;
                    cv.notify_one();
                });
        }

        bool timedOut = false;

        // Used to detect a deadlock.  If we wait for more than 10 seconds, it's likely a deadlock has occurred
        while (threadCount > 0 && !timedOut)
        {
            AZStd::unique_lock<AZStd::mutex> lock(mutex);
            timedOut =
                (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::steady_clock::now() + AZStd::chrono::seconds(1)));
        }

        EXPECT_TRUE(threadCount == 0) << "One or more threads appear to be deadlocked at " << timer.GetDeltaTimeInSeconds()
                                      << " seconds";

        for (auto& thread : threads)
        {
            thread.join();
        }

        // printf("Took %f seconds\n", timer.GetDeltaTimeInSeconds());
    }

    void ParallelGetBufferViewTest(const MultiDeviceParallelGetBufferViewTestCases& testCase)
    {
        // This is the original test scenario from when InstanceDatabase was first implemented
        //                           threads, bufferViews,  seconds
        ParallelGetBufferViewHelper(8, 100, 5, testCase);

        // This value is checked in as 1 so this test doesn't take too much time, but can be increased locally to soak the test.
        const size_t attempts = 1;

        for (size_t i = 0; i < attempts; ++i)
        {
            // printf("Attempt %zu of %zu... \n", i, attempts);

            // The idea behind this series of tests is that there are two threads sharing one bufferView, and both threads try to
            // create or release that view at the same time.
            const uint32_t iterations = 1000;
            //                           threads, AssetIds, iterations
            ParallelGetBufferViewHelper(2, 1, iterations, testCase);
            ParallelGetBufferViewHelper(4, 1, iterations, testCase);
            ParallelGetBufferViewHelper(8, 1, iterations, testCase);
            // printf("Attempt %zu of %zu... \n", i, attempts);

            // Here we try a bunch of different threadCount:bufferViewCount ratios to be thorough
            //                           threads, views, iterations
            ParallelGetBufferViewHelper(2, 1, iterations, testCase);
            ParallelGetBufferViewHelper(4, 1, iterations, testCase);
            ParallelGetBufferViewHelper(4, 2, iterations, testCase);
            ParallelGetBufferViewHelper(4, 4, iterations, testCase);
            ParallelGetBufferViewHelper(8, 1, iterations, testCase);
            ParallelGetBufferViewHelper(8, 2, iterations, testCase);
            ParallelGetBufferViewHelper(8, 3, iterations, testCase);
            ParallelGetBufferViewHelper(8, 4, iterations, testCase);
        }
    }

    TEST_F(MultiDeviceBufferTests, DISABLED_ParallelGetBufferViewTests_Get)
    {
        ParallelGetBufferViewTest(MultiDeviceParallelGetBufferViewTestCases::Get);
    }

    TEST_F(MultiDeviceBufferTests, DISABLED_ParallelGetBufferViewTests_GetAndDeferRemoval)
    {
        ParallelGetBufferViewTest(MultiDeviceParallelGetBufferViewTestCases::GetAndDeferRemoval);
    }

    TEST_F(MultiDeviceBufferTests, DISABLED_ParallelGetBufferViewTests_GetCreateAndDeferRemoval)
    {
        ParallelGetBufferViewTest(MultiDeviceParallelGetBufferViewTestCases::GetCreateAndDeferRemoval);
    }
} // namespace UnitTest
