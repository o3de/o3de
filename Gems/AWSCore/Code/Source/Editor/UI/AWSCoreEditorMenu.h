/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

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
        ~AWSCoreEditorMenu() override;

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
        void SetAWSGameLiftEnabled() override;

        QMenu* SetAWSFeatureSubMenu(const AZStd::string& menuText);

        // To improve experience, use process watcher to keep track of ongoing tool process
        AZStd::unique_ptr<AzFramework::ProcessWatcher> m_resourceMappingToolWatcher;
    };
} // namespace AWSCore
