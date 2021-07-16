/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Base Crashpad Hook
#include <AzCore/PlatformIncl.h>

#include <client/crashpad_client.h>
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/crashpad_info.h>

#include <fstream>

#include <CrashHandler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <CrashSupport.h>

#include <AzCore/Module/Environment.h>
#include <CrashHandler_Traits_Platform.h>

namespace CrashHandler
{
    static const char* disableFile = "disable";
    static const char* crashHandlerEnvVar = "ExceptionHandlerIsSet";
    static const char* crashHandlerAnnotationEnvVar = "CrashHandlerAnnotations";
    static const char* crashSettingsFile = "crashSettings.cfg";

    void CrashHandlerBase::InitCrashHandler(const std::string& moduleTag, const std::string& devRoot, const std::string& crashUrl, const std::string& crashToken, const std::string& handlerFolder, const CrashHandlerAnnotations& baseAnnotations, const CrashHandlerArguments& arguments )
    {
        CrashHandlerBase crashHandler;
        crashHandler.Initialize( moduleTag,  devRoot,  crashUrl, crashToken, handlerFolder, baseAnnotations, arguments);
    }

    std::string CrashHandlerBase::GetCrashReportFolder(const std::string& devRoot) const
    {
        std::string returnFolder{ devRoot };
        returnFolder.append(GetDefaultCrashFolder());
        return returnFolder;
    }

    std::string CrashHandlerBase::GetCrashHandlerPath(const std::string& handlerBasePath) const
    {
        std::string returnPath;
        if (handlerBasePath.length())
        {
            returnPath = handlerBasePath;
        }
        else
        {
            GetExecutableFolder(returnPath);
        }
        
        returnPath.append(GetCrashHandlerExecutableName());
        return returnPath;
    }

    std::string CrashHandlerBase::GetAppRootFromCWD() const
    {
        std::string returnPath{ "./" };

        return returnPath;
    }

    std::string CrashHandlerBase::DetermineAppPath() const
    {
        return GetAppRootFromCWD();
    }

    void CrashHandlerBase::Initialize(const std::string& moduleTag, const std::string& devRoot)
    {
        Initialize(moduleTag, devRoot, GetCrashSubmissionURL(), GetCrashSubmissionToken());
    }

    void CrashHandlerBase::GetBuildAnnotations(CrashHandlerAnnotations& annotations) const
    {
        annotations["product"] = GetProductName();

        annotations["build_tag"] = MAKE_DEFINE_STRING(EXTERNAL_CRASH_REPORTING);

        std::string versionString = std::to_string(EXE_VERSION_INFO_0) + "." + std::to_string(EXE_VERSION_INFO_1) + "." + std::to_string(EXE_VERSION_INFO_2) + "." + std::to_string(EXE_VERSION_INFO_3);
        annotations["version"] = versionString;

        versionString = std::to_string(LY_BUILD);
        annotations["ly_build"] = versionString;

#if defined(_MSC_VER)
        versionString = std::to_string(_MSC_VER);
        annotations["msc_ver"] = versionString;
#endif
    }

    bool CrashHandlerBase::CreateCrashHandlerDB(const std::string& reportPath) const
    {
        AZ_TracePrintf("CrashReporting", "Creating new crash dump db at %s", reportPath.c_str());

#if AZ_TRAIT_CRASHHANDLER_CONVERT_MULTIBYTE_CHARS
        wchar_t pathStr[AZ_MAX_PATH_LEN];
        size_t chars_converted{ 0 };
        mbstowcs_s(&chars_converted, pathStr, reportPath.c_str(), reportPath.length());
        base::FilePath db{ pathStr };
#else
        base::FilePath db{ reportPath };
#endif
        std::unique_ptr<crashpad::CrashReportDatabase> crashDb = crashpad::CrashReportDatabase::Initialize(db);
        if (crashDb)
        {
            return crashDb->GetSettings()->SetUploadsEnabled(true);
        }
        return false;
    }

    void CrashHandlerBase::AppendSep(std::string& pathStr)
    {
        if (pathStr.length() && pathStr[pathStr.length() - 1] != '/' && pathStr[pathStr.length() - 1] != '\\')
        {
            pathStr.append("/");
        }
    }

    void CrashHandlerBase::ReadConfigFile()
    {
        std::string filePath;
        GetExecutableFolder(filePath);

        filePath += crashSettingsFile;

        std::ifstream inputFile(filePath);
        if (!inputFile.is_open())
        {
            return;
        }
        std::string inputString;
        while (std::getline(inputFile, inputString))
        {
            size_t equalPos = inputString.find_first_of('=');
            if (equalPos != std::string::npos && equalPos != inputString.length() - 1)
            {
                std::string tokenStr = inputString.substr(0, equalPos);
                std::string valueStr = inputString.substr(equalPos + 1);

                if (tokenStr == "SubmissionToken")
                {
                    m_submissionToken = valueStr;
                }
            }
        }
    }

    const std::string& CrashHandlerBase::GetConfigSubmissionToken() const
    {
        return m_submissionToken;
    }

