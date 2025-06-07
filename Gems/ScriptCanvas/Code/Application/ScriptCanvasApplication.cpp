/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasApplication.h"

#include <AzFramework/Network/IRemoteTools.h>
#include <AzFramework/Script/ScriptRemoteDebuggingConstants.h>
#include <QApplication>
#include <QIcon>

#if defined(EXTERNAL_CRASH_REPORTING)
#include <ToolsCrashHandler.h>
#endif

// This has to live outside of any namespaces due to issues on Linux with calls to Q_INIT_RESOURCE if they are inside a namespace
void InitScriptCanvasApplicationResources()
{
    Q_INIT_RESOURCE(ScriptCanvasApplicationResources);
}

namespace ScriptCanvas
{
    static const char* GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return LY_CMAKE_TARGET;
    }

    ScriptCanvasApplication::ScriptCanvasApplication(int* argc, char*** argv)
        : Base(GetBuildTargetName(), argc, argv)
    {
#if defined(EXTERNAL_CRASH_REPORTING)
        CrashHandler::ToolsCrashHandler::InitCrashHandler(GetBuildTargetName(), {});
#endif

        InitScriptCanvasApplicationResources();

        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("O3DE Script Canvas");
        QApplication::setWindowIcon(QIcon(":/Resources/application.svg"));

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    ScriptCanvasApplication::~ScriptCanvasApplication()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
    }

    void ScriptCanvasApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        InitMainWindow();

#if defined(ENABLE_REMOTE_TOOLS)
        if (auto* remoteToolsInterface = AzFramework::RemoteToolsInterface::Get())
        {
            remoteToolsInterface->RegisterToolingServiceHost(
                AzFramework::ScriptCanvasToolsKey, AzFramework::ScriptCanvasToolsName, AzFramework::ScriptCanvasToolsPort);
        }
#endif
    }

    void ScriptCanvasApplication::Destroy()
    {
        m_window.reset();
        Base::Destroy();
    }

    QWidget* ScriptCanvasApplication::GetAppMainWindow()
    {
        return m_window.get();
    }

    void ScriptCanvasApplication::InitMainWindow()
    {
        m_window.reset(aznew ScriptCanvasEditor::MainWindow(nullptr));
        m_window->show();
    }

} // namespace ScriptCanvas
