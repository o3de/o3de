/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Editor Crashpad Hook

#include <ToolsCrashHandler.h>
#include <CrashSupport.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/string.h>

#include <QOperatingSystemVersion>
#include <QString>

namespace CrashHandler
{
    // Per-project override, e.g. AutomatedTesting/Registry/crash_handler.setreg
    static const char* settingKey_crashReportingUrl = "/O3DE/CrashReporting/Url";
    static const char* settingKey_crashReportingToken = "/O3DE/CrashReporting/SubmissionToken";

    void ToolsCrashHandler::InitCrashHandler(const std::string& moduleTag, const std::string& devRoot, const std::string& crashUrl, const std::string& crashToken, const std::string& handlerFolder, const CrashHandlerAnnotations& baseAnnotations, const CrashHandlerArguments& arguments)
    {
        ToolsCrashHandler crashHandler;
        crashHandler.Initialize( moduleTag,  devRoot,  crashUrl,  crashToken, handlerFolder, baseAnnotations, arguments);
    }

    std::string ToolsCrashHandler::GetCrashSubmissionURL() const
    {
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            AZStd::string url;
            if (settingsRegistry->Get(url, settingKey_crashReportingUrl) && !url.empty())
            {
                return url.c_str();
            }
        }
#if defined(CRASH_HANDLER_URL)
        return MAKE_DEFINE_STRING(CRASH_HANDLER_URL);
#else
        return "";
#endif
    }

    std::string ToolsCrashHandler::GetCrashSubmissionToken() const
    {
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            AZStd::string submissionToken;
            if (settingsRegistry->Get(submissionToken, settingKey_crashReportingToken) && !submissionToken.empty())
            {
                return submissionToken.c_str();
            }
        }

        std::string configToken = GetConfigSubmissionToken();
        if (configToken.length())
        {
            return configToken;
        }
#if defined(CRASH_HANDLER_TOKEN)
        return MAKE_DEFINE_STRING(CRASH_HANDLER_TOKEN);
#else
        return "";
#endif
    }

    std::string ToolsCrashHandler::DetermineAppPath() const
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            // If our devroot alias is available, use that
            const char* devAlias = fileIO->GetAlias("@engroot@");
            if (devAlias)
            {
                return devAlias;
            }
        }
        return GetAppRootFromCWD();
    }


    void ToolsCrashHandler::GetOSAnnotations(CrashHandlerAnnotations& annotations) const
    {
        CrashHandlerBase::GetOSAnnotations(annotations);

        std::string annotationBuf = QOperatingSystemVersion::current().name().toUtf8().data();
        annotations["os.qtversion"] = annotationBuf;

    }
}
