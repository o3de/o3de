/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AWSCoreInternalBus.h>
#include <Configuration/AWSCoreConfiguration.h>
#include <Editor/UI/AWSCoreResourceMappingToolAction.h>

namespace AWSCore
{
    AWSCoreResourceMappingToolAction::AWSCoreResourceMappingToolAction(const QString& text, QObject* parent)
        : QAction(text, parent)
        , m_isDebug(false)
        , m_enginePythonEntryPath("")
        , m_toolScriptPath("")
        , m_toolQtBinDirectoryPath("")
        , m_toolLogDirectoryPath("")
        , m_toolConfigDirectoryPath("")
        , m_toolReadMePath("")
    {
        InitAWSCoreResourceMappingToolAction();
    }

    void AWSCoreResourceMappingToolAction::InitAWSCoreResourceMappingToolAction()
    {
        auto engineRootPath = AZ::Utils::GetEnginePath();

        m_enginePythonEntryPath = AZStd::string::format("%s/%s", engineRootPath.c_str(), EngineWindowsPythonEntryScriptPath);
        AzFramework::StringFunc::Path::Normalize(m_enginePythonEntryPath);
        if (!AZ::IO::SystemFile::Exists(m_enginePythonEntryPath.c_str()))
        {
            AZ_Error(
                AWSCoreResourceMappingToolActionName, false, "Failed to find engine python entry at %s.", m_enginePythonEntryPath.c_str());
            m_enginePythonEntryPath.clear();
        }

        m_toolScriptPath =
            AZStd::string::format("%s/%s/resource_mapping_tool.py", engineRootPath.c_str(), ResourceMappingToolDirectoryPath);
        AzFramework::StringFunc::Path::Normalize(m_toolScriptPath);
        if (!AZ::IO::SystemFile::Exists(m_toolScriptPath.c_str()))
        {
            AZ_Error(
                AWSCoreResourceMappingToolActionName, false, "Failed to find ResourceMappingTool python script at %s.",
                m_toolScriptPath.c_str());
            m_toolScriptPath.clear();
        }

        auto projectPath = AZ::Utils::GetProjectPath();
        m_toolLogDirectoryPath = AZStd::string::format("%s/%s", projectPath.c_str(), ResourceMappingToolLogDirectoryPath);
        AzFramework::StringFunc::Path::Normalize(m_toolLogDirectoryPath);

        m_toolConfigDirectoryPath =
            AZStd::string::format("%s/%s", projectPath.c_str(), AWSCoreConfiguration::AWSCoreResourceMappingConfigFolderName);
        AzFramework::StringFunc::Path::Normalize(m_toolConfigDirectoryPath);

        m_toolReadMePath = AZStd::string::format("%s/%s/README.md", engineRootPath.c_str(), ResourceMappingToolDirectoryPath);
        AzFramework::StringFunc::Path::Normalize(m_toolReadMePath);
        if (!AZ::IO::SystemFile::Exists(m_toolReadMePath.c_str()))
        {
            AZ_Error(
                AWSCoreResourceMappingToolActionName, false, "Failed to find ResourceMappingTool README file at %s.",
                m_toolReadMePath.c_str());
            m_toolReadMePath.clear();
        }

        char executablePath[AZ_MAX_PATH_LEN];
        auto result = AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);
        if (result.m_pathStored != AZ::Utils::ExecutablePathResult::Success)
        {
            AZ_Error(AWSCoreResourceMappingToolActionName, false, "Failed to find engine executable path.");
        }
        else
        {
            if (result.m_pathIncludesFilename)
            {
                // Remove the file name if it exists, and keep the parent folder only
                char* lastSeparatorAddress = strrchr(executablePath, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                if (lastSeparatorAddress)
                {
                    *lastSeparatorAddress = '\0';
                }
            }
        }

        AZStd::string binDirectoryPath(executablePath);
        auto lastSeparator = binDirectoryPath.find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
        if (lastSeparator != AZStd::string::npos)
        {
            m_isDebug = binDirectoryPath.substr(lastSeparator).contains("debug");
        }

        m_toolQtBinDirectoryPath = AZStd::string::format("%s/%s", binDirectoryPath.c_str(), "AWSCoreEditorQtBin");
        AzFramework::StringFunc::Path::Normalize(m_toolQtBinDirectoryPath);
        if (!AZ::IO::SystemFile::Exists(m_toolQtBinDirectoryPath.c_str()))
        {
            AZ_Error(
                AWSCoreResourceMappingToolActionName, false, "Failed to find ResourceMappingTool Qt binaries at %s.",
                m_toolQtBinDirectoryPath.c_str());
            m_toolQtBinDirectoryPath.clear();
        }
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolLaunchCommand() const
    {
        if (m_enginePythonEntryPath.empty() ||
            m_toolScriptPath.empty() ||
            m_toolQtBinDirectoryPath.empty() ||
            m_toolConfigDirectoryPath.empty() ||
            m_toolLogDirectoryPath.empty())
        {
            AZ_Error(AWSCoreResourceMappingToolActionName, false,
                "Failed to get tool launch command, engine python path: %s, tool script path: %s, tool qt binaries path: %s, tool config path: %s, tool log path: %s",
                m_enginePythonEntryPath.c_str(), m_toolScriptPath.c_str(), m_toolQtBinDirectoryPath.c_str(), m_toolConfigDirectoryPath.c_str(), m_toolLogDirectoryPath.c_str());
            return "";
        }

        AZStd::string profileName = "default";
        AWSCoreInternalRequestBus::BroadcastResult(profileName, &AWSCoreInternalRequests::GetProfileName);

        if (m_isDebug)
        {
            return AZStd::string::format(
                "%s debug -B %s --binaries_path %s --debug --profile %s --config_path %s --log_path %s",
                m_enginePythonEntryPath.c_str(), m_toolScriptPath.c_str(), m_toolQtBinDirectoryPath.c_str(),
                profileName.c_str(), m_toolConfigDirectoryPath.c_str(), m_toolLogDirectoryPath.c_str());
        }
        else
        {
            return AZStd::string::format(
                "%s -B %s --binaries_path %s --profile %s --config_path %s --log_path %s",
                m_enginePythonEntryPath.c_str(), m_toolScriptPath.c_str(), m_toolQtBinDirectoryPath.c_str(),
                profileName.c_str(), m_toolConfigDirectoryPath.c_str(), m_toolLogDirectoryPath.c_str());
        }
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolLogFilePath() const
    {
        AZStd::string toolLogFilePath = AZStd::string::format("%s/resource_mapping_tool.log", m_toolLogDirectoryPath.c_str());
        AzFramework::StringFunc::Path::Normalize(toolLogFilePath);
        return toolLogFilePath;
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolReadMePath() const
    {
        return m_toolReadMePath;
    }
} // namespace AWSCore
