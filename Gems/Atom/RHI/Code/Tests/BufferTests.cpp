/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Tests/Factory.h>
#include <Tests/Device.h>

using namespace AZ;

namespace UnitTest
{
    class BufferTests
        : public RHITestFixture
    {
    public:
        BufferTests()
            : RHITestFixture()
        {}

        void SetUp() override
        {
            RHITestFixture::SetUp();
            m_factory.reset(aznew Factory());
        }

        void TearDown() override
        {
            m_factory.reset();
            RHITestFixture::TearDown();
        }

    private:

        AZStd::unique_ptr<Factory> m_factory;
    };

    TEST_F(BufferTests, TestNoop)
    {
        RHI::Ptr<RHI::Buffer> noopBuffer;
        noopBuffer = RHI::Factory::Get().CreateBuffer();
    }

    TEST_F(BufferTests, Test)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::Buffer> bufferA;
        bufferA = RHI::Factory::Get().CreateBuffer();

        bufferA->SetName(Name("BufferA"));
        AZ_TEST_ASSERT(bufferA->GetName().GetStringView() == "BufferA");
        AZ_TEST_ASSERT(bufferA->use_count() == 1);

        {
            RHI::Ptr<RHI::BufferPool> bufferPool;
            bufferPool = RHI::Factory::Get().CreateBufferPool();

            AZ_TEST_ASSERT(bufferPool->use_count() == 1);

            RHI::Ptr<RHI::Buffer> bufferB;
            bufferB = RHI::Factory::Get().CreateBuffer();
            AZ_TEST_ASSERT(bufferB->use_count() == 1);

            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::Constant;
            bufferPool->Init(*device, bufferPoolDesc);

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

            RHI::Ptr<RHI::BufferView> bufferView;
            bufferView = bufferA->GetBufferView(RHI::BufferViewDescriptor::CreateRaw(0, 32));
            
            AZ_TEST_ASSERT(bufferView->IsInitialized());
            ASSERT_FALSE(bufferView->IsStale());
            AZ_TEST_ASSERT(bufferView->IsFullView());

            AZ_TEST_ASSERT(bufferA->use_count() == 2);
            AZ_TEST_ASSERT(bufferA->IsInitialized());
            AZ_TEST_ASSERT(static_cast<Buffer&>(*bufferA).IsMapped() == false);

            initRequest.m_buffer = bufferB.get();
            initRequest.m_descriptor = RHI::BufferDescriptor(RHI::BufferBindFlags::Constant, 16);
            initRequest.m_initialData = testData.data() + 16;
            bufferPool->InitBuffer(initRequest);

            AZ_TEST_ASSERT(bufferB->IsInitialized());

            AZ_TEST_ASSERT(AZStd::equal(testData.begin(), testData.end(), static_cast<Buffer&>(*bufferA).GetData().begin()));
            AZ_TEST_ASSERT(AZStd::equal(testData.begin() + 16, testData.end(), static_cast<Buffer&>(*bufferB).GetData().begin()));

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
                    AZ_Assert(buffers[bufferIndex] == &buffer, "buffers don't match");
                    bufferIndex++;
                });
            }

            bufferB->Shutdown();
            AZ_TEST_ASSERT(bufferB->GetPool() == nullptr);

            RHI::Ptr<RHI::BufferPool> bufferPoolB;
            bufferPoolB = RHI::Factory::Get().CreateBufferPool();
            bufferPoolB->Init(*device, bufferPoolDesc);

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

    TEST_F(BufferTests, TestViews)
    {
        RHI::Ptr<RHI::Device> device = MakeTestDevice();

        RHI::Ptr<RHI::BufferView> bufferViewA;
        
        {
            RHI::Ptr<RHI::BufferPool> bufferPool;
            bufferPool = RHI::Factory::Get().CreateBufferPool();

            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::Constant;
            bufferPool->Init(*device, bufferPoolDesc);

            RHI::Ptr<RHI::Buffer> buffer;
            buffer = RHI::Factory::Get().CreateBuffer();

            RHI::BufferInitRequest initRequest;
            initRequest.m_buffer = buffer.get();
            initRequest.m_descriptor = RHI::BufferDescriptor(RHI::BufferBindFlags::Constant, 32);
            bufferPool->InitBuffer(initRequest);

            // Should report initialized and not stale.
            bufferViewA = buffer->GetBufferView(RHI::BufferViewDescriptor::CreateRaw(0, 32));
            
            AZ_TEST_ASSERT(bufferViewA->IsInitialized());
            AZ_TEST_ASSERT(bufferViewA->IsStale() == false);

            // Should report as still initialized and also stale.
            buffer->Shutdown();
            AZ_TEST_ASSERT(bufferViewA->IsInitialized());
            AZ_TEST_ASSERT(bufferViewA->IsStale());

            // Should *still* report as stale since resource invalidation events are queued.
            bufferPool->InitBuffer(initRequest);
            AZ_TEST_ASSERT(bufferViewA->IsInitialized());
            AZ_TEST_ASSERT(bufferViewA->IsStale());

            // This should re-initialize the views.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
            AZ_TEST_ASSERT(bufferViewA->IsInitialized());
            AZ_TEST_ASSERT(bufferViewA->IsStale() == false);

            // Explicit invalidation should mark it stale.
            buffer->InvalidateViews();
            AZ_TEST_ASSERT(bufferViewA->IsInitialized());
            AZ_TEST_ASSERT(bufferViewA->IsStale());

            // This should re-initialize the views.
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
            AZ_TEST_ASSERT(bufferViewA->IsInitialized());
            AZ_TEST_ASSERT(bufferViewA->IsStale() == false);
        }
    }

    struct BufferAndViewBindFlags
    {
        RHI::BufferBindFlags bufferBindFlags;
        RHI::BufferBindFlags viewBindFlags;
    };

    class BufferBindFlagTests
        : public BufferTests
        , public ::testing::WithParamInterface <BufferAndViewBindFlags>
    {
    public:
        void SetUp() override
        {
            BufferTests::SetUp();

            m_device = MakeTestDevice();

            // Create a pool and buffer with the buffer bind flags from the parameterized test
            m_bufferPool = RHI::Factory::Get().CreateBufferPool();
            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_bindFlags = GetParam().bufferBindFlags;
            m_bufferPool->Init(*m_device, bufferPoolDesc);

            m_buffer = RHI::Factory::Get().CreateBuffer();
            RHI::BufferInitRequest initRequest;
            initRequest.m_buffer = m_buffer.get();
            initRequest.m_descriptor = RHI::BufferDescriptor(GetParam().bufferBindFlags, 32);
            m_bufferPool->InitBuffer(initRequest);
       }

        RHI::Ptr<RHI::Device> m_device; 
        RHI::Ptr<RHI::BufferPool> m_bufferPool;
        RHI::Ptr<RHI::Buffer> m_buffer;
        RHI::Ptr<RHI::BufferView> m_bufferView;
    };

    TEST_P(BufferBindFlagTests, InitView_ViewIsCreated)
    {
        RHI::BufferViewDescriptor bufferViewDescriptor;
        bufferViewDescriptor.m_overrideBindFlags = GetParam().viewBindFlags;
        m_bufferView = m_buffer->GetBufferView(bufferViewDescriptor);
        EXPECT_EQ(m_bufferView.get()!=nullptr, true);
    }


    // This test fixture is the same as BufferBindFlagTests, but exists separately so that
    // we can instantiate different test cases that are expected to fail
    class BufferBindFlagFailureCases
        : public BufferBindFlagTests
    {

    };

    TEST_P(BufferBindFlagFailureCases, InitView_ViewIsNotCreated)
    {
        RHI::BufferViewDescriptor bufferViewDescriptor;
        bufferViewDescriptor.m_overrideBindFlags = GetParam().viewBindFlags;
        m_bufferView = m_buffer->GetBufferView(bufferViewDescriptor);
        EXPECT_EQ(m_bufferView.get()==nullptr, true);
    }

    // These combinations should result in a successful creation of the buffer view
    std::vector<BufferAndViewBindFlags> GenerateCompatibleBufferBindFlagCombinations()
    {
        std::vector<BufferAndViewBindFlags> testCases;
        BufferAndViewBindFlags flags;

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
    std::vector<BufferAndViewBindFlags> GenerateIncompatibleBufferBindFlagCombinations()
    {
        std::vector<BufferAndViewBindFlags> testCases;
        BufferAndViewBindFlags flags;

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

    std::string BufferBindFlagsToString(RHI::BufferBindFlags bindFlags)
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

    std::string GenerateBufferBindFlagTestCaseName(const ::testing::TestParamInfo<BufferAndViewBindFlags>& info)
    {
        return BufferBindFlagsToString(info.param.bufferBindFlags) + "BufferWith" + BufferBindFlagsToString(info.param.viewBindFlags) + "BufferView";
    }

    INSTANTIATE_TEST_CASE_P(BufferView, BufferBindFlagTests, ::testing::ValuesIn(GenerateCompatibleBufferBindFlagCombinations()), GenerateBufferBindFlagTestCaseName);
    INSTANTIATE_TEST_CASE_P(BufferView, BufferBindFlagFailureCases, ::testing::ValuesIn(GenerateIncompatibleBufferBindFlagCombinations()), GenerateBufferBindFlagTestCaseName);
}
