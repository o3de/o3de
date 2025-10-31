/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>

namespace AZ
{
    namespace IO
    {
        StreamStackEntry::StreamStackEntry(AZStd::string&& name)
            : m_name(AZStd::move(name))
        {
        }

        const AZStd::string& StreamStackEntry::GetName() const
        {
            return m_name;
        }

        void StreamStackEntry::SetNext(AZStd::shared_ptr<StreamStackEntry> next)
        {
            m_next = AZStd::move(next);
        }

        AZStd::shared_ptr<StreamStackEntry> StreamStackEntry::GetNext() const
        {
            return m_next;
        }

        void StreamStackEntry::SetContext(StreamerContext& context)
        {
            m_context = &context;
            if (m_next)
            {
                m_next->SetContext(context);
            }
        }

        void StreamStackEntry::PrepareRequest(FileRequest* request)
        {
            if (m_next)
            {
                m_next->PrepareRequest(request);
            }
            else
            {
                m_context->PushPreparedRequest(request);
            }
        }

        void StreamStackEntry::QueueRequest(FileRequest* request)
        {
            if (m_next)
            {
                m_next->QueueRequest(request);
            }
            else
            {
                request->SetStatus(request->FailsWhenUnhandled() ? IStreamerTypes::RequestStatus::Failed : IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
        }

        bool StreamStackEntry::ExecuteRequests()
        {
            if (m_next)
            {
                return m_next->ExecuteRequests();
            }
            else
            {
                return false;
            }
        }

        void StreamStackEntry::UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now,
            AZStd::vector<FileRequest*>& internalPending, StreamerContext::PreparedQueue::iterator pendingBegin,
            StreamerContext::PreparedQueue::iterator pendingEnd)
        {
            if (m_next)
            {
                m_next->UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);
            }
        }

        void StreamStackEntry::UpdateStatus(Status& status) const
        {
            if (m_next)
            {
                m_next->UpdateStatus(status);
            }
        }

        void StreamStackEntry::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            if (m_next)
            {
                m_next->CollectStatistics(statistics);
            }
        }
    } // namespace IO
} // namespace AZ
