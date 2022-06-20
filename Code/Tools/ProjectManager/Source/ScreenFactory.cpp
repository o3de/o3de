/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ScreenFactory.h>

#include <CreateProjectCtrl.h>
#include <UpdateProjectCtrl.h>
#include <NewProjectSettingsScreen.h>
#include <GemCatalog/GemCatalogScreen.h>
#include <ProjectsScreen.h>
#include <UpdateProjectSettingsScreen.h>
#include <EngineScreenCtrl.h>
#include <EngineSettingsScreen.h>
#include <GemRepo/GemRepoScreen.h>

namespace O3DE::ProjectManager
{
    ScreenWidget* BuildScreen(QWidget* parent, ProjectManagerScreen screen)
    {
        ScreenWidget* newScreen;

        switch(screen)
        {
        case (ProjectManagerScreen::CreateProject):
            newScreen = new CreateProjectCtrl(parent);
            break;
        case (ProjectManagerScreen::NewProjectSettings):
            newScreen = new NewProjectSettingsScreen(parent);
            break;
        case (ProjectManagerScreen::GemCatalog):
            newScreen = new GemCatalogScreen(parent);
            break;
        case (ProjectManagerScreen::Projects):
            newScreen = new ProjectsScreen(parent);
            break;
        case (ProjectManagerScreen::UpdateProject):
            newScreen = new UpdateProjectCtrl(parent);
            break;
        case (ProjectManagerScreen::UpdateProjectSettings):
            newScreen = new UpdateProjectSettingsScreen(parent);
            break;
        case (ProjectManagerScreen::Engine):
            newScreen = new EngineScreenCtrl(parent);
            break;
        case (ProjectManagerScreen::EngineSettings):
            newScreen = new EngineSettingsScreen(parent);
            break;
        case (ProjectManagerScreen::GemRepos):
            newScreen = new GemRepoScreen(parent);
            break;
        case (ProjectManagerScreen::Empty):
        default:
            newScreen = new ScreenWidget(parent);
        }

        return newScreen;
    }
} // namespace O3DE::ProjectManager
