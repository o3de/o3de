/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <limits>
#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <Tests/Streamer/StreamStackEntryMock.h>

namespace AZ::IO
{
    using MaxConcurrentRequestsType = decltype(IStreamerTypes::Recommendations::m_maxConcurrentRequests);

    template<typename T>
    class StreamStackEntryConformityTestsDescriptor
    {
    public:
        using Type = T;
        virtual ~StreamStackEntryConformityTestsDescriptor() = default;

        virtual void SetUp() {}
        virtual void TearDown() {}

        virtual T CreateInstance() = 0;

        virtual bool UsesSlots() const
        {
            return true;
        }
    };

    template<typename T>
    class StreamStackEntryConformityTests
        : public UnitTest::LeakDetectionFixture
    {
    public:
        using Descriptor = T;
        using Type = typename T::Type;

        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();
            m_description.SetUp();

            m_context = AZStd::make_unique<StreamerContext>();
        }

        void TearDown() override
        {
            m_context.reset();

            m_description.TearDown();
            UnitTest::LeakDetectionFixture::TearDown();
        }

        FileRequest* CreateUnknownRequest()
        {
            FileRequest* request = m_context->GetNewInternalRequest();
            AZStd::any command{ AZStd::string("Unknown request") };
            request->CreateCustom(AZStd::move(command));
            return request;
        }

