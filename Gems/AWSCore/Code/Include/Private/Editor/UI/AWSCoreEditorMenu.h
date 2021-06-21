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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#include <AWSCoreBus.h>

#include <QMenu>

namespace AzFramework
{
    class ProcessWatcher;
}

namespace AWSCore
{
    class AWSCoreEditorMenu
        : public QMenu
        , AWSCoreEditorRequestBus::Handler
    {
    public:
        static constexpr const char AWSResourceMappingToolReadMeWarningText[] =
            "Failed to launch Resource Mapping Tool, please follow <a href=\"file:///%s\">README</a> to setup tool before using it.";
        static constexpr const char AWSResourceMappingToolIsRunningText[] = "Resource Mapping Tool is running...";
        static constexpr const char AWSResourceMappingToolLogWarningText[] =
            "Failed to launch Resource Mapping Tool, please check <a href=\"file:///%s\">logs</a> for details.";

        AWSCoreEditorMenu(const QString& text);
        ~AWSCoreEditorMenu();

    private:
        QAction* AddExternalLinkAction(const AZStd::string& name, const AZStd::string& url, const AZStd::string& icon = "");

        void InitializeResourceMappingToolAction();
        void InitializeAWSDocActions();
        void InitializeAWSGlobalDocsSubMenu();
        void InitializeAWSFeatureGemActions();
        void AddSpaceForIcon(QMenu* menu);

        // AWSCoreEditorRequestBus interface implementation
        void SetAWSClientAuthEnabled() override;
        void SetAWSMetricsEnabled() override;

        QMenu* SetAWSFeatureSubMenu(const AZStd::string& menuText);

        // To improve experience, use process watcher to keep track of ongoing tool process
        AZStd::unique_ptr<AzFramework::ProcessWatcher> m_resourceMappingToolWatcher;
    };
} // namespace AWSCore
