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

#include <ProjectsHomeScreen.h>

#include <Source/ui_ProjectsHomeScreen.h>

#include <PythonBindingsInterface.h>

namespace O3DE::ProjectManager
{
    ProjectsHomeScreen::ProjectsHomeScreen(QWidget* parent)
        : ScreenWidget(parent)
        , m_ui(new Ui::ProjectsHomeClass())
    {
        m_ui->setupUi(this);

        QObject::connect(m_ui->newProjectButton, &QPushButton::pressed, this, &ProjectsHomeScreen::HandleNewProjectButton);
        QObject::connect(m_ui->addProjectButton, &QPushButton::pressed, this, &ProjectsHomeScreen::HandleAddProjectButton);
        QObject::connect(m_ui->editProjectButton, &QPushButton::pressed, this, &ProjectsHomeScreen::HandleEditProjectButton);

        // example of how to get the current project name
        //ProjectInfo currentProject = PythonBindingsInterface::Get()->GetCurrentProject();
    }

    ProjectManagerScreen ProjectsHomeScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::ProjectsHome;
    }

    void ProjectsHomeScreen::HandleNewProjectButton()
    {
        emit ResetScreenRequest(ProjectManagerScreen::NewProjectSettingsCore);
        emit ChangeScreenRequest(ProjectManagerScreen::NewProjectSettingsCore);
    }
    void ProjectsHomeScreen::HandleAddProjectButton()
    {
        // Do nothing for now
    }
    void ProjectsHomeScreen::HandleEditProjectButton()
    {
        emit ChangeScreenRequest(ProjectManagerScreen::ProjectSettings);
    }

} // namespace O3DE::ProjectManager
