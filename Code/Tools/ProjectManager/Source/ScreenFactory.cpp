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
#include <ScreenFactory.h>

#include <FirstTimeUseScreen.h>
#include <NewProjectSettingsScreen.h>
#include <GemCatalog/GemCatalogScreen.h>
#include <ProjectsHomeScreen.h>
#include <ProjectSettingsScreen.h>
#include <EngineSettingsScreen.h>

namespace O3DE::ProjectManager
{
    ScreenWidget* BuildScreen(QWidget* parent, ProjectManagerScreen screen)
    {
        ScreenWidget* newScreen;

        switch(screen)
        {
        case (ProjectManagerScreen::FirstTimeUse):
            newScreen = new FirstTimeUseScreen(parent);
            break;
        case (ProjectManagerScreen::NewProjectSettings):
            newScreen = new NewProjectSettingsScreen(parent);
            break;
        case (ProjectManagerScreen::GemCatalog):
            newScreen = new GemCatalogScreen(parent);
            break;
        case (ProjectManagerScreen::ProjectsHome):
            newScreen = new ProjectsHomeScreen(parent);
            break;
        case (ProjectManagerScreen::ProjectSettings):
            newScreen = new ProjectSettingsScreen(parent);
            break;
        case (ProjectManagerScreen::EngineSettings):
            newScreen = new EngineSettingsScreen(parent);
            break;
        case (ProjectManagerScreen::Empty):
        default:
            newScreen = new ScreenWidget(parent);
        }

        return newScreen;
    }
} // namespace O3DE::ProjectManager
