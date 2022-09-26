/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <gmock/gmock.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/Streamer/FileRequest.h>

using namespace AZ::IO;

class StreamerMock : public AZ::IO::IStreamer
{
public:

    MOCK_METHOD7(Read, FileRequestPtr(AZStd::string_view, void*, size_t, size_t,
        AZStd::chrono::microseconds, IStreamerTypes::Priority, size_t));
    MOCK_METHOD8(Read, FileRequestPtr& (FileRequestPtr&, AZStd::string_view, void*, size_t, size_t,
        AZStd::chrono::microseconds, IStreamerTypes::Priority, size_t));
    MOCK_METHOD6(Read, FileRequestPtr(AZStd::string_view, IStreamerTypes::RequestMemoryAllocator&,
        size_t, AZStd::chrono::microseconds, IStreamerTypes::Priority, size_t));
    MOCK_METHOD7(Read, FileRequestPtr& (FileRequestPtr&, AZStd::string_view, IStreamerTypes::RequestMemoryAllocator&,
        size_t, AZStd::chrono::microseconds, IStreamerTypes::Priority, size_t));
    MOCK_METHOD1(Cancel, FileRequestPtr(FileRequestPtr));
    MOCK_METHOD2(Cancel, FileRequestPtr& (FileRequestPtr&, FileRequestPtr));
    MOCK_METHOD3(RescheduleRequest, FileRequestPtr(FileRequestPtr, AZStd::chrono::microseconds, IStreamerTypes::Priority));
    MOCK_METHOD4(RescheduleRequest, FileRequestPtr& (FileRequestPtr&, FileRequestPtr, AZStd::chrono::microseconds, IStreamerTypes::Priority));
    MOCK_METHOD1(CreateDedicatedCache, FileRequestPtr(AZStd::string_view));
    MOCK_METHOD2(CreateDedicatedCache, FileRequestPtr& (FileRequestPtr&, AZStd::string_view));
    MOCK_METHOD1(DestroyDedicatedCache, FileRequestPtr(AZStd::string_view));
    MOCK_METHOD2(DestroyDedicatedCache, FileRequestPtr&(FileRequestPtr&, AZStd::string_view));
    MOCK_METHOD1(FlushCache, FileRequestPtr(AZStd::string_view));
    MOCK_METHOD2(FlushCache, FileRequestPtr& (FileRequestPtr&, AZStd::string_view));
    MOCK_METHOD0(FlushCaches, FileRequestPtr());
    MOCK_METHOD1(FlushCaches, FileRequestPtr& (FileRequestPtr&));
    MOCK_METHOD2(Report, FileRequestPtr(AZStd::vector<Statistic>& output, IStreamerTypes::ReportType reportType));
    MOCK_METHOD3(Report, FileRequestPtr&(FileRequestPtr& request, AZStd::vector<Statistic>& output, IStreamerTypes::ReportType reportType));
    MOCK_METHOD1(Custom, FileRequestPtr(AZStd::any));
    MOCK_METHOD2(Custom, FileRequestPtr& (FileRequestPtr&, AZStd::any));
    MOCK_METHOD2(SetRequestCompleteCallback, FileRequestPtr&(FileRequestPtr&, OnCompleteCallback));
    MOCK_METHOD0(CreateRequest, FileRequestPtr());
    MOCK_METHOD2(CreateRequestBatch, void(AZStd::vector<FileRequestPtr>&, size_t));
    MOCK_METHOD1(QueueRequest, void(const FileRequestPtr&));
    MOCK_METHOD1(QueueRequestBatch, void(const AZStd::vector<FileRequestPtr>&));
    MOCK_METHOD1(QueueRequestBatch, void(AZStd::vector<FileRequestPtr>&&));
    MOCK_CONST_METHOD1(HasRequestCompleted, bool(FileRequestHandle));
    MOCK_CONST_METHOD1(GetRequestStatus, IStreamerTypes::RequestStatus(FileRequestHandle));
    MOCK_CONST_METHOD1(GetEstimatedRequestCompletionTime, AZStd::chrono::steady_clock::time_point(FileRequestHandle));
    MOCK_CONST_METHOD4(GetReadRequestResult, bool(FileRequestHandle, void*&, AZ::u64&, IStreamerTypes::ClaimMemory));
    MOCK_METHOD1(CollectStatistics, void(AZStd::vector<Statistic>&));
    MOCK_CONST_METHOD0(GetRecommendations, const IStreamerTypes::Recommendations&());
    MOCK_METHOD0(SuspendProcessing, void());
    MOCK_METHOD0(ResumeProcessing, void());
    MOCK_CONST_METHOD0(IsSuspended, bool());
};
