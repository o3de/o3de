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

#include <ProjectsScreen.h>

#include <ProjectButtonWidget.h>
#include <PythonBindingsInterface.h>
#include <ProjectUtils.h>

#include <AzQtComponents/Components/FlowLayout.h>
#include <AzCore/Platform.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/AzFramework_Traits_Platform.h>
#include <AzFramework/Process/ProcessCommon.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzCore/Utils/Utils.h>

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
#include <QScrollArea>
#include <QStackedWidget>
#include <QFrame>
#include <QIcon>
#include <QPixmap>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>

//#define DISPLAY_PROJECT_DEV_DATA true 

namespace O3DE::ProjectManager
{
    ProjectsScreen::ProjectsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignTop);
        vLayout->setContentsMargins(s_contentMargins, 0, s_contentMargins, 0);
        setLayout(vLayout);

        m_background.load(":/Backgrounds/FirstTimeBackgroundImage.jpg");

        m_stack = new QStackedWidget(this);

        m_firstTimeContent = CreateFirstTimeContent();
        m_stack->addWidget(m_firstTimeContent);

        m_projectsContent = CreateProjectsContent();
        m_stack->addWidget(m_projectsContent);

        vLayout->addWidget(m_stack);
    }

    QFrame* ProjectsScreen::CreateFirstTimeContent()
    {
        QFrame* frame = new QFrame(this);
        frame->setObjectName("firstTimeContent");
        {
            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignTop);
            frame->setLayout(layout);

            QLabel* titleLabel = new QLabel(tr("Ready. Set. Create."), this);
            titleLabel->setObjectName("titleLabel");
            layout->addWidget(titleLabel);

            QLabel* introLabel = new QLabel(this);
            introLabel->setObjectName("introLabel");
            introLabel->setText(tr("Welcome to O3DE! Start something new by creating a project. Not sure what to create? \nExplore what's "
                                   "available by downloading our sample project."));
            layout->addWidget(introLabel);

            QHBoxLayout* buttonLayout = new QHBoxLayout(this);
            buttonLayout->setAlignment(Qt::AlignLeft);
            buttonLayout->setSpacing(s_spacerSize);

            // use a newline to force the text up
            QPushButton* createProjectButton = new QPushButton(tr("Create a Project\n"), this);
            createProjectButton->setObjectName("createProjectButton");
            buttonLayout->addWidget(createProjectButton);

            QPushButton* addProjectButton = new QPushButton(tr("Add a Project\n"), this);
            addProjectButton->setObjectName("addProjectButton");
            buttonLayout->addWidget(addProjectButton);

            connect(createProjectButton, &QPushButton::clicked, this, &ProjectsScreen::HandleNewProjectButton);
            connect(addProjectButton, &QPushButton::clicked, this, &ProjectsScreen::HandleAddProjectButton);

            layout->addLayout(buttonLayout);
        }

        return frame;
    }

    QFrame* ProjectsScreen::CreateProjectsContent()
    {
        QFrame* frame = new QFrame(this);
        frame->setObjectName("projectsContent");
        {
            QVBoxLayout* layout = new QVBoxLayout();
            layout->setAlignment(Qt::AlignTop);
            layout->setContentsMargins(0, 0, 0, 0);
            frame->setLayout(layout);

            QFrame* header = new QFrame(this);
            QHBoxLayout* headerLayout = new QHBoxLayout();
            {
                QLabel* titleLabel = new QLabel(tr("My Projects"), this);
                titleLabel->setObjectName("titleLabel");
                headerLayout->addWidget(titleLabel);

                QMenu* newProjectMenu = new QMenu(this);
                m_createNewProjectAction = newProjectMenu->addAction("Create New Project");
                m_addExistingProjectAction = newProjectMenu->addAction("Add Existing Project");

                connect(m_createNewProjectAction, &QAction::triggered, this, &ProjectsScreen::HandleNewProjectButton);
                connect(m_addExistingProjectAction, &QAction::triggered, this, &ProjectsScreen::HandleAddProjectButton);

                QPushButton* newProjectMenuButton = new QPushButton(tr("New Project..."), this);
                newProjectMenuButton->setObjectName("newProjectButton");
                newProjectMenuButton->setMenu(newProjectMenu);
                newProjectMenuButton->setDefault(true);
                headerLayout->addWidget(newProjectMenuButton);
            }
            header->setLayout(headerLayout);

            layout->addWidget(header);

            // Get all projects and create a horizontal scrolling list of them
            auto projectsResult = PythonBindingsInterface::Get()->GetProjects();
            if (projectsResult.IsSuccess() && !projectsResult.GetValue().isEmpty())
            {
                QScrollArea* projectsScrollArea = new QScrollArea(this);
                QWidget* scrollWidget = new QWidget();

                FlowLayout* flowLayout = new FlowLayout(0, s_spacerSize, s_spacerSize);
                scrollWidget->setLayout(flowLayout);

                projectsScrollArea->setWidget(scrollWidget);
                projectsScrollArea->setWidgetResizable(true);

#ifndef DISPLAY_PROJECT_DEV_DATA
                for (auto project : projectsResult.GetValue())
#else
                ProjectInfo project = projectsResult.GetValue().at(0);
                for (int i = 0; i < 15; i++)
#endif
                {
                    ProjectButton* projectButton;

                    QString projectPreviewPath = project.m_path + m_projectPreviewImagePath;
                    QFileInfo doesPreviewExist(projectPreviewPath);
                    if (doesPreviewExist.exists() && doesPreviewExist.isFile())
                    {
                        project.m_imagePath = projectPreviewPath;
                    }

                    projectButton = new ProjectButton(project, this);

                    flowLayout->addWidget(projectButton);

                    connect(projectButton, &ProjectButton::OpenProject, this, &ProjectsScreen::HandleOpenProject);
                    connect(projectButton, &ProjectButton::EditProject, this, &ProjectsScreen::HandleEditProject);
                    connect(projectButton, &ProjectButton::CopyProject, this, &ProjectsScreen::HandleCopyProject);
                    connect(projectButton, &ProjectButton::RemoveProject, this, &ProjectsScreen::HandleRemoveProject);
                    connect(projectButton, &ProjectButton::DeleteProject, this, &ProjectsScreen::HandleDeleteProject);

#ifdef SHOW_ALL_PROJECT_ACTIONS
                    connect(projectButton, &ProjectButton::EditProjectGems, this, &ProjectsScreen::HandleEditProjectGems);
#endif
                }

                layout->addWidget(projectsScrollArea);
            }
        }

        return frame;
    }

    ProjectManagerScreen ProjectsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::Projects;
    }

    bool ProjectsScreen::IsTab()
    {
        return true;
    }

    QString ProjectsScreen::GetTabText()
    {
        return tr("Projects");
    }

    void ProjectsScreen::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        // we paint the background here because qss does not support background cover scaling
        QPainter painter(this);

        auto winSize = size();
        auto pixmapRatio = (float)m_background.width() / m_background.height();
        auto windowRatio = (float)winSize.width() / winSize.height();

        if (pixmapRatio > windowRatio)
        {
            auto newWidth = (int)(winSize.height() * pixmapRatio);
            auto offset = (newWidth - winSize.width()) / -2;
            painter.drawPixmap(offset, 0, newWidth, winSize.height(), m_background);
        }
        else
        {
            auto newHeight = (int)(winSize.width() / pixmapRatio);
            painter.drawPixmap(0, 0, winSize.width(), newHeight, m_background);
        }
    }

    void ProjectsScreen::HandleNewProjectButton()
    {
        emit ResetScreenRequest(ProjectManagerScreen::CreateProject);
        emit ChangeScreenRequest(ProjectManagerScreen::CreateProject);
    }
    void ProjectsScreen::HandleAddProjectButton()
    {
        if (ProjectUtils::AddProjectDialog(this))
        {
            emit ResetScreenRequest(ProjectManagerScreen::Projects);
            emit ChangeScreenRequest(ProjectManagerScreen::Projects);
        }
    }
    void ProjectsScreen::HandleOpenProject(const QString& projectPath)
    {
        if (!projectPath.isEmpty())
        {
            AZ::IO::FixedMaxPath executableDirectory = AZ::Utils::GetExecutableDirectory();
            AZStd::string executableFilename = "Editor";
            AZ::IO::FixedMaxPath editorExecutablePath = executableDirectory / (executableFilename + AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
            auto cmdPath = AZ::IO::FixedMaxPathString::format("%s -regset=\"/Amazon/AzCore/Bootstrap/project_path=%s\"", editorExecutablePath.c_str(), projectPath.toStdString().c_str());

            AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
            processLaunchInfo.m_commandlineParameters = cmdPath;
            bool launchSucceeded = AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
            if (!launchSucceeded)
            {
                AZ_Error("ProjectManager", false, "Failed to launch editor");
                QMessageBox::critical( this, tr("Error"), tr("Failed to launch the Editor, please verify the project settings are valid."));
            }
            else
            {
                // prevent the user from accidentally pressing the button while the editor is launching
                // and let them know what's happening
                ProjectButton* button = qobject_cast<ProjectButton*>(sender());
                if (button)
                {
                    button->SetButtonEnabled(false);
                    button->SetButtonOverlayText(tr("Opening Editor..."));
                }

                // enable the button after 3 seconds 
                constexpr int waitTimeInMs = 3000;
                QTimer::singleShot(waitTimeInMs, this, [this, button] {
                        if (button)
                        {
                            button->SetButtonEnabled(true);
                        }
                    });
            }
        }
        else
        {
            AZ_Error("ProjectManager", false, "Cannot open editor because an empty project path was provided");
            QMessageBox::critical( this, tr("Error"), tr("Failed to launch the Editor because the project path is invalid."));
        }

    }
    void ProjectsScreen::HandleEditProject(const QString& projectPath)
    {
        emit NotifyCurrentProject(projectPath);
        emit ResetScreenRequest(ProjectManagerScreen::UpdateProject);
        emit ChangeScreenRequest(ProjectManagerScreen::UpdateProject);
    }
    void ProjectsScreen::HandleEditProjectGems(const QString& projectPath)
    {
        emit NotifyCurrentProject(projectPath);
        emit ChangeScreenRequest(ProjectManagerScreen::GemCatalog);
    }
    void ProjectsScreen::HandleCopyProject(const QString& projectPath)
    {
        // Open file dialog and choose location for copied project then register copy with O3DE
        if (ProjectUtils::CopyProjectDialog(projectPath, this))
        {
            emit ResetScreenRequest(ProjectManagerScreen::Projects);
            emit ChangeScreenRequest(ProjectManagerScreen::Projects);
        }
    }
    void ProjectsScreen::HandleRemoveProject(const QString& projectPath)
    {
        // Unregister Project from O3DE and reload projects
        if (ProjectUtils::UnregisterProject(projectPath))
        {
            emit ResetScreenRequest(ProjectManagerScreen::Projects);
            emit ChangeScreenRequest(ProjectManagerScreen::Projects);
        }
    }
    void ProjectsScreen::HandleDeleteProject(const QString& projectPath)
    {
        QMessageBox::StandardButton warningResult = QMessageBox::warning(
            this, tr("Delete Project"), tr("Are you sure?\nProject will be removed from O3DE and directory will be deleted!"),
            QMessageBox::No | QMessageBox::Yes);

        if (warningResult == QMessageBox::Yes)
        {
            // Remove project from O3DE and delete from disk
            HandleRemoveProject(projectPath);
            ProjectUtils::DeleteProjectFiles(projectPath);
        }
    }

    void ProjectsScreen::NotifyCurrentScreen()
    {
        if (ShouldDisplayFirstTimeContent())
        {
            m_stack->setCurrentWidget(m_firstTimeContent);
        }
        else
        {
            m_stack->setCurrentWidget(m_projectsContent);
        }
    }

    bool ProjectsScreen::ShouldDisplayFirstTimeContent()
    {
        auto projectsResult = PythonBindingsInterface::Get()->GetProjects();
        if (!projectsResult.IsSuccess() || projectsResult.GetValue().isEmpty())
        {
            return true;
        }

        QSettings settings;
        bool displayFirstTimeContent = settings.value("displayFirstTimeContent", true).toBool();
        if (displayFirstTimeContent)
        {
            settings.setValue("displayFirstTimeContent", false);
        }

        return displayFirstTimeContent;
    }

} // namespace O3DE::ProjectManager
