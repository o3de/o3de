/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/LuaIDEApplication.h>

#include <AzQtComponents/Utilities/HandleDpiAwareness.h>

#include <QtCore/QCoreApplication>

#if defined(EXTERNAL_CRASH_REPORTING)
#include <ToolsCrashHandler.h>
#endif

#include <QApplication>
#include <QDir>
#include <QFileInfo>

// Editor.cpp : Defines the entry point for the application.

int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;
    // here we free the console (and close the console window) in release.

    int exitCode = 0;

    {
        AZStd::unique_ptr<AZ::IO::LocalFileIO> fileIO = AZStd::unique_ptr<AZ::IO::LocalFileIO>(aznew AZ::IO::LocalFileIO());
        AZ::IO::FileIOBase::SetInstance(fileIO.get());

        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
        AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::PerScreenDpiAware);

        LUAEditor::Application app(argc, argv);

        QString procName;
        {
            QCoreApplication qca(argc, argv);
            procName = QFileInfo(qca.applicationFilePath()).fileName();
        }

        LegacyFramework::ApplicationDesc desc(procName.toUtf8().data(), argc, argv);
        desc.m_applicationModule = NULL;
        desc.m_enableProjectManager = false;

#if defined(EXTERNAL_CRASH_REPORTING)
        CrashHandler::ToolsCrashHandler::InitCrashHandler("LuaEditor", {});
#endif

        exitCode = app.Run(desc);
        // this call will block until someone tells the core app to shut down via a bus message.
        // the bus message is usually sent (in gui mode) in response to pressing the quit button or something.
        // in an app that does not require GUI to be manufactured or use GUI windows, you should still call RUN
        // but make a component which does your processing, in response to RestoreState(). in the  CoreMessages bus.
        // RestoreState will always be called right before the main message pump activates.
        // and will then block until someone calls:
        /*UIFramework::FrameworkMessages::Bus::Broadcast(&UIFramework::FrameworkMessages::Bus::Events::UserWantsToQuit); */
        // so ideally to make a file processor or something, simply call app.Initialize(.... but with false as the gui mode ... )
        // and make at least one component which starts processing in response to CoreMessages::RestoreState(), and then sends UserWantsToQuit() once it has done its processing.
        // calling UserWantsToQuit will simply queue the quit, so its safe to call from any thread.
        // your components can query
        // FrameworkApplicationMessages::Bus::BroadcastResult(result, &FrameworkApplicationMessages::Bus::Events::IsRunningInGUIMode) to
        // determine if its in GUI mode or not.
    }

    return exitCode;
}
