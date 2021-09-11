/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>
#include <AzCore/Debug/TraceMessageBus.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class Log
            : private AZ::Debug::TraceMessageBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Log, AZ::SystemAllocator, 0);

            void Activate();
            void AddEntry(const char* format, ...);
            void Deactivate();
            const AZStd::vector<AZStd::string>& GetEntries() const;
            void SetVersionExporerExclusivity(bool enabled);
            void SetVerbose(bool verbosity);

        private:
            bool m_isExclusiveReportingEnabled = false;
            bool m_isVerbose = false;
            AZStd::vector<AZStd::string> m_logs;

            bool CaptureFromTraceBus(const char* window, const char* message);
            bool OnException(const char* /*message*/) override;
            bool OnPrintf(const char* /*window*/, const char* /*message*/) override;
            bool OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
            bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        };
    }
}
