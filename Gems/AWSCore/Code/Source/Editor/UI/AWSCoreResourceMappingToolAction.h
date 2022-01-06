/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/string.h>

#include <QAction>
#include <QObject>

#include "AWSCoreEditor_Traits_Platform.h"

namespace AWSCore
{
    class AWSCoreResourceMappingToolAction
        : public QAction
    {
    public:
        static constexpr const char AWSCoreResourceMappingToolActionName[] = "AWSCoreResourceMappingToolAction";
        static constexpr const char ResourceMappingToolDirectoryPath[] = "Gems/AWSCore/Code/Tools/ResourceMappingTool";
        static constexpr const char ResourceMappingToolLogDirectoryPath[] = "user/log/";
        static constexpr const char EngineWindowsPythonEntryScriptPath[] = AWSCORE_EDITOR_PYTHON_COMMAND;

        AWSCoreResourceMappingToolAction(const QString& text, QObject* parent = nullptr);

        void InitAWSCoreResourceMappingToolAction();

        AZStd::string GetToolLaunchCommand() const;
        AZStd::string GetToolLogFilePath() const;
        AZStd::string GetToolReadMePath() const;

    private:
        bool m_isDebug;
        AZ::IO::Path m_enginePythonEntryPath;
        AZ::IO::Path m_toolScriptPath;
        AZ::IO::Path m_toolQtBinDirectoryPath;
        AZ::IO::Path m_toolLogDirectoryPath;
        AZ::IO::Path m_toolConfigDirectoryPath;
        AZ::IO::Path m_toolReadMePath;
    };
} // namespace AWSCore