    void CrashHandlerBase::Initialize(const std::string& moduleTag, const std::string& appRoot, const std::string& crashUrl, const std::string& crashToken, const std::string& handlerFolder, const CrashHandlerAnnotations& baseAnnotations, const CrashHandlerArguments& arguments)
    {
        ReadConfigFile();

        const std::string url{ crashUrl.length() ? crashUrl : GetCrashSubmissionURL() };
        const std::string token{ crashToken.length() ? crashToken : GetCrashSubmissionToken() };

        std::string lyAppRoot{ appRoot };
        AppendSep(lyAppRoot);

        if (!lyAppRoot.length())
        {
            lyAppRoot = DetermineAppPath();

            if(!lyAppRoot.length())
            {
                AZ_Warning("CrashReporting", false, "Could not determine app root");
                return;
            }
            AppendSep(lyAppRoot);
        }

        std::string dbPath = GetCrashReportFolder(lyAppRoot);
        std::string disableFilePath{ dbPath + disableFile };

        if (AZ::IO::SystemFile::Exists(disableFilePath.c_str()))
        {
            AZ_TracePrintf("CrashReporting", "Disabling crash reporting - disable file found at %s", disableFilePath.c_str());
            return;
        }


#if AZ_TRAIT_CRASHHANDLER_CONVERT_MULTIBYTE_CHARS
        wchar_t pathStr[AZ_MAX_PATH_LEN];
        size_t chars_converted{ 0 };
        mbstowcs_s(&chars_converted, pathStr, dbPath.c_str(), dbPath.length());
        base::FilePath db{ pathStr };
#else
        base::FilePath db{ dbPath };
#endif
        if (!CreateCrashHandlerDB(dbPath))
        {
            AZ_Warning("CrashReporting", false, "Failed to create crash dump path.");
        }

        std::string crashHandlerPath = GetCrashHandlerPath(handlerFolder);

#if AZ_TRAIT_CRASHHANDLER_CONVERT_MULTIBYTE_CHARS
        mbstowcs_s(&chars_converted, pathStr, crashHandlerPath.c_str(), crashHandlerPath.length());
        base::FilePath handler{ pathStr };
#else
        base::FilePath handler{ crashHandlerPath };
#endif

        CrashHandlerAnnotations defaultAnnotations{ baseAnnotations };
        defaultAnnotations["executable"] = moduleTag;

        GetBuildAnnotations(defaultAnnotations);

        // User specific OS info
        GetOSAnnotations(defaultAnnotations);

        GetUserAnnotations(defaultAnnotations);

        // Credentials for crash upload
        defaultAnnotations["token"] = token;
        defaultAnnotations["format"] = "minidump";

        // Our 3rd party provider handles "bad actors" sending too many crash reports, so this should be turned off so we aren't
        // arbitrarily throwing out potentially valid crashes (Default rate limit is 1 hour)

        CrashHandlerArguments argumentList{ arguments };
        argumentList.push_back("--no-rate-limit");
        std::string submissionToken{ "--submission-token=" };
        submissionToken += token;
        argumentList.push_back(submissionToken);
        std::string executableName;
        GetExecutableBaseName(executableName);
        executableName = "--executable-name=" + executableName;
        argumentList.push_back(executableName);
        crashpad::CrashpadClient client;
        // Initialize automatic crashpad handling.
        bool rc = client.StartHandler(handler,
            db,
            db,
            url,
            defaultAnnotations,
            argumentList,
            true,
            true);
        if (rc == false)
        {
            AZ_Warning("CrashReporting", false, "Failed to start crash handler");
            return;
        }
#if AZ_TRAIT_CRASHHANDLER_WAIT_FOR_COMPLETED_HANDLER_LAUNCH
        rc = client.WaitForHandlerStart(INFINITE);
#endif
        if (rc == false)
        {
            AZ_Warning("CrashReporting", false, "Failed to wait for handler to start");
            return;
        }

        static AZ::EnvironmentVariable<bool> envVar = AZ::Environment::CreateVariable<bool>(crashHandlerEnvVar, true);

        AZ_TracePrintf("CrashReporting", "Initialized Crash Handler Successfully.  Crash dumps written to %s.  Handler at %s.", dbPath.c_str(), crashHandlerPath.c_str());
    }

    void CrashHandlerBase::AddAnnotation(const std::string& keyName, const std::string& valueStr)
    {
        AZ::EnvironmentVariable<crashpad::SimpleStringDictionary> annotationVar = AZ::Environment::FindVariable<crashpad::SimpleStringDictionary>(crashHandlerAnnotationEnvVar);
        if (!annotationVar)
        {
            static AZ::EnvironmentVariable<crashpad::SimpleStringDictionary> createAnnotationVar = AZ::Environment::CreateVariable<crashpad::SimpleStringDictionary>(crashHandlerAnnotationEnvVar);
            crashpad::CrashpadInfo* crashpad_info = crashpad::CrashpadInfo::GetCrashpadInfo();
            crashpad_info->set_simple_annotations(&createAnnotationVar.Get());
            annotationVar = createAnnotationVar;
        }

        if (annotationVar)
        {
            annotationVar->SetKeyValue(keyName.c_str(), valueStr.c_str());
        }
    }
}
