/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    // Records all TraceMessageBus activity to a string
    class TraceRecorder : private AZ::Debug::TraceMessageBus::Handler
    {
    public:
        AZ_TYPE_INFO(AtomToolsFramework::TraceRecorder, "{7B49AFD0-D0AB-4CB7-A4B5-6D88D30DCBFD}");

        TraceRecorder(size_t maxMessageCount = std::numeric_limits<size_t>::max());
        ~TraceRecorder();

        //! Get the combined output of all messages
        AZStd::string GetDump() const;

    private:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessageBus::Handler overrides...
        bool OnAssert(const char* /*message*/) override;
        bool OnException(const char* /*message*/) override;
        bool OnError(const char* /*window*/, const char* /*message*/) override;
        bool OnWarning(const char* /*window*/, const char* /*message*/) override;
        bool OnPrintf(const char* /*window*/, const char* /*message*/) override;
        //////////////////////////////////////////////////////////////////////////

        size_t m_maxMessageCount = std::numeric_limits<size_t>::max();
        AZStd::list<AZStd::string> m_messages;
    };
} // namespace AtomToolsFramework
