/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>

#include <AWSCoreInternalBus.h>
#include <Configuration/AWSCoreConfiguration.h>
#include <Editor/UI/AWSCoreResourceMappingToolAction.h>

namespace AWSCore
{
    AWSCoreResourceMappingToolAction::AWSCoreResourceMappingToolAction(const QString& text, QObject* parent)
        : QAction(text, parent)
        , m_isDebug(false)
    {
        InitAWSCoreResourceMappingToolAction();
    }

    void AWSCoreResourceMappingToolAction::InitAWSCoreResourceMappingToolAction()
    {
        AZ::IO::Path engineRootPath = AZ::IO::PathView(AZ::Utils::GetEnginePath());
        m_enginePythonEntryPath = (engineRootPath / EngineWindowsPythonEntryScriptPath).LexicallyNormal();
        m_toolScriptPath = (engineRootPath / ResourceMappingToolDirectoryPath / "resource_mapping_tool.py").LexicallyNormal();
        m_toolReadMePath = (engineRootPath / ResourceMappingToolDirectoryPath / "README.md").LexicallyNormal();

        AZ::IO::Path projectPath = AZ::IO::PathView(AZ::Utils::GetProjectPath());
        m_toolLogDirectoryPath = (projectPath / ResourceMappingToolLogDirectoryPath).LexicallyNormal();
        m_toolConfigDirectoryPath = (projectPath / AWSCoreConfiguration::AWSCoreResourceMappingConfigFolderName).LexicallyNormal();

        AZ::IO::Path executablePath = AZ::IO::PathView(AZ::Utils::GetExecutableDirectory());
        m_toolQtBinDirectoryPath = (executablePath / "AWSCoreEditorQtBin").LexicallyNormal();

        m_isDebug = AZStd::string_view(AZ_BUILD_CONFIGURATION_TYPE) == "debug";
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolLaunchCommand() const
    {
        if (!AZ::IO::SystemFile::Exists(m_enginePythonEntryPath.c_str()) ||
            !AZ::IO::SystemFile::Exists(m_toolScriptPath.c_str()) ||
            !AZ::IO::SystemFile::Exists(m_toolQtBinDirectoryPath.c_str()) ||
            !AZ::IO::SystemFile::Exists(m_toolConfigDirectoryPath.c_str()) ||
            !AZ::IO::SystemFile::Exists(m_toolLogDirectoryPath.c_str()))
        {
            AZ_Error(AWSCoreResourceMappingToolActionName, false,
                "Expected parameter for tool launch command is invalid, engine python path: %s, tool script path: %s, tool qt binaries path: %s, tool config path: %s, tool log path: %s",
                m_enginePythonEntryPath.c_str(), m_toolScriptPath.c_str(), m_toolQtBinDirectoryPath.c_str(), m_toolConfigDirectoryPath.c_str(), m_toolLogDirectoryPath.c_str());
            return "";
        }

        AZStd::string profileName = "default";
        AWSCoreInternalRequestBus::BroadcastResult(profileName, &AWSCoreInternalRequests::GetProfileName);

        if (m_isDebug)
        {
            return AZStd::string::format(
                "\"%s\" " AWSCORE_EDITOR_PYTHON_DEBUG_ARGUMENT "-B \"%s\" --binaries-path \"%s\" --debug --profile \"%s\" --config-path \"%s\" --log-path \"%s\"",
                m_enginePythonEntryPath.c_str(), m_toolScriptPath.c_str(), m_toolQtBinDirectoryPath.c_str(),
                profileName.c_str(), m_toolConfigDirectoryPath.c_str(), m_toolLogDirectoryPath.c_str());
        }
        else
        {
            return AZStd::string::format(
                "\"%s\" -B \"%s\" --binaries-path \"%s\" --profile \"%s\" --config-path \"%s\" --log-path \"%s\"",
                m_enginePythonEntryPath.c_str(), m_toolScriptPath.c_str(), m_toolQtBinDirectoryPath.c_str(),
                profileName.c_str(), m_toolConfigDirectoryPath.c_str(), m_toolLogDirectoryPath.c_str());
        }
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolLogFilePath() const
    {
        AZ::IO::Path toolLogFilePath = (m_toolLogDirectoryPath / "resource_mapping_tool.log").LexicallyNormal();
        if (!AZ::IO::SystemFile::Exists(toolLogFilePath.c_str()))
        {
            AZ_Error(AWSCoreResourceMappingToolActionName, false, "Invalid tool log file path: %s", toolLogFilePath.c_str());
            return "";
        }
        return toolLogFilePath.Native();
    }

    AZStd::string AWSCoreResourceMappingToolAction::GetToolReadMePath() const
    {
        if (!AZ::IO::SystemFile::Exists(m_toolReadMePath.c_str()))
        {
            AZ_Error(AWSCoreResourceMappingToolActionName, false, "Invalid tool readme path: %s", m_toolReadMePath.c_str());
            return "";
        }
        return m_toolReadMePath.Native();
    }
} // namespace AWSCore
