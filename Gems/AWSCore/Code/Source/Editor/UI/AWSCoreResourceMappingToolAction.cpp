/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AWSCoreInternalBus.h>
#include <Editor/UI/AWSCoreResourceMappingToolAction.h>

namespace AWSCore
{
    AWSCoreResourceMappingToolAction::AWSCoreResourceMappingToolAction(const QString& text)
        : QAction(text)
        , m_isDebug(false)
        , m_enginePythonEntryPath("")
        , m_toolScriptPath("")
        , m_toolQtBinDirectoryPath("")
        , m_toolLogPath("")
        , m_toolReadMePath("")
    {
        auto engineRootPath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@engroot@");
        if (!engineRootPath)
        {
            AZ_Error("AWSCoreEditor", false, "Failed to determine engine root path.");
        }
        else
        {
            m_enginePythonEntryPath = AZStd::string::format("%s/%s", engineRootPath, EngineWindowsPythonEntryScriptPath);
            AzFramework::StringFunc::Path::Normalize(m_enginePythonEntryPath);
            if (!AZ::IO::SystemFile::Exists(m_enginePythonEntryPath.c_str()))
            {
                AZ_Error("AWSCoreEditor", false, "Failed to find engine python entry at %s.", m_enginePythonEntryPath.c_str());
                m_enginePythonEntryPath.clear();
            }

            m_toolScriptPath = AZStd::string::format("%s/%s/resource_mapping_tool.py", engineRootPath, ResourceMappingToolDirectoryPath);
            AzFramework::StringFunc::Path::Normalize(m_toolScriptPath);
            if (!AZ::IO::SystemFile::Exists(m_toolScriptPath.c_str()))
            {
                AZ_Error("AWSCoreEditor", false, "Failed to find ResourceMappingTool python script at %s.", m_toolScriptPath.c_str());
                m_toolScriptPath.clear();
            }

            m_toolLogPath = AZStd::string::format("%s/%s/resource_mapping_tool.log", engineRootPath, ResourceMappingToolDirectoryPath);
            AzFramework::StringFunc::Path::Normalize(m_toolLogPath);
            if (!AZ::IO::SystemFile::Exists(m_toolLogPath.c_str()))
            {
                AZ_Error("AWSCoreEditor", false, "Failed to find ResourceMappingTool log file at %s.", m_toolLogPath.c_str());
                m_toolLogPath.clear();
            }

            m_toolReadMePath = AZStd::string::format("%s/%s/README.md", engineRootPath, ResourceMappingToolDirectoryPath);
            AzFramework::StringFunc::Path::Normalize(m_toolReadMePath);
            if (!AZ::IO::SystemFile::Exists(m_toolReadMePath.c_str()))
            {
                AZ_Error("AWSCoreEditor", false, "Failed to find ResourceMappingTool README file at %s.", m_toolReadMePath.c_str());
                m_toolReadMePath.clear();
            }

            char executablePath[AZ_MAX_PATH_LEN];
            auto result = AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);
            if (result.m_pathStored != AZ::Utils::ExecutablePathResult::Success)
            {
                AZ_Error("AWSCoreEditor", false, "Failed to find engine executable path.");
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
                AZ_Error("AWSCoreEditor", false, "Failed to find ResourceMappingTool Qt binaries at %s.", m_toolQtBinDirectoryPath.c_str());
                m_toolQtBinDirectoryPath.clear();
            }
        }
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolLaunchCommand() const
    {
        if (m_enginePythonEntryPath.empty() || m_toolScriptPath.empty() || m_toolQtBinDirectoryPath.empty())
        {
            return "";
        }

        AZStd::string profileName = "default";
        AWSCoreInternalRequestBus::BroadcastResult(profileName, &AWSCoreInternalRequests::GetProfileName);

        AZStd::string configPath = "";
        AWSCoreInternalRequestBus::BroadcastResult(configPath, &AWSCoreInternalRequests::GetResourceMappingConfigFolderPath);

        if (m_isDebug)
        {
            return AZStd::string::format(
                "%s debug %s --binaries_path %s --debug --profile %s --config_path %s", m_enginePythonEntryPath.c_str(),
                m_toolScriptPath.c_str(), m_toolQtBinDirectoryPath.c_str(), profileName.c_str(), configPath.c_str());
        }
        else
        {
            return AZStd::string::format(
                "%s %s --binaries_path %s --profile %s --config_path %s", m_enginePythonEntryPath.c_str(),
                m_toolScriptPath.c_str(), m_toolQtBinDirectoryPath.c_str(), profileName.c_str(), configPath.c_str());
        }
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolLogPath() const
    {
        return m_toolLogPath;
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolReadMePath() const
    {
        return m_toolReadMePath;
    }
} // namespace AWSCore
