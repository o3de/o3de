/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>

namespace AZ::IO
{
    class StreamStackEntryMock
        : public StreamStackEntry
    {
    public:
        StreamStackEntryMock() : StreamStackEntry("StreamStackEntryMock") {}
        ~StreamStackEntryMock() override = default;

        MOCK_CONST_METHOD0(GetName, const AZStd::string&());
            
        MOCK_METHOD1(SetNext, void(AZStd::shared_ptr<StreamStackEntry>));
        MOCK_CONST_METHOD0(GetNext, AZStd::shared_ptr<StreamStackEntry>());

        MOCK_METHOD1(SetContext, void(StreamerContext&));
            
        MOCK_METHOD1(PrepareRequest, void(FileRequest*));
        MOCK_METHOD1(QueueRequest, void(FileRequest*));
        MOCK_METHOD0(ExecuteRequests, bool());
        
        MOCK_CONST_METHOD1(UpdateStatus, void(Status& status));
        MOCK_METHOD4(UpdateCompletionEstimates, void(AZStd::chrono::steady_clock::time_point,
            AZStd::vector<FileRequest*>&, StreamerContext::PreparedQueue::iterator, StreamerContext::PreparedQueue::iterator));

        MOCK_CONST_METHOD1(CollectStatistics, void(AZStd::vector<Statistic>&));

        inline void ForwardSetContext(StreamerContext& context) { StreamStackEntry::SetContext(context); }
        inline void ForwardPrepareRequest(FileRequest* request) { StreamStackEntry::PrepareRequest(request); }
        inline void ForwardQueueRequest(FileRequest* request) { StreamStackEntry::QueueRequest(request); }
    };
} // namespace AZ::IO
