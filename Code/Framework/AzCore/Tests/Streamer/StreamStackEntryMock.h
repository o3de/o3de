/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        MOCK_METHOD4(UpdateCompletionEstimates, void(AZStd::chrono::system_clock::time_point,
            AZStd::vector<FileRequest*>&, StreamerContext::PreparedQueue::iterator, StreamerContext::PreparedQueue::iterator));

        MOCK_CONST_METHOD1(CollectStatistics, void(AZStd::vector<Statistic>&));

        inline void ForwardSetContext(StreamerContext& context) { StreamStackEntry::SetContext(context); }
        inline void ForwardPrepareRequest(FileRequest* request) { StreamStackEntry::PrepareRequest(request); }
        inline void ForwardQueueRequest(FileRequest* request) { StreamStackEntry::QueueRequest(request); }
    };
} // namespace AZ::IO
