/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzToolsFramework/Debug/TraceContextMultiStackHandler.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace AssetBuilder
{
    class TraceMessageHook
        : public AZ::Debug::TraceMessageBus::Handler
        , public AssetBuilderSDK::AssetBuilderTraceBus::Handler
    {
    public:
        TraceMessageHook();
        ~TraceMessageHook() override;

        void EnableTraceContext(bool enable);
        void EnableDebugMode(bool enable);

        bool OnAssert(const char* message) override;
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnException(const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;
        bool OnOutput(const char* window, const char* message) override;

        void IgnoreNextErrors(AZ::u32 count) override;
        void IgnoreNextWarning(AZ::u32 count) override;
        void IgnoreNextPrintf(AZ::u32 count) override;

        void ResetWarningCount() override;
        void ResetErrorCount() override;
        AZ::u32 GetWarningCount() override;
        AZ::u32 GetErrorCount() override;

        void DumpTraceContext(FILE* stream) const;

        void CleanMessage(FILE* stream, const char* prefix, const char* message, bool forceFlush, const char* extraPrefix = nullptr, bool includeTraceContext = true) const;

    protected:
        AzToolsFramework::Debug::TraceContextMultiStackHandler* m_stacks;
        AZ::u32 m_skipErrorsCount;
        AZ::u32 m_skipWarningsCount;
        AZ::u32 m_skipPrintfsCount;
        AZ::u32 m_totalWarningCount;
        AZ::u32 m_totalErrorCount;
        bool m_inDebugMode;

        // once we're in an exception, we accept all log data as error, since we will terminate
        // this ensures that call stack info (which is 'traced', not 'exceptioned') is present.
        bool m_isInException = false;
    };
} // namespace AssetBuilder
