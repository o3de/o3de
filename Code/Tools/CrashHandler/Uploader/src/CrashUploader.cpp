/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CrashUploader.h>
#include <CrashSupport.h>

#include <AzCore/PlatformDef.h>

#include <getopt.h>
#include <util/file/file_reader.h>
#include <util/string/split_string.h>
#include <tools/tool_support.h>
#include <base/strings/utf_string_conversions.h>

#include <fstream>
#include <memory>


namespace O3de
{
    using namespace crashpad;

    std::shared_ptr<CrashUploader> CrashUploader::s_uploader = nullptr;

    bool CheckConfirmation(const crashpad::CrashReportDatabase::Report& report)
    {
        return CrashUploader::GetCrashUploader()->CheckConfirmation(report);
    }

    bool AddAttachments(HTTPMultipartBuilder& builder)
    {
        return CrashUploader::GetCrashUploader()->AddAttachments(builder);
    }

    bool UpdateHttpTransport(std::unique_ptr<crashpad::HTTPTransport>& httpTransport, const std::string& baseURL)
    {
        return CrashUploader::GetCrashUploader()->UpdateHttpTransport(httpTransport, baseURL);
    }

    CrashUploader::CrashUploader(int& argc, char** argv)
    {
        ParseArguments(argc, argv);
    }

    CrashUploader::~CrashUploader()
    {
    }

    bool CrashUploader::DoLogging([[maybe_unused]] logging::LogSeverity severity,
        [[maybe_unused]] const char* file_path,
        [[maybe_unused]] int line,
        [[maybe_unused]] size_t message_start,
        const std::string& string) 
    {
        const char* crashLog = GetLogFileName();

        std::ofstream outFile(crashLog, std::ofstream::out | std::ofstream::app);
        outFile << "[" << CrashHandler::GetTimeString() << "] " << string;

        outFile.close();
        return true;
    }

    void CrashUploader::InstallLogHandler() const
    {
        logging::SetLogMessageHandler(&CrashUploader::DoLogging);
    }

    void CrashUploader::SetCrashUploader(std::shared_ptr<CrashUploader> uploader)
    {
        s_uploader = uploader;
        if (uploader)
        {
            uploader->InstallLogHandler();
        }
        else
        {
            logging::SetLogMessageHandler(nullptr);
        }
    }

    std::shared_ptr<CrashUploader> CrashUploader::GetCrashUploader()
    {
        if (!s_uploader)
        {
            int noArgCount{ 0 };
            s_uploader = std::make_shared<CrashUploader>(noArgCount, nullptr);
        }
        return s_uploader;
    }

    crashpad::UserStreamDataSources* CrashUploader::GetUserStreamSources()
    {
        return &m_userStreams;
    }

    bool CrashUploader::CheckConfirmation([[maybe_unused]] const crashpad::CrashReportDatabase::Report& report)
    {
        return !m_noConfirmation;
    }

    bool CrashUploader::UpdateHttpTransport(std::unique_ptr<crashpad::HTTPTransport>& httpTransport, const std::string& baseURL)
    {
        std::string newURL{ baseURL };
        // Are there already query parameters
        auto findPos = baseURL.find('?');
        if (findPos == std::string::npos)
        {
            newURL += "?";
        }
        else
        {
            newURL += "&";
        }
        newURL += "token=" + m_submissionToken;
        httpTransport->SetURL(newURL);
        return true;
    }

    bool CrashUploader::AddAttachments(crashpad::HTTPMultipartBuilder& builder)
    {
        int attachmentCount = 0;
        for(auto thisFile : m_uploadPaths)
        { 
            static std::vector<std::unique_ptr<crashpad::FileReader>> logFileReaders;

            logFileReaders.push_back(std::make_unique<crashpad::FileReader>());
            crashpad::FileReader* logFileReader = logFileReaders.back().get();
            if (!logFileReader->Open(thisFile))
            {
#if defined(AZ_PLATFORM_WINDOWS)
                LOG(ERROR) << "Failed to open " << base::WideToUTF8(thisFile.BaseName().value());
#else
                LOG(ERROR) << "Failed to open " << thisFile.BaseName().value();
#endif
                continue;
            }

            FileOffset start_offset = logFileReader->SeekGet();
            if (start_offset < 0)
            {
#if defined(AZ_PLATFORM_WINDOWS)
                LOG(ERROR) << "Failed to get offset for " << base::WideToUTF8(thisFile.BaseName().value());
#else
                LOG(ERROR) << "Failed to get offset for " << thisFile.BaseName().value();
#endif
                continue;
            }
            
            ++attachmentCount;
            std::string attachmentKey{ "attachment_" };
            attachmentKey += std::to_string(attachmentCount);

            std::string fileNameKey{ "attachment_" };
#if defined(AZ_PLATFORM_WINDOWS)
            fileNameKey += base::WideToUTF8(thisFile.BaseName().value());
#else
            fileNameKey += thisFile.BaseName().value();
#endif
            attachmentKey = fileNameKey;
            builder.SetFileAttachment(
                attachmentKey,
                fileNameKey,
                logFileReader,
                "");

        }
        return true;
    }

    void CrashUploader::ParseArguments(int& argc, char** argv)
    {
        if (!argc)
        {
            return;
        }

        enum ParsedFlags
        {
            NoConfirmation,
            UploadPath,
            UserStream,
            SubmissionToken,
            ExecutableName,
            EndFlags
        };

        static constexpr option options_list[] = {
            {"noconfirmation", no_argument, nullptr, NoConfirmation},
            {"uploadpath", required_argument, nullptr, UploadPath},
            {"userstream", required_argument, nullptr, UserStream },
            {"submission-token", required_argument, nullptr, SubmissionToken },
            {"executable-name", required_argument, nullptr, ExecutableName },
            {nullptr, no_argument, nullptr, EndFlags}
        };

        int opt{ 0 };
        while ((opt = getopt_long(argc, argv, "", options_list, nullptr)) != -1)
        {
            switch (opt)
            {
            case NoConfirmation:
            {
                m_noConfirmation = true;
                argv[--optind] = argv[--argc];
                break;
            }
            case UploadPath:
            {
                m_uploadPaths.push_back(base::FilePath(crashpad::ToolSupport::CommandLineArgumentToFilePathStringType(optarg)));
                argv[--optind] = argv[--argc];
                break;
            }
            case UserStream:
            {
                argv[--optind] = argv[--argc];
                break;
            }
            case SubmissionToken:
            {
                m_submissionToken = optarg;
                argv[--optind] = argv[--argc];
                break;
            }
            case ExecutableName:
            {
                m_executableName = optarg;
                argv[--optind] = argv[--argc];
                break;
            }
            default:
            {
                // Likely an argument for crashpad - passthrough
            }
            }
        }
        // Reset so we can loop again inside crashpad
        optind = 0;
    }
}
