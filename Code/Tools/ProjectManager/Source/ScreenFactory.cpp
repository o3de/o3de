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
#include <ProjectGemCatalogScreen.h>
#include <UpdateProjectSettingsScreen.h>
#include <GemsGemRepoScreen.h>
#include <EngineScreenCtrl.h>
#include <EngineSettingsScreen.h>
#include <GemRepo/GemRepoScreen.h>
#include <CreateAGemScreen.h>
#include <EditAGemScreen.h>
#include <DownloadController.h>

namespace O3DE::ProjectManager
{
    ScreenWidget* BuildScreen(QWidget* parent, ProjectManagerScreen screen, DownloadController* downloadController)
    {
        ScreenWidget* newScreen;

        switch(screen)
        {
        case (ProjectManagerScreen::CreateProject):
            newScreen = new CreateProjectCtrl(downloadController, parent);
            break;
        case (ProjectManagerScreen::NewProjectSettings):
            newScreen = new NewProjectSettingsScreen(downloadController, parent);
            break;
        case (ProjectManagerScreen::GemCatalog):
            newScreen = new GemCatalogScreen(downloadController, true, parent);
            break;
        case (ProjectManagerScreen::ProjectGemCatalog):
            newScreen = new ProjectGemCatalogScreen(downloadController, parent);
            break;
        case (ProjectManagerScreen::Projects):
            newScreen = new ProjectsScreen(downloadController, parent);
            break;
        case (ProjectManagerScreen::UpdateProject):
            newScreen = new UpdateProjectCtrl(downloadController, parent);
            break;
        case (ProjectManagerScreen::UpdateProjectSettings):
            newScreen = new UpdateProjectSettingsScreen(parent);
            break;
        case (ProjectManagerScreen::GemsGemRepos):
            newScreen = new GemsGemRepoScreen(parent);
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
        case (ProjectManagerScreen::CreateGem):
            newScreen = new CreateGem(parent);
            break;
        case (ProjectManagerScreen::EditGem):
            newScreen = new EditGem(parent);
            break;
        case (ProjectManagerScreen::Empty):
        default:
            newScreen = new ScreenWidget(parent);
        }

        //handle any code that needs to run after construction but before startup 
        newScreen->Init();

        return newScreen;
    }
} // namespace O3DE::ProjectManager
