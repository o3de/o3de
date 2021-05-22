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

#include <ProjectManagerWindow.h>
#include <ScreensCtrl.h>

#include <AzQtComponents/Components/StyleManager.h>
#include <AzCore/IO/Path/Path.h>

#include <QDir>

namespace O3DE::ProjectManager
{
    ProjectManagerWindow::ProjectManagerWindow(QWidget* parent, const AZ::IO::PathView& engineRootPath)
        : QMainWindow(parent)
    {
        m_pythonBindings = AZStd::make_unique<PythonBindings>(engineRootPath);

        setWindowTitle(tr("O3DE Project Manager"));

        ScreensCtrl* screensCtrl = new ScreensCtrl();

        // currently the tab order on the home page is based on the order of this list
        QVector<ProjectManagerScreen> screenEnums =
        {
            ProjectManagerScreen::NewProjectSettingsCore,
            ProjectManagerScreen::ProjectsHome,
            ProjectManagerScreen::ProjectSettings,
            ProjectManagerScreen::EngineSettings,
            ProjectManagerScreen::FirstTimeUse 
        };
        screensCtrl->BuildScreens(screenEnums);

        setCentralWidget(screensCtrl);

        // setup stylesheets and hot reloading 
        QDir rootDir = QString::fromUtf8(engineRootPath.Native().data(), aznumeric_cast<int>(engineRootPath.Native().size()));
        const auto pathOnDisk = rootDir.absoluteFilePath("Code/Tools/ProjectManager/Resources");
        const auto qrcPath = QStringLiteral(":/ProjectManager/style");
        AzQtComponents::StyleManager::addSearchPaths("style", pathOnDisk, qrcPath, engineRootPath);

        // set stylesheet after creating the screens or their styles won't get updated
        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("style:ProjectManager.qss"));

        screensCtrl->ForceChangeToScreen(ProjectManagerScreen::FirstTimeUse, false);
    }

    ProjectManagerWindow::~ProjectManagerWindow()
    {
        m_pythonBindings.reset();
    }

} // namespace O3DE::ProjectManager
