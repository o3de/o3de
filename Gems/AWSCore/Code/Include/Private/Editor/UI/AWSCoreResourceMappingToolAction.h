/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

#include <QAction>

namespace AWSCore
{
    class AWSCoreResourceMappingToolAction
        : public QAction
    {
    public:
        static constexpr const char ResourceMappingToolDirectoryPath[] = "Gems/AWSCore/Code/Tools/ResourceMappingTool";
        static constexpr const char EngineWindowsPythonEntryScriptPath[] = "python/python.cmd";

        AWSCoreResourceMappingToolAction(const QString& text);

        AZStd::string GetToolLaunchCommand() const;
        AZStd::string GetToolLogPath() const;
        AZStd::string GetToolReadMePath() const;

    private:
        bool m_isDebug;
        AZStd::string m_enginePythonEntryPath;
        AZStd::string m_toolScriptPath;
        AZStd::string m_toolQtBinDirectoryPath;

        AZStd::string m_toolLogPath;
        AZStd::string m_toolReadMePath;
    };
} // namespace AWSCore
