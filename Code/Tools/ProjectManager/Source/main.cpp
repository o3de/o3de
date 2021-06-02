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
#include <AzFramework/CommandLine/CommandLine.h>

#include <ProjectManagerWindow.h>
#include <ProjectUtils.h>

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>

using namespace O3DE::ProjectManager;

int main(int argc, char* argv[])
{
    QApplication::setOrganizationName("O3DE");
    QApplication::setOrganizationDomain("o3de.org");
    QCoreApplication::setApplicationName("ProjectManager");
    QCoreApplication::setApplicationVersion("1.0");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::SystemDpiAware);

    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    int runSuccess = 0;
    {
        QApplication app(argc, argv);

        // Need to use settings registry to get EngineRootFolder
        AZ::IO::FixedMaxPath engineRootPath;
        {
            AZ::ComponentApplication componentApplication;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        }

        AzQtComponents::StyleManager styleManager(&app);
        styleManager.initialize(&app, engineRootPath);

        // Get the initial start screen if one is provided via command line
        constexpr char optionPrefix[] = "--";
        AZ::CommandLine commandLine(optionPrefix);
        commandLine.Parse(argc, argv);

        ProjectManagerScreen startScreen = ProjectManagerScreen::Projects;
        if(commandLine.HasSwitch("screen"))
        {
            QString screenOption = commandLine.GetSwitchValue("screen", 0).c_str();
            ProjectManagerScreen screen = ProjectUtils::GetProjectManagerScreen(screenOption);
            if (screen != ProjectManagerScreen::Invalid)
            {
                startScreen = screen;
            }
        }

        AZ::IO::FixedMaxPath projectPath;
        if (commandLine.HasSwitch("project-path"))
        {
            projectPath = commandLine.GetSwitchValue("project-path", 0).c_str();
        }

        ProjectManagerWindow window(nullptr, engineRootPath, projectPath, startScreen);
        window.show();

        // somethings is preventing us from moving the window to the center of the
        // primary screen - likely an Az style or component helper
        constexpr int width = 1200;
        constexpr int height = 800;
        window.resize(width, height);

        runSuccess = app.exec();
    }
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

    return runSuccess;
}
