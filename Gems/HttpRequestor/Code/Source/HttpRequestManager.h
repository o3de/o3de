/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/queue.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/thread.h>

#include <HttpRequestor/HttpRequestParameters.h>
#include <HttpRequestor/HttpTextRequestParameters.h>

namespace HttpRequestor
{
    class Manager
    {
    public:
        Manager();
        virtual ~Manager();

        // Add these parameters to a queue of request parameters to send off as an HTTP request as soon as they reach the head of the queue
        void AddRequest(Parameters && httpRequestParameters);

        // Add these parameters to a queue of request parameters to send off as an HTTP TEXT request as soon as they reach the head of the queue
        void AddTextRequest(TextParameters && httpTextRequestParameters);

    private:
        // RequestManager thread loop.
        void ThreadFunction();

        // Called by ThreadFunction. Waits for timeout or until notified and processes any requests queued up.
        void HandleRequestBatch();

        // Perform an HTTP request, block until a response is received, then give the returned JSON to the callback to parse. Returns the HTTPResponseCode to the callback to handle any errors.
        void HandleRequest(const Parameters & httpRequestParameters);

        // Perform an HTTP request, block until a response is received, then give the returned TEXT to the callback to parse. Returns the HTTPResponseCode to the callback to handle any errors.
        void HandleTextRequest(const TextParameters & httpTextRequestParameters);

    private:
        AZStd::queue<Parameters>                m_requestsToHandle;                 // Queue of requests that will be made in order of time received
        AZStd::queue<TextParameters>            m_textRequestsToHandle;             // Queue of requests for TEXT blobs that will be made in order of time received
        AZStd::mutex                            m_requestMutex;                     // Member variables for synchronization
        AZStd::condition_variable               m_requestConditionVar;
        AZStd::atomic<bool>                     m_runThread;                        // Run flag used to signal the worker thread
        AZStd::thread                           m_thread;                           // This is the thread that will be used for all async operations
        static const char*                      s_loggingName;                      // Name to use for log messages etc...
    };

    using ManagerPtr = AZStd::shared_ptr<Manager>;
}
