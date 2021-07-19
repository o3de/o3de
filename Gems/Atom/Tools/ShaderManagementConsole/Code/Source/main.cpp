/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/GlobalEventFilter.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/O3DEStylesheet.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Application/AzQtApplication.h>

#include <QtWidgets/QApplication>
#include <QtGui/private/qhighdpiscaling_p.h>

#include <Source/ShaderManagementConsoleApplication.h>

int main(int argc, char** argv)
{
    AzQtComponents::AzQtApplication::InitializeDpiScaling();

    ShaderManagementConsole::ShaderManagementConsoleApplication app(&argc, &argv);

    AZ::IO::FixedMaxPath engineRootPath;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }

    auto globalEventFilter = new AzQtComponents::GlobalEventFilter(&app);
    app.installEventFilter(globalEventFilter);

    AzQtComponents::StyleManager styleManager(&app);
    styleManager.initialize(&app, engineRootPath);

    app.Start({});
    app.exec();
    app.Stop();
    return 0;
}
