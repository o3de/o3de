/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
