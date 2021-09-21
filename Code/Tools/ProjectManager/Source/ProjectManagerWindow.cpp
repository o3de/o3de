/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectManagerWindow.h>
#include <ScreensCtrl.h>

namespace O3DE::ProjectManager
{
    ProjectManagerWindow::ProjectManagerWindow(QWidget* parent, const AZ::IO::PathView& projectPath, ProjectManagerScreen startScreen)
        : QMainWindow(parent)
    {
        setWindowTitle(tr("O3DE Project Manager"));

        ScreensCtrl* screensCtrl = new ScreensCtrl();

        // currently the tab order on the home page is based on the order of this list
        QVector<ProjectManagerScreen> screenEnums =
        {
            ProjectManagerScreen::Projects,
            ProjectManagerScreen::Engine,
            ProjectManagerScreen::CreateProject,
            ProjectManagerScreen::UpdateProject
        };
        screensCtrl->BuildScreens(screenEnums);

        setCentralWidget(screensCtrl);

        // always push the projects screen first so we have something to come back to
        if (startScreen != ProjectManagerScreen::Projects)
        {
            screensCtrl->ForceChangeToScreen(ProjectManagerScreen::Projects);
        }
        screensCtrl->ForceChangeToScreen(startScreen);

        if (!projectPath.empty())
        {
            const QString path = QString::fromUtf8(projectPath.Native().data(), aznumeric_cast<int>(projectPath.Native().size()));
            emit screensCtrl->NotifyCurrentProject(path);
        }
    }
} // namespace O3DE::ProjectManager
