/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasApplication.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <QApplication>
#include <QPointer>
#include <View/Windows/MainWindow.h>

// Flow similar to CryEditMain
int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;

    AzQtComponents::PrepareQtPaths();
    ScriptCanvas::ScriptCanvasQtApplication::InitializeDpiScaling();
    ScriptCanvas::ScriptCanvasQtApplication* qtApp = aznew ScriptCanvas::ScriptCanvasQtApplication(argc, argv);

    int ret = 0;

    // Scope for ToolsApp
    {
        ScriptCanvas::ScriptCanvasToolsApplication AZToolsApp(&argc, &argv, {});

        // The settings registry has been created by the AZ::ComponentApplication constructor at this point
        // Merge settings is needed for proper dependency resolution
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        AZStd::string_view buildTargetName = AZStd::string_view{ LY_CMAKE_TARGET };
        AZ::SettingsRegistryInterface& registry = *AZ::SettingsRegistry::Get();
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(registry, buildTargetName);

        AZ::IO::FixedMaxPath engineRootPath;
        {
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        }

        AzQtComponents::StyleManager styleManager(qtApp);
        styleManager.initialize(qtApp, engineRootPath);

        AZToolsApp.Start(AzFramework::Application::Descriptor());

        QPointer<ScriptCanvasEditor::MainWindow> mainWindow = aznew ScriptCanvasEditor::MainWindow(nullptr);

        mainWindow->show();

        ret = qtApp->exec();
    }

    delete qtApp;

    return ret;
}
