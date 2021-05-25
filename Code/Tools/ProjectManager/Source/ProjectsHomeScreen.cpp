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
#include <PythonBindingsInterface.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QListView>
#include <QSpacerItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFileInfo>
#include <QScrollArea>

namespace O3DE::ProjectManager
{
    inline constexpr static int s_contentMargins = 80;
    inline constexpr static int s_spacerSize = 20;
    inline constexpr static int s_projectButtonRowCount = 4;
    inline constexpr static int s_newProjectButtonWidth = 156;

    static QString s_projectPreviewImagePath = "/preview.png";

    ProjectsHomeScreen::ProjectsHomeScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        setLayout(vLayout);
        vLayout->setContentsMargins(s_contentMargins, s_contentMargins, s_contentMargins, s_contentMargins);

        QHBoxLayout* topLayout = new QHBoxLayout();

        QLabel* titleLabel = new QLabel(this);
        titleLabel->setText("My Projects");
        titleLabel->setStyleSheet("font-size: 24px");
        topLayout->addWidget(titleLabel);

        QSpacerItem* topSpacer = new QSpacerItem(s_spacerSize, s_spacerSize, QSizePolicy::Expanding, QSizePolicy::Minimum);
        topLayout->addItem(topSpacer);

        QMenu* newProjectMenu = new QMenu(this);
        m_createNewProjectAction = newProjectMenu->addAction("Create New Project");
        m_addExistingProjectAction = newProjectMenu->addAction("Add Existing Project");

        QPushButton* newProjectMenuButton = new QPushButton(this);
        newProjectMenuButton->setText("New Project...");
        newProjectMenuButton->setMenu(newProjectMenu);
        newProjectMenuButton->setFixedWidth(s_newProjectButtonWidth);
        newProjectMenuButton->setStyleSheet("font-size: 14px;");
        topLayout->addWidget(newProjectMenuButton);

        vLayout->addLayout(topLayout);

        // Get all projects and create a horizontal scrolling list of them
        auto projectsResult = PythonBindingsInterface::Get()->GetProjects();
        if (projectsResult.IsSuccess() && !projectsResult.GetValue().isEmpty())
        {
            QScrollArea* projectsScrollArea = new QScrollArea(this);
            QWidget* scrollWidget = new QWidget();
            QGridLayout* projectGridLayout = new QGridLayout();
            scrollWidget->setLayout(projectGridLayout);
            projectsScrollArea->setWidget(scrollWidget);
            projectsScrollArea->setWidgetResizable(true);

            int gridIndex = 0;
            for (auto project : projectsResult.GetValue())
            {
                ProjectButton* projectButton;
                QString projectPreviewPath = project.m_path + s_projectPreviewImagePath;
                QFileInfo doesPreviewExist(projectPreviewPath);
                if (doesPreviewExist.exists() && doesPreviewExist.isFile())
                {
                    projectButton = new ProjectButton(project.m_projectName, projectPreviewPath, this);
                }
                else
                {
                    projectButton = new ProjectButton(project.m_projectName, this);
                }

                // Create rows of projects buttons s_projectButtonRowCount buttons wide
                projectGridLayout->addWidget(projectButton, gridIndex / s_projectButtonRowCount, gridIndex % s_projectButtonRowCount);

                connect(projectButton, &ProjectButton::OpenProject, this, &ProjectsHomeScreen::HandleOpenProject);
                connect(projectButton, &ProjectButton::EditProject, this, &ProjectsHomeScreen::HandleEditProject);

#ifdef SHOW_ALL_PROJECT_ACTIONS
                connect(projectButton, &ProjectButton::EditProjectGems, this, &ProjectsHomeScreen::HandleEditProjectGems);
                connect(projectButton, &ProjectButton::CopyProject, this, &ProjectsHomeScreen::HandleCopyProject);
                connect(projectButton, &ProjectButton::RemoveProject, this, &ProjectsHomeScreen::HandleRemoveProject);
                connect(projectButton, &ProjectButton::DeleteProject, this, &ProjectsHomeScreen::HandleDeleteProject);
#endif
                ++gridIndex;
            }

            vLayout->addWidget(projectsScrollArea);
        }

        // Using border-image allows for scaling options background-image does not support
        setStyleSheet("O3DE--ProjectManager--ScreenWidget { border-image: url(:/Resources/Backgrounds/FirstTimeBackgroundImage.jpg) repeat repeat; }");

        connect(m_createNewProjectAction, &QAction::triggered, this, &ProjectsHomeScreen::HandleNewProjectButton);
        connect(m_addExistingProjectAction, &QAction::triggered, this, &ProjectsHomeScreen::HandleAddProjectButton);
    }

    ProjectManagerScreen ProjectsHomeScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::ProjectsHome;
    }

    void ProjectsHomeScreen::HandleNewProjectButton()
    {
        emit ResetScreenRequest(ProjectManagerScreen::CreateProject);
        emit ChangeScreenRequest(ProjectManagerScreen::CreateProject);
    }
    void ProjectsHomeScreen::HandleAddProjectButton()
    {
        // Do nothing for now
    }
    void ProjectsHomeScreen::HandleOpenProject(const QString& projectPath)
    {
        // Open the editor with this project open
        emit NotifyCurrentProject(projectPath);
    }
    void ProjectsHomeScreen::HandleEditProject(const QString& projectPath)
    {
        emit NotifyCurrentProject(projectPath);
        emit ResetScreenRequest(ProjectManagerScreen::UpdateProject);
        emit ChangeScreenRequest(ProjectManagerScreen::UpdateProject);
    }
    void ProjectsHomeScreen::HandleEditProjectGems(const QString& projectPath)
    {
        emit NotifyCurrentProject(projectPath);
        emit ChangeScreenRequest(ProjectManagerScreen::GemCatalog);
    }
    void ProjectsHomeScreen::HandleCopyProject([[maybe_unused]] const QString& projectPath)
    {
        // Open file dialog and choose location for copied project then register copy with O3DE
    }
    void ProjectsHomeScreen::HandleRemoveProject([[maybe_unused]] const QString& projectPath)
    {
        // Unregister Project from O3DE 
    }
    void ProjectsHomeScreen::HandleDeleteProject([[maybe_unused]] const QString& projectPath)
    {
        // Remove project from 03DE and delete from disk
        ProjectsHomeScreen::HandleRemoveProject(projectPath);
    }

} // namespace O3DE::ProjectManager
