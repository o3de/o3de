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

#include <ProjectButtonWidget.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSpacerItem>

#include <PythonBindingsInterface.h>

namespace O3DE::ProjectManager
{
    ProjectsHomeScreen::ProjectsHomeScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        this->setLayout(vLayout);
        vLayout->setContentsMargins(80, 80, 80, 80);

        QHBoxLayout* topLayout = new QHBoxLayout();

        QLabel* titleLabel = new QLabel(this);
        titleLabel->setText("My Projects");
        titleLabel->setStyleSheet("font-size: 24px; font-family: 'Open Sans';");
        topLayout->addWidget(titleLabel);

        QSpacerItem* topSpacer = new QSpacerItem(20, 40, QSizePolicy::Expanding, QSizePolicy::Minimum);
        topLayout->addItem(topSpacer);

        QMenu* newProjectMenu = new QMenu(this);
        m_createNewProjectAction = newProjectMenu->addAction("Create New Project");
        m_addExistingProjectAction = newProjectMenu->addAction("Add Existing Project");

        QPushButton* newProjectMenuButton = new QPushButton(this);
        newProjectMenuButton->setText("New Project");
        newProjectMenuButton->setMenu(newProjectMenu);
        topLayout->addWidget(newProjectMenuButton);

        vLayout->addLayout(topLayout);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(30);

        ProjectButton* tempProjectButton1 = new ProjectButton("Project 1", this);
        buttonLayout->addWidget(tempProjectButton1);

        ProjectButton* tempProjectButton2 = new ProjectButton("Project 2", this);
        buttonLayout->addWidget(tempProjectButton2);

        QSpacerItem* buttonSpacer = new QSpacerItem(20, 40, QSizePolicy::Expanding, QSizePolicy::Minimum);
        buttonLayout->addItem(buttonSpacer);

        vLayout->addLayout(buttonLayout);

        //QSpacerItem* verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        //vLayout->addItem(verticalSpacer);

        // Using border-image allows for scaling options background-image does not support
        setStyleSheet("O3DE--ProjectManager--ScreenWidget { border-image: url(:/Resources/FirstTimeBackgroundImage.jpg) repeat repeat; }");

        connect(m_createNewProjectAction, &QAction::triggered, this, &ProjectsHomeScreen::HandleNewProjectButton);
        connect(m_addExistingProjectAction, &QAction::triggered, this, &ProjectsHomeScreen::HandleAddProjectButton);
        //connect(m_ui->editProjectButton, &QPushButton::pressed, this, &ProjectsHomeScreen::HandleEditProjectButton);
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
