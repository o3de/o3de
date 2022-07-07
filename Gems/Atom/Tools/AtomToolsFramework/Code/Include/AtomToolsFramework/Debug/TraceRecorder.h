/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        //! Return the number of OnAssert calls
        size_t GetAssertCount() const;
        
        //! Return the number of OnException calls
        size_t GetExceptionCount() const;
        
        //! Return the number of OnError calls, and includes OnAssert and OnException if @includeHigher is true
        size_t GetErrorCount(bool includeHigher = false) const;
        
        //! Return the number of OnWarning calls, and includes higher categories if @includeHigher is true
        size_t GetWarningCount(bool includeHigher = false) const;
        
        //! Return the number of OnPrintf calls, and includes higher categories if @includeHigher is true
        size_t GetPrintfCount(bool includeHigher = false) const;

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
        
        size_t m_assertCount = 0;
        size_t m_exceptionCount = 0;
        size_t m_errorCount = 0;
        size_t m_warningCount = 0;
        size_t m_printfCount = 0;
    };
} // namespace AtomToolsFramework
