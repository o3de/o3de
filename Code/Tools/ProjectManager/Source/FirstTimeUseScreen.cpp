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

#include <FirstTimeUseScreen.h>

#include <Source/ui_FirstTimeUseScreen.h>

namespace O3DE::ProjectManager
{
    FirstTimeUseScreen::FirstTimeUseScreen(QWidget* parent)
        : ScreenWidget(parent)
        , m_ui(new Ui::FirstTimeUseClass())
    {
        m_ui->setupUi(this);

        QObject::connect(m_ui->createProjectButton, &QPushButton::pressed, this, &FirstTimeUseScreen::HandleNewProjectButton);
        QObject::connect(m_ui->openProjectButton, &QPushButton::pressed, this, &FirstTimeUseScreen::HandleOpenProjectButton);
    }

    void FirstTimeUseScreen::HandleNewProjectButton()
    {
        emit ChangeScreenRequest(ProjectManagerScreen::NewProjectSettings);
    }
    void FirstTimeUseScreen::HandleOpenProjectButton()
    {
        emit ChangeScreenRequest(ProjectManagerScreen::ProjectsHome);
    }

} // namespace O3DE::ProjectManager
