/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <MaterialEditorApplication.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/GlobalEventFilter.h>
#include <AzQtComponents/Components/O3DEStylesheet.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtWidgets/QApplication>
#endif

int main(int argc, char** argv)
{
    QApplication::setOrganizationName("O3DE");
    QApplication::setOrganizationDomain("o3de.org");
    QApplication::setApplicationName("O3DE Material Editor");

    AzQtComponents::PrepareQtPaths();

    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    // Must be set before QApplication is initialized, so that we support HighDpi monitors, like the Retina displays
    // on Windows 10
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::SystemDpiAware);

    MaterialEditor::MaterialEditorApplication app(&argc, &argv);

    auto globalEventFilter = new AzQtComponents::GlobalEventFilter(&app);
    app.installEventFilter(globalEventFilter);

    AZ::IO::FixedMaxPath engineRootPath;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }

    AzQtComponents::StyleManager styleManager(&app);
    styleManager.initialize(&app, engineRootPath);

    app.Start(AZ::ComponentApplication::Descriptor{});
    app.exec();
    app.Stop();
    return 0;
}
