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
        static constexpr const char AWSResourceMappingToolActionText[] = "AWS Resource Mapping Tool...";
        static constexpr const char CredentialConfigurationActionText[] = "Credential Configuration";
        static constexpr const char CredentialConfigurationUrl[] = "https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/credentials.html";
        static constexpr const char NewToAWSActionText[] = "New to AWS?";
        static constexpr const char NewToAWSUrl[] = "https://o3deorg.netlify.app/docs/user-guide/gems/reference/aws";
        static constexpr const char AWSAndScriptCanvasActionText[] = "AWS && ScriptCanvas";
        static constexpr const char AWSAndScriptCanvasUrl[] = "https://o3deorg.netlify.app/docs/user-guide/gems/reference/aws";
        static constexpr const char AWSClientAuthActionText[] = "Client Auth";
        static constexpr const char AWSMetricsActionText[] = "Metrics";

        AWSCoreEditorMenu(const QString& text);
        ~AWSCoreEditorMenu();

    private:
        void InitializeResourceMappingToolAction();
        void InitializeAWSDocActions();
        void InitializeAWSFeatureGemActions();

        // AWSCoreEditorRequestBus interface implementation
        void SetAWSClientAuthEnabled() override;
        void SetAWSMetricsEnabled() override;

        void SetAWSFeatureActionsEnabled(const AZStd::string actionText);

        // To improve experience, use process watcher to keep track of ongoing tool process
        AZStd::unique_ptr<AzFramework::ProcessWatcher> m_resourceMappingToolWatcher;
    };
} // namespace AWSCore
