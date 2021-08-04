/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

#include <QAction>
#include <QObject>

namespace AWSCore
{
    class AWSCoreResourceMappingToolAction
        : public QAction
    {
    public:
        static constexpr const char AWSCoreResourceMappingToolActionName[] = "AWSCoreResourceMappingToolAction";
        static constexpr const char ResourceMappingToolDirectoryPath[] = "Gems/AWSCore/Code/Tools/ResourceMappingTool";
        static constexpr const char ResourceMappingToolLogDirectoryPath[] = "user/log/";
        static constexpr const char EngineWindowsPythonEntryScriptPath[] = "python/python.cmd";

        AWSCoreResourceMappingToolAction(const QString& text, QObject* parent = nullptr);

        void InitAWSCoreResourceMappingToolAction();

        AZStd::string GetToolLaunchCommand() const;
        AZStd::string GetToolLogFilePath() const;
        AZStd::string GetToolReadMePath() const;

    private:
        bool m_isDebug;
        AZStd::string m_enginePythonEntryPath;
        AZStd::string m_toolScriptPath;
        AZStd::string m_toolQtBinDirectoryPath;

        AZStd::string m_toolLogDirectoryPath;
        AZStd::string m_toolConfigDirectoryPath;
        AZStd::string m_toolReadMePath;
    };
} // namespace AWSCore
