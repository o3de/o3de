/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ToolsCrashHandler.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

namespace CrashHandler
{
    namespace
    {
        AZStd::string ResolveAlias(const char* alias)
        {
            if (AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance())
            {
                if (auto resolved = fileIO->ResolvePath(AZ::IO::PathView(alias)); resolved.has_value())
                {
                    return resolved->c_str();
                }
            }
            return {};
        }

        AZStd::string ExecutableSibling(const char* fileName)
        {
            const AZ::IO::FixedMaxPathString exeDir = AZ::Utils::GetExecutableDirectory();
            if (exeDir.empty())
            {
                return fileName;
            }
            return (AZ::IO::FixedMaxPath(exeDir) / fileName).c_str();
        }
    }

    void ToolsCrashHandler::InitCrashHandler(
        const std::string& moduleTag,
        const std::string& devRoot,
        const std::string& crashUrl,
        const std::string& /*crashToken*/,
        const std::string& /*handlerFolder*/,
        const CrashHandlerAnnotations& baseAnnotations,
        const CrashHandlerArguments& /*arguments*/)
    {
        // Callers pass an empty devRoot and expect the handler to work it out; the crash database
        // belongs with the user's writable data, not next to the read-only executable.
        AZStd::string appRoot{ devRoot.c_str() };
        if (appRoot.empty())
        {
            appRoot = ResolveAlias("@user@");
        }
        if (appRoot.empty())
        {
            appRoot = ResolveAlias("@products@");
        }

        SentryCrash::Config config = SentryCrash::LoadConfig(moduleTag.c_str(), appRoot);

        // An explicit URL argument still wins, preserving the old override-by-parameter behaviour.
        if (!crashUrl.empty())
        {
            config.m_dsn = crashUrl.c_str();
        }

        // Attach this tool's own log. It does not exist yet at startup - the logging system creates
        // it moments later - so this is registered unconditionally; sentry only reads attachments
        // when a crash actually occurs, by which point the file is there.
        AZStd::string logDir = ResolveAlias("@log@");
        if (!logDir.empty())
        {
            config.m_attachments.push_back(
                AZStd::string::format("%s/%s.log", logDir.c_str(), moduleTag.c_str()));
        }

        config.m_handlerPath = ExecutableSibling("crashpad_handler.exe");
        config.m_externalReporterPath = ExecutableSibling("SentryCrashReporter.exe");

        if (!SentryCrash::Initialize(config))
        {
            return;
        }

        for (const auto& [key, value] : baseAnnotations)
        {
            SentryCrash::SetTag(key.c_str(), value.c_str());
        }

        // The companion reporter needs this to relaunch on the right project. Prefer the real
        // project path from the SettingsRegistry over the (usually empty) devRoot argument.
        AZStd::string projectPath{ devRoot.c_str() };
        if (projectPath.empty())
        {
            if (auto* registry = AZ::SettingsRegistry::Get(); registry != nullptr)
            {
                AZ::IO::FixedMaxPath resolved;
                if (registry->Get(
                        resolved.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
                {
                    projectPath = resolved.c_str();
                }
            }
        }
        if (!projectPath.empty())
        {
            SentryCrash::SetTag("project_path", projectPath.c_str());
        }
    }
}
