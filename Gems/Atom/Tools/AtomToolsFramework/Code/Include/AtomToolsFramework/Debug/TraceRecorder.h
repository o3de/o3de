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

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    // Records all TraceMessageBus activity to a string
    class TraceRecorder
        : private AZ::Debug::TraceMessageBus::Handler
    {
    public:
        AZ_TYPE_INFO(AtomToolsFramework::TraceRecorder, "{7B49AFD0-D0AB-4CB7-A4B5-6D88D30DCBFD}");

        TraceRecorder();
        ~TraceRecorder();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessageBus::Handler overrides...
        bool OnAssert(const char* /*message*/) override;
        bool OnException(const char* /*message*/) override;
        bool OnError(const char* /*window*/, const char* /*message*/) override;
        bool OnWarning(const char* /*window*/, const char* /*message*/) override;
        bool OnPrintf(const char* /*window*/, const char* /*message*/) override;
        //////////////////////////////////////////////////////////////////////////

        AZStd::string m_messageSink;
    };
} // namespace AtomToolsFramework
