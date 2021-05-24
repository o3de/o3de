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
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QListView>
#include <QSpacerItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFileInfo>

namespace O3DE::ProjectManager
{
    inline constexpr static int s_contentMargins = 80;
    inline constexpr static int s_projectButtonSpacing = 30;
    inline constexpr static int s_spacerSize = 20;
    inline constexpr static int s_newProjectButtonWidth = 156;
    inline constexpr static int s_projectListAdditionalHeight = 40;

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

        QSpacerItem* topVerticalSpacer = new QSpacerItem(s_spacerSize, s_spacerSize, QSizePolicy::Minimum, QSizePolicy::Expanding);
        vLayout->addItem(topVerticalSpacer);

        // Get all projects and create a horizontal scrolling list of them
        auto projectsResult = PythonBindingsInterface::Get()->GetProjects();
        if (projectsResult.IsSuccess() && !projectsResult.GetValue().isEmpty())
        {
            QListWidget* projectsButtonList = new QListWidget;
            projectsButtonList->setFlow(QListView::Flow::LeftToRight);
            projectsButtonList->setSpacing(s_projectButtonSpacing);
            projectsButtonList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            for (auto project : projectsResult.GetValue())
            {
                QListWidgetItem* projectListItem;
                projectListItem = new QListWidgetItem(projectsButtonList);
                projectListItem->setBackground(Qt::transparent);
                projectsButtonList->addItem(projectListItem);
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
                projectListItem->setSizeHint(projectButton->minimumSizeHint());
                projectsButtonList->setItemWidget(projectListItem, projectButton);

                connect(projectButton, &ProjectButton::OpenProject, this, &ProjectsHomeScreen::HandleOpenProject);
                connect(projectButton, &ProjectButton::EditProject, this, &ProjectsHomeScreen::HandleEditProject);

#ifdef SHOW_ALL_PROJECT_ACTIONS
                connect(projectButton, &ProjectButton::EditProjectGems, this, &ProjectsHomeScreen::HandleEditProjectGems);
                connect(projectButton, &ProjectButton::CopyProject, this, &ProjectsHomeScreen::HandleCopyProject);
                connect(projectButton, &ProjectButton::RemoveProject, this, &ProjectsHomeScreen::HandleRemoveProject);
                connect(projectButton, &ProjectButton::DeleteProject, this, &ProjectsHomeScreen::HandleDeleteProject);
#endif
            }

            projectsButtonList->setFixedHeight(projectsButtonList->sizeHintForRow(0) + s_projectListAdditionalHeight);
            vLayout->addWidget(projectsButtonList);
        }

        QSpacerItem* bottomVerticalSpacer = new QSpacerItem(s_spacerSize, s_spacerSize, QSizePolicy::Minimum, QSizePolicy::Expanding);
        vLayout->addItem(bottomVerticalSpacer);

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
        emit ResetScreenRequest(ProjectManagerScreen::NewProjectSettingsCore);
        emit ChangeScreenRequest(ProjectManagerScreen::NewProjectSettingsCore);
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
        emit ResetScreenRequest(ProjectManagerScreen::ProjectSettingsCore);
        emit ChangeScreenRequest(ProjectManagerScreen::ProjectSettingsCore);
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
