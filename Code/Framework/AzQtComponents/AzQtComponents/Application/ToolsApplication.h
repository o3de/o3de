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

#include <QApplication>
#include <QTimer>
#include <QMainWindow>

#include <AzFramework/Application/Application.h>

#include <AzQtComponents/Components/GlobalEventFilter.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/O3DEStylesheet.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>


#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Logging/LogFile.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Archive/IArchive.h>
#include <AzFramework/Archive/ArchiveFileIO.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/NativeUI/NativeUISystemComponent.h>

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ToolsApplication
        : public QApplication
    {
    public:       
        ToolsApplication(int& argc, char** argv);
        ~ToolsApplication();
        
    private:
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        class Impl;
        AZStd::unique_ptr<Impl> m_impl;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

        //QTimer m_timer;
        //void Tick(float deltaOverride = -1.f) override;


        /*
        bool AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area, uint32_t orientation);
        void RemoveDockWidget(const AZStd::string& name);
        void SetDockWidgetVisible(const AZStd::string& name, bool visible);
        bool IsDockWidgetVisible(const AZStd::string& name) const;
        AZStd::vector<AZStd::string> GetDockWidgetNames() const;

        AZStd::unordered_map<AZStd::string, AzQtComponents::StyledDockWidget*> m_dockWidgets;
        */
    };
} // namespace AzQtComponents


