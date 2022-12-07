/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AWSCoreBus.h>

namespace AzFramework
{
    class ProcessWatcher;
}

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
    class MenuManagerInternalInterface;
}

namespace AWSCore
{
    class AWSCoreEditorMenu
    {
    public:
        static constexpr const char AWSResourceMappingToolReadMeWarningText[] =
            "Failed to launch Resource Mapping Tool, please follow <a href=\"file:///%s\">README</a> to setup tool before using it.";
        static constexpr const char AWSResourceMappingToolIsRunningText[] = "Resource Mapping Tool is running...";
        static constexpr const char AWSResourceMappingToolLogWarningText[] =
            "Failed to launch Resource Mapping Tool, please check <a href=\"file:///%s\">logs</a> for details.";

        AWSCoreEditorMenu();
        ~AWSCoreEditorMenu();

        void UpdateMenuBinding();

    private:

        void InitializeResourceMappingToolAction();
        void InitializeAWSDocActions();
        void InitializeAWSGlobalDocsSubMenu();

        // To improve experience, use process watcher to keep track of ongoing tool process
        AZStd::unique_ptr<AzFramework::ProcessWatcher> m_resourceMappingToolWatcher;

        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInternalInterface* m_menuManagerInternalInterface = nullptr;

    };
} // namespace AWSCore
