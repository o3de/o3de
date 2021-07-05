/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <client/crash_report_database.h>
#include <util/net/http_multipart_builder.h>
#include <util/net/http_transport.h>
#include <handler/user_stream_data_source.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <base/logging.h>
#include <string>

namespace O3de
{
    bool CheckConfirmation(const crashpad::CrashReportDatabase::Report& report);
    void InstallCrashUploader(int& argc, char* argv[]);
    bool AddAttachments(crashpad::HTTPMultipartBuilder& builder);
    bool UpdateHttpTransport(std::unique_ptr<crashpad::HTTPTransport>& httpTransport, const std::string& baseURL);

    class CrashUploader
    {
    public:
        CrashUploader(int& argc, char** argv);
        virtual ~CrashUploader();

        static void SetCrashUploader(std::shared_ptr<CrashUploader> uploader);
        static std::shared_ptr<CrashUploader> GetCrashUploader();

        virtual bool CheckConfirmation(const crashpad::CrashReportDatabase::Report& report);
        virtual void InstallLogHandler() const;
        virtual bool AddAttachments(crashpad::HTTPMultipartBuilder& builder);
        virtual bool UpdateHttpTransport(std::unique_ptr<crashpad::HTTPTransport>& httpTransport, const std::string& baseURL);

        static const char* GetLogFileName() { return "CrashUploaderLog.txt"; }

        // base/logging.h::LogMessageHandlerFunction
        static bool DoLogging(logging::LogSeverity severity,
            const char* file_poath,
            int line,
            size_t message_start,
            const std::string& string);

        // We parse out our arguments from the list and let the rest pass through to crashpad
        virtual void ParseArguments(int& argc, char** argv);
        virtual crashpad::UserStreamDataSources* GetUserStreamSources();
    protected:
        // Skip confirmation dialog by argument
        bool m_noConfirmation{ false };

        // Log paths to uploadas user streams with the minidump
        std::vector<base::FilePath> m_uploadPaths;
        crashpad::UserStreamDataSources m_userStreams;
        static std::shared_ptr<CrashUploader> s_uploader;
        std::string m_submissionToken;
        std::string m_executableName;
    };
}
