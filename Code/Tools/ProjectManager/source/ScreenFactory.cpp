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

#include <Qt/FirstTimeUse.h>
#include <Qt/NewProjectSettings.h>
#include <Qt/GemCatalog.h>
#include <Qt/ProjectsHome.h>
#include <Qt/ProjectSettings.h>
#include <Qt/EngineSettings.h>

namespace O3de::ProjectManager
{
    QWidget* BuildScreen(ProjectManagerWindow* window, ProjectManagerScreen screen)
    {
        switch(screen)
        {
        case (ProjectManagerScreen::FirstTimeUse):
            return new FirstTimeUse(window);
        case (ProjectManagerScreen::NewProjectSettings):
            return new NewProjectSettings(window);
        case (ProjectManagerScreen::GemCatalog):
            return new GemCatalog(window);
        case (ProjectManagerScreen::ProjectsHome):
            return new ProjectsHome(window);
        case (ProjectManagerScreen::ProjectSettings):
            return new ProjectSettings(window);
        case (ProjectManagerScreen::EngineSettings):
            return new EngineSettings(window);
        default:
            return new QWidget(window->GetScreenStack());
        }
    }
} // namespace O3de::ProjectManager
