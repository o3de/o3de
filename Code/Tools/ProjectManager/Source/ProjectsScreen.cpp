/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectsScreen.h>

#include <ProjectManagerDefs.h>
#include <ProjectButtonWidget.h>
#include <PythonBindingsInterface.h>
#include <ProjectUtils.h>
#include <ProjectBuilder.h>
#include <ScreensCtrl.h>

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
#include <QScrollArea>
#include <QStackedWidget>
#include <QFrame>
#include <QIcon>
#include <QPixmap>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>
#include <QQueue>
#include <QDir>
#include <QGuiApplication>

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

        connect(reinterpret_cast<ScreensCtrl*>(parent), &ScreensCtrl::NotifyBuildProject, this, &ProjectsScreen::SuggestBuildProject);
    }

    ProjectsScreen::~ProjectsScreen()

    {
        delete m_currentBuilder;
    }

    QFrame* ProjectsScreen::CreateFirstTimeContent()
    {
        QFrame* frame = new QFrame(this);
        frame->setObjectName("firstTimeContent");
        {
            QVBoxLayout* layout = new QVBoxLayout();
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

            QHBoxLayout* buttonLayout = new QHBoxLayout();
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

    QFrame* ProjectsScreen::CreateProjectsContent(QString buildProjectPath, ProjectButton** projectButton)
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
                // Iterate once to insert building project first
                if (!buildProjectPath.isEmpty())
                {
                    buildProjectPath = QDir::fromNativeSeparators(buildProjectPath);
                    for (auto project : projectsResult.GetValue())
                    {
                        if (QDir::fromNativeSeparators(project.m_path) == buildProjectPath)
                        {
                            ProjectButton* buildingProjectButton = CreateProjectButton(project, flowLayout, true);

                            if (projectButton)
                            {
                                *projectButton = buildingProjectButton;
                            }

                            break;
                        }
                    }
                }

                for (auto project : projectsResult.GetValue())
#else
                ProjectInfo project = projectsResult.GetValue().at(0);
                for (int i = 0; i < 15; i++)
#endif
                {
                    // Add all other projects skipping building project
                    // Safe if no building project because it is just an empty string
                    if (project.m_path != buildProjectPath)
                    {
                        ProjectButton* projectButtonWidget = CreateProjectButton(project, flowLayout);

                        if (RequiresBuildProjectIterator(project.m_path) != m_requiresBuild.end())
                        {
                            projectButtonWidget->ShowBuildButton(true);
                        }
                    }
                }

                layout->addWidget(projectsScrollArea);
            }
        }

        return frame;
    }

    ProjectButton* ProjectsScreen::CreateProjectButton(ProjectInfo& project, QLayout* flowLayout, bool processing)
    {
        ProjectButton* projectButton = new ProjectButton(project, this, processing);

        flowLayout->addWidget(projectButton);

        if (!processing)
        {
            connect(projectButton, &ProjectButton::OpenProject, this, &ProjectsScreen::HandleOpenProject);
            connect(projectButton, &ProjectButton::EditProject, this, &ProjectsScreen::HandleEditProject);
            connect(projectButton, &ProjectButton::CopyProject, this, &ProjectsScreen::HandleCopyProject);
            connect(projectButton, &ProjectButton::RemoveProject, this, &ProjectsScreen::HandleRemoveProject);
            connect(projectButton, &ProjectButton::DeleteProject, this, &ProjectsScreen::HandleDeleteProject);
        }
        connect(projectButton, &ProjectButton::BuildProject, this, &ProjectsScreen::QueueBuildProject);

        return projectButton;
    }

    void ProjectsScreen::ResetProjectsContent()
    {
        // refresh the projects content by re-creating it for now
        if (m_projectsContent)
        {
            m_stack->removeWidget(m_projectsContent);
            m_projectsContent->deleteLater();
        }

        // Make sure to update builder with latest Project Button
        if (m_currentBuilder)
        {
            ProjectButton* projectButtonPtr;

            m_projectsContent = CreateProjectsContent(m_currentBuilder->GetProjectPath(), &projectButtonPtr);
            m_currentBuilder->SetProjectButton(projectButtonPtr);
        }
        else
        {
            m_projectsContent = CreateProjectsContent();
        }

        m_stack->addWidget(m_projectsContent);
        m_stack->setCurrentWidget(m_projectsContent);
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
            ResetProjectsContent();
            emit ChangeScreenRequest(ProjectManagerScreen::Projects);
        }
    }
    void ProjectsScreen::HandleOpenProject(const QString& projectPath)
    {
        if (!projectPath.isEmpty())
        {
            if (!WarnIfInBuildQueue(projectPath))
            {
                AZ::IO::FixedMaxPath executableDirectory = AZ::Utils::GetExecutableDirectory();
                AZStd::string executableFilename = "Editor";
                AZ::IO::FixedMaxPath editorExecutablePath = executableDirectory / (executableFilename + AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
                auto cmdPath = AZ::IO::FixedMaxPathString::format(
                    "%s -regset=\"/Amazon/AzCore/Bootstrap/project_path=%s\"", editorExecutablePath.c_str(),
                    projectPath.toStdString().c_str());

                AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
                processLaunchInfo.m_commandlineParameters = cmdPath;
                bool launchSucceeded = AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
                if (!launchSucceeded)
                {
                    AZ_Error("ProjectManager", false, "Failed to launch editor");
                    QMessageBox::critical(
                        this, tr("Error"), tr("Failed to launch the Editor, please verify the project settings are valid."));
                }
                else
                {
                    // prevent the user from accidentally pressing the button while the editor is launching
                    // and let them know what's happening
                    ProjectButton* button = qobject_cast<ProjectButton*>(sender());
                    if (button)
                    {
                        button->SetLaunchButtonEnabled(false);
                        button->SetButtonOverlayText(tr("Opening Editor..."));
                    }

                    // enable the button after 3 seconds
                    constexpr int waitTimeInMs = 3000;
                    QTimer::singleShot(
                        waitTimeInMs, this,
                        [this, button]
                        {
                            if (button)
                            {
                                button->SetLaunchButtonEnabled(true);
                            }
                        });
                }
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
        if (!WarnIfInBuildQueue(projectPath))
        {
            emit NotifyCurrentProject(projectPath);
            emit ChangeScreenRequest(ProjectManagerScreen::UpdateProject);
        }
    }
    void ProjectsScreen::HandleCopyProject(const QString& projectPath)
    {
        if (!WarnIfInBuildQueue(projectPath))
        {
            // Open file dialog and choose location for copied project then register copy with O3DE
            if (ProjectUtils::CopyProjectDialog(projectPath, this))
            {
                ResetProjectsContent();
                emit ChangeScreenRequest(ProjectManagerScreen::Projects);
            }
        }
    }
    void ProjectsScreen::HandleRemoveProject(const QString& projectPath)
    {
        if (!WarnIfInBuildQueue(projectPath))
        {
            // Unregister Project from O3DE and reload projects
            if (ProjectUtils::UnregisterProject(projectPath))
            {
                ResetProjectsContent();
                emit ChangeScreenRequest(ProjectManagerScreen::Projects);
            }
        }
    }
    void ProjectsScreen::HandleDeleteProject(const QString& projectPath)
    {
        if (!WarnIfInBuildQueue(projectPath))
        {
            QMessageBox::StandardButton warningResult = QMessageBox::warning(this,
                tr("Delete Project"),
                tr("Are you sure?\nProject will be unregistered from O3DE and project directory will be deleted from your disk."),
                QMessageBox::No | QMessageBox::Yes);

            if (warningResult == QMessageBox::Yes)
            {
                QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                // Remove project from O3DE and delete from disk
                HandleRemoveProject(projectPath);
                ProjectUtils::DeleteProjectFiles(projectPath);
                QGuiApplication::restoreOverrideCursor();
            }
        }
    }

    void ProjectsScreen::SuggestBuildProject(const ProjectInfo& projectInfo)
    {
        if (projectInfo.m_needsBuild)
        {
            if (RequiresBuildProjectIterator(projectInfo.m_path) == m_requiresBuild.end())
            {
                m_requiresBuild.append(projectInfo);
            }
            ResetProjectsContent();
        }
        else
        {
            QMessageBox::information(this,
                tr("Project Should be rebuilt."),
                projectInfo.GetProjectDisplayName() + tr(" project likely needs to be rebuilt."));
        }
    }

    void ProjectsScreen::QueueBuildProject(const ProjectInfo& projectInfo)
    {
        auto requiredIter = RequiresBuildProjectIterator(projectInfo.m_path);
        if (requiredIter != m_requiresBuild.end())
        {
            m_requiresBuild.erase(requiredIter);
        }

        if (!BuildQueueContainsProject(projectInfo.m_path))
        {
            if (m_buildQueue.empty() && !m_currentBuilder)
            {
                StartProjectBuild(projectInfo);
            }
            else
            {
                m_buildQueue.append(projectInfo);
            }
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
            ResetProjectsContent();
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

    void ProjectsScreen::StartProjectBuild(const ProjectInfo& projectInfo)
    {
        if (ProjectUtils::IsVS2019Installed())
        {
            QMessageBox::StandardButton buildProject = QMessageBox::information(
                this,
                tr("Building \"%1\"").arg(projectInfo.GetProjectDisplayName()),
                tr("Ready to build \"%1\"?").arg(projectInfo.GetProjectDisplayName()),
                QMessageBox::No | QMessageBox::Yes);

            if (buildProject == QMessageBox::Yes)
            {
                m_currentBuilder = new ProjectBuilderController(projectInfo, nullptr, this);
                ResetProjectsContent();
                connect(m_currentBuilder, &ProjectBuilderController::Done, this, &ProjectsScreen::ProjectBuildDone);

                m_currentBuilder->Start();
            }
            else
            {
                ProjectBuildDone();
            }
        }
    }

    void ProjectsScreen::ProjectBuildDone()
    {
        delete m_currentBuilder;
        m_currentBuilder = nullptr;

        if (!m_buildQueue.empty())
        {
            StartProjectBuild(m_buildQueue.front());
            m_buildQueue.pop_front();
        }
        else
        {
            ResetProjectsContent();
        }
    }

    QList<ProjectInfo>::iterator ProjectsScreen::RequiresBuildProjectIterator(const QString& projectPath)
    {
        QString nativeProjPath(QDir::toNativeSeparators(projectPath));
        auto projectIter = m_requiresBuild.begin();
        for (; projectIter != m_requiresBuild.end(); ++projectIter)
        {
            if (QDir::toNativeSeparators(projectIter->m_path) == nativeProjPath)
            {
                break;
            }
        }

        return projectIter;
    }

    bool ProjectsScreen::BuildQueueContainsProject(const QString& projectPath)
    {
        QString nativeProjPath(QDir::toNativeSeparators(projectPath));
        for (const ProjectInfo& project : m_buildQueue)
        {
            if (QDir::toNativeSeparators(project.m_path) == nativeProjPath)
            {
                return true;
            }
        }

        return false;
    }

    bool ProjectsScreen::WarnIfInBuildQueue(const QString& projectPath)
    {
        if (BuildQueueContainsProject(projectPath))
        {
            QMessageBox::warning(
                this,
                tr("Action Temporarily Disabled!"),
                tr("Action not allowed on projects in build queue."));

            return true;
        }

        return false;
    }

} // namespace O3DE::ProjectManager
