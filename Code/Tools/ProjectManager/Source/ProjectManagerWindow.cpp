/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectManagerWindow.h>
#include <PythonBindingsInterface.h>
#include <ScreensCtrl.h>
#include <DownloadController.h>

namespace O3DE::ProjectManager
{
    ProjectManagerWindow::ProjectManagerWindow(QWidget* parent, const AZ::IO::PathView& projectPath, ProjectManagerScreen startScreen)
        : QMainWindow(parent)
    {
        if (auto engineInfoOutcome = PythonBindingsInterface::Get()->GetEngineInfo(); engineInfoOutcome)
        {
            auto engineInfo = engineInfoOutcome.GetValue<EngineInfo>();
            auto versionToDisplay = engineInfo.m_displayVersion == "00.00" ?
                                        engineInfo.m_version : engineInfo.m_displayVersion;
            setWindowTitle(QString("%1 %2 %3").arg(engineInfo.m_name.toUpper(), versionToDisplay, tr("Project Manager")));
        }
        else
        {
            setWindowTitle(QString("O3DE %1").arg(tr("Project Manager")));
        }

        m_downloadController = new DownloadController(this);

        ScreensCtrl* screensCtrl = new ScreensCtrl(nullptr, m_downloadController);

        // currently the tab order on the home page is based on the order of this list
        QVector<ProjectManagerScreen> screenEnums =
        {
            ProjectManagerScreen::Projects,
            ProjectManagerScreen::CreateGem,
            ProjectManagerScreen::EditGem,
            ProjectManagerScreen::GemCatalog,
            ProjectManagerScreen::Engine,
            ProjectManagerScreen::CreateProject,
            ProjectManagerScreen::UpdateProject,
            ProjectManagerScreen::GemsGemRepos
        };
        screensCtrl->BuildScreens(screenEnums);

        setCentralWidget(screensCtrl);

        // Projects is the default first screen because it is first in the above order
        if (startScreen != ProjectManagerScreen::Projects)
        {
            // always push the projects screen first so we have something to come back to
            screensCtrl->ForceChangeToScreen(ProjectManagerScreen::Projects);
            screensCtrl->ForceChangeToScreen(startScreen);
        }

        if (!projectPath.empty())
        {
            const QString path = QString::fromUtf8(projectPath.Native().data(), aznumeric_cast<int>(projectPath.Native().size()));
            emit screensCtrl->NotifyCurrentProject(path);
        }
    }
} // namespace O3DE::ProjectManager
