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

#include <Qt/ProjectsHome.h>

#include <Qt/ui_ProjectsHome.h>

namespace ProjectManager
{
    ProjectsHome::ProjectsHome(ProjectManagerWindow* window)
        : ScreenWidget(window)
        , m_ui(new Ui::ProjectsHomeClass())
    {
        m_ui->setupUi(this);

        Setup();
    }

    ProjectsHome::~ProjectsHome()
    {
    }

    void ProjectsHome::ConnectSlotsAndSignals()
    {
        QObject::connect(m_ui->newProjectButton, &QPushButton::pressed, this, &ProjectsHome::HandleNewProjectButton);
        QObject::connect(m_ui->addProjectButton, &QPushButton::pressed, this, &ProjectsHome::HandleAddProjectButton);
        QObject::connect(m_ui->editProjectButton, &QPushButton::pressed, this, &ProjectsHome::HandleEditProjectButton);
    }

    void ProjectsHome::HandleNewProjectButton()
    {
        m_projectManagerWindow->ChangeToScreen(ProjectManagerScreen::NewProjectSettings);
    }
    void ProjectsHome::HandleAddProjectButton()
    {
        //Do nothing for now
    }
    void ProjectsHome::HandleEditProjectButton()
    {
        m_projectManagerWindow->ChangeToScreen(ProjectManagerScreen::ProjectSettings);
    }

} // namespace ProjectManager
