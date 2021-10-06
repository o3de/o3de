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

#include <QOperatingSystemVersion>
#include <QString>

namespace CrashHandler
{

    void ToolsCrashHandler::InitCrashHandler(const std::string& moduleTag, const std::string& devRoot, const std::string& crashUrl, const std::string& crashToken, const std::string& handlerFolder, const CrashHandlerAnnotations& baseAnnotations, const CrashHandlerArguments& arguments)
    {
        ToolsCrashHandler crashHandler;
        crashHandler.Initialize( moduleTag,  devRoot,  crashUrl,  crashToken, handlerFolder, baseAnnotations, arguments);
    }

    std::string ToolsCrashHandler::GetCrashSubmissionURL() const
    {
#if defined(CRASH_HANDLER_URL)
        return MAKE_DEFINE_STRING(CRASH_HANDLER_URL);
#else
        return "https://lumberyard.sp.backtrace.io:8443/";
#endif
    }

    std::string ToolsCrashHandler::GetCrashSubmissionToken() const
    {
        std::string configToken = GetConfigSubmissionToken();
        if (configToken.length())
        {
            return configToken;
        }
#if defined(CRASH_HANDLER_TOKEN)
        return MAKE_DEFINE_STRING(CRASH_HANDLER_TOKEN);
#else
        return "8f562f6bf0ecb674e5f64344d76e6afeccb3244b4a9ea191ee61dc4e3528c5bd";
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
