/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <limits>
#include <AzCore/base.h>
#include <AzCore/IO/Streamer/FileRange.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace IO
    {
        class FileRequest;
        class StreamerContext;

        //! The StreamStack is a stack of elements that serve a specific file IO function such
        //! collecting information from a file, cache read data, decompressing data, etc. The basic
        //! functionality tries perform its task and if it fails pass the request to the next entry
        //! in the stack. For instance if a request isn't cached it passes the request to the next
        //! entry which might read it from disk instead.
        class StreamStackEntry
        {
        public:
            struct Status
            {
                //! The number of requests that can still be submitted to the stack. If this is positive
                //! more requests can be issued. If it's negative the stack is saturated and no more requests
                //! should be issued.
                s32 m_numAvailableSlots{ std::numeric_limits<s32>::max() };
                //! True if no node in the stack is doing any work or has any work pending.
                bool m_isIdle{ true };
            };

            explicit StreamStackEntry(AZStd::string&& name);
            virtual ~StreamStackEntry() = default;

            //! Returns the name that uniquely identifies this entry.
            virtual const AZStd::string& GetName() const;

            //! Set the next entry in the stack or reset it by using a nullptr.
            //! If the next entry has already been set, this will overwrite it.
            virtual void SetNext(AZStd::shared_ptr<StreamStackEntry> next);
            //! Returns the next entry in the stack that handles a request if 
            //! this request can't handle the request, or null if there are no more entries. 
            virtual AZStd::shared_ptr<StreamStackEntry> GetNext() const;

            virtual void SetContext(StreamerContext& context);

            //! Prepare an external request for processing. This can include resolving file paths,
            //! create more specific internal requests, etc. The returned will be queued for further
            //! processing by QueueRequest and ExecuteRequest.
            virtual void PrepareRequest(FileRequest* request);
            //! Queues a request to be executed at a later point when ExecuteRequests is called. This can
            //! include splitting up the request in more fine-grained steps.
            virtual void QueueRequest(FileRequest* request);
            //! Executes one or more queued requests. This is needed for synchronously executing requests,
            //! but asynchronous requests can already be running from the PrepareRequest call in which case
            //! this call is ignored.
            //! @return True if a request was processed, otherwise false.
            virtual bool ExecuteRequests();
            
            //! Gets a combined status update from all the nodes in the stack.
            virtual void UpdateStatus(Status& status) const;
            //! Updates the estimate of the time the requests will complete. This generally works by bubbling
            //! up the estimation and each stack entry adding it's additional overhead if any. When chaining
            //! this call, first call the next entry in the stack before adding the current entry's estimate.
            //! @param now The current time. This is captured once to avoid repeatedly querying the system clock.
            //! @param internalPending The requests that are pending in the stream stack. These are always estimated as coming after
            //!     the queued requests. Because this call will go from the top of the stack to the bottom, but estimation is calculated
            //!     from the bottom to the top, this list should be processed in reverse order.
            //! @param pendingBegin Iterator pointing to the start of the requests that are waiting for a processing slot in the stack.
            //! @param pendingEnd Iterator pointing to the end of the requests that are waiting for a processing slot in the stack.
            virtual void UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
                StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd);

            //! Collect various statistics on this stack entry. These are for profiling and debugging
            //! purposes only.
            virtual void CollectStatistics(AZStd::vector<Statistic>& statistics) const;

        protected:
            StreamStackEntry() = default;
            
            //! The name that uniquely identifies this entry.
            AZStd::string m_name;
            //! The next entry in the stack
            AZStd::shared_ptr<StreamStackEntry> m_next;
            //! Context information for the entire streaming stack.
            StreamerContext* m_context;
        };
    } // namespace IO
} // namespace AZ