        T m_description{};
        AZStd::unique_ptr<StreamerContext> m_context;
    };

    TYPED_TEST_CASE_P(StreamStackEntryConformityTests);

    TYPED_TEST_P(StreamStackEntryConformityTests, GetName_RetrieveNameSetOnConstruction_NameIsNotEmpty)
    {
        auto entry = this->m_description.CreateInstance();
        const AZStd::string& name = entry.GetName();
        EXPECT_FALSE(name.empty());
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, Next_SetAndGetNext_NextIsSetAndCanBeRetrieved)
    {
        auto next = AZStd::make_shared<StreamStackEntry>("Next");
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(next);
        auto storedNext = entry.GetNext();
        EXPECT_EQ(next.get(), storedNext.get());
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, SetContext_ContextIsForwardedToNext_SetContextOnMockIsCalled)
    {
        using ::testing::_;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        EXPECT_CALL(*mock, SetContext(_))
            .Times(1)
            .WillOnce([this](StreamerContext& context) { EXPECT_EQ(this->m_context.get(), &context); });

        entry.SetContext(*(this->m_context));
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, PrepareRequest_UnsupportedRequestIsForwarded_RequestIsForwarded)
    {
        using ::testing::_;
        using ::testing::Invoke;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        FileRequest* request = this->CreateUnknownRequest();
        EXPECT_CALL(*mock, PrepareRequest(_))
            .Times(1)
            .WillOnce(Invoke([request](FileRequest* forwarded) { EXPECT_EQ(request, forwarded); }));

        entry.PrepareRequest(request);

        this->m_context->RecycleRequest(request);
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, QueueRequest_UnsupportedRequestIsForwarded_RequestIsForwarded)
    {
        using ::testing::_;
        using ::testing::Invoke;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        FileRequest* request = this->CreateUnknownRequest();
        EXPECT_CALL(*mock, QueueRequest(_))
            .Times(1)
            .WillOnce(Invoke([request](FileRequest* forwarded) { EXPECT_EQ(request, forwarded); }));

        entry.QueueRequest(request);

        this->m_context->RecycleRequest(request);
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, ExecuteRequests_CallIsForwardedToNext_MockReceivedCall)
    {
        using ::testing::_;
        using ::testing::Return;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        EXPECT_CALL(*mock, ExecuteRequests())
            .Times(1)
            .WillOnce(Return(false));

        entry.ExecuteRequests();
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, ExecuteRequests_ForwardsFalseResultFromNext_ReturnsFalse)
    {
        using ::testing::_;
        using ::testing::Return;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        EXPECT_CALL(*mock, ExecuteRequests()).WillRepeatedly(Return(false));

        EXPECT_FALSE(entry.ExecuteRequests());
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, ExecuteRequests_ForwardsTrueResultFromNext_ReturnsTrue)
    {
        using ::testing::_;
        using ::testing::Return;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        EXPECT_CALL(*mock, ExecuteRequests()).WillRepeatedly(Return(true));

        EXPECT_TRUE(entry.ExecuteRequests());
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, UpdateStatus_ForwardsCallToNext_NextRecievedCall)
    {
        using ::testing::_;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        EXPECT_CALL(*mock, UpdateStatus(_))
            .Times(1);

        StreamStackEntry::Status status;
        entry.UpdateStatus(status);
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, UpdateStatus_IsIdleByDefault_StatusReturnsIsIdleWithTrue)
    {
        auto entry = this->m_description.CreateInstance();
        StreamStackEntry::Status status;
        entry.UpdateStatus(status);

        EXPECT_TRUE(status.m_isIdle);
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, UpdateStatus_HasAtLeastOneSlot_ReturnsSlotCountLargerThanOne)
    {
        if (this->m_description.UsesSlots())
        {
            auto entry = this->m_description.CreateInstance();
            StreamStackEntry::Status status;
            entry.UpdateStatus(status);
            EXPECT_GE(status.m_numAvailableSlots, 1);
        }
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, UpdateStatus_InitialValueFitsInRecommendations_NumberOfSlotsSmallEnough)
    {
        if (this->m_description.UsesSlots())
        {
            auto entry = this->m_description.CreateInstance();
            StreamStackEntry::Status status;
            entry.UpdateStatus(status);
            static constexpr size_t MaxRecommendationsRequestsSize = std::numeric_limits<AZ::IO::MaxConcurrentRequestsType>::max();
            EXPECT_LE(status.m_numAvailableSlots, MaxRecommendationsRequestsSize);
        }
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, UpdateStatus_NextHasSmallerNumSlots_ReturnsSmallestNumSlots)
    {
        using ::testing::_;

        if (this->m_description.UsesSlots())
        {
            auto mock = AZStd::make_shared<StreamStackEntryMock>();
            auto entry = this->m_description.CreateInstance();
            entry.SetNext(mock);

            constexpr s32 minValue = std::numeric_limits<s32>::min();
            EXPECT_CALL(*mock, UpdateStatus(_))
                .WillOnce([](StreamStackEntry::Status& status)
                {
                    status.m_numAvailableSlots = minValue;
                });

            StreamStackEntry::Status status;
            entry.UpdateStatus(status);
            EXPECT_EQ(minValue, status.m_numAvailableSlots);
        }
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, UpdateStatus_NextHasLargerNumSlots_ReturnsSmallestNumSlots)
    {
        using ::testing::_;

        if (this->m_description.UsesSlots())
        {
            auto mock = AZStd::make_shared<StreamStackEntryMock>();
            auto entry = this->m_description.CreateInstance();
            entry.SetNext(mock);

            s32 maxValue = std::numeric_limits<s32>::max();
            EXPECT_CALL(*mock, UpdateStatus(_))
                .WillOnce([maxValue](StreamStackEntry::Status& status)
                {
                    status.m_numAvailableSlots = maxValue;
                });

            StreamStackEntry::Status status;
            entry.UpdateStatus(status);
            EXPECT_NE(maxValue, status.m_numAvailableSlots);
            // When nothing is happening there should always be at least one slot available.
            EXPECT_GT(status.m_numAvailableSlots, 0);
        }
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, UpdateCompletionEstimates_ForwardsCallToNext_NextRecievedCall)
    {
        using ::testing::_;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        EXPECT_CALL(*mock, UpdateCompletionEstimates(_, _, _, _)).Times(1);

        auto now = AZStd::chrono::steady_clock::now();
        AZStd::vector<FileRequest*> internalRequests;
        StreamerContext::PreparedQueue pendingRequests;
        entry.UpdateCompletionEstimates(now, internalRequests, pendingRequests.begin(), pendingRequests.end());
    }

    TYPED_TEST_P(StreamStackEntryConformityTests, CollectStatistics_ForwardsCallToNext_NextRecievedCall)
    {
        using ::testing::_;

        auto mock = AZStd::make_shared<StreamStackEntryMock>();
        auto entry = this->m_description.CreateInstance();
        entry.SetNext(mock);

        EXPECT_CALL(*mock, CollectStatistics(_)).Times(1);

        AZStd::vector<Statistic> statistics;
        entry.CollectStatistics(statistics);
    }

    REGISTER_TYPED_TEST_CASE_P(StreamStackEntryConformityTests,
        GetName_RetrieveNameSetOnConstruction_NameIsNotEmpty,
        Next_SetAndGetNext_NextIsSetAndCanBeRetrieved,
        SetContext_ContextIsForwardedToNext_SetContextOnMockIsCalled,
        PrepareRequest_UnsupportedRequestIsForwarded_RequestIsForwarded,
        QueueRequest_UnsupportedRequestIsForwarded_RequestIsForwarded,
        ExecuteRequests_CallIsForwardedToNext_MockReceivedCall,
        ExecuteRequests_ForwardsFalseResultFromNext_ReturnsFalse,
        ExecuteRequests_ForwardsTrueResultFromNext_ReturnsTrue,
        UpdateStatus_ForwardsCallToNext_NextRecievedCall,
        UpdateStatus_IsIdleByDefault_StatusReturnsIsIdleWithTrue,
        UpdateStatus_HasAtLeastOneSlot_ReturnsSlotCountLargerThanOne,
        UpdateStatus_InitialValueFitsInRecommendations_NumberOfSlotsSmallEnough,
        UpdateStatus_NextHasSmallerNumSlots_ReturnsSmallestNumSlots,
        UpdateStatus_NextHasLargerNumSlots_ReturnsSmallestNumSlots,
        UpdateCompletionEstimates_ForwardsCallToNext_NextRecievedCall,
        CollectStatistics_ForwardsCallToNext_NextRecievedCall);
} // namespace AZ::IO
