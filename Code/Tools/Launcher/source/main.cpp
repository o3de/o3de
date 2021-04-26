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

#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/Path/Path.h>

#include <Qt/LauncherWindow.h>

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>

int main(int argc, char* argv[])
{
    QApplication::setOrganizationName("Amazon");
    QApplication::setOrganizationDomain("amazon.com");
    QCoreApplication::setApplicationName("ProjectLauncher");
    QCoreApplication::setApplicationVersion("1.0");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::SystemDpiAware);

    QApplication app(argc, argv);

    AZ::IO::FixedMaxPath engineRootPath;
    {
        AZ::ComponentApplication componentApplication;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }

    AzQtComponents::StyleManager styleManager(&app);
    styleManager.initialize(&app, engineRootPath);

    ProjectLauncher::LauncherWindow window(nullptr, engineRootPath);
    window.show();

    return app.exec();
}
