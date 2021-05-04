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

#include <Qt/FirstTimeUse.h>

#include <Qt/ui_FirstTimeUse.h>

namespace O3de::ProjectManager
{
    FirstTimeUse::FirstTimeUse(ProjectManagerWindow* window)
        : ScreenWidget(window)
        , m_ui(new Ui::FirstTimeUseClass())
    {
        m_ui->setupUi(this);

        Setup();
    }

    FirstTimeUse::~FirstTimeUse()
    {
    }

    void FirstTimeUse::ConnectSlotsAndSignals()
    {
        QObject::connect(m_ui->createProjectButton, &QPushButton::pressed, this, &FirstTimeUse::HandleNewProjectButton);
        QObject::connect(m_ui->openProjectButton, &QPushButton::pressed, this, &FirstTimeUse::HandleOpenProjectButton);
    }

    void FirstTimeUse::HandleNewProjectButton()
    {
        m_projectManagerWindow->ChangeToScreen(ProjectManagerScreen::NewProjectSettings);
    }
    void FirstTimeUse::HandleOpenProjectButton()
    {
        m_projectManagerWindow->ChangeToScreen(ProjectManagerScreen::ProjectsHome);
    }

} // namespace O3de::ProjectManager
