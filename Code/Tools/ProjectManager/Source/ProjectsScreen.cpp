/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectsScreen.h>

#include <ProjectManagerDefs.h>
#include <ProjectButtonWidget.h>
#include <PythonBindingsInterface.h>
#include <ProjectUtils.h>
#include <ProjectBuilderController.h>
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

namespace O3DE::ProjectManager
{
    ProjectsScreen::ProjectsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignTop);
        vLayout->setContentsMargins(s_contentMargins, 0, s_contentMargins, 0);
        setLayout(vLayout);

        m_stack = new QStackedWidget(this);

        m_firstTimeContent = CreateFirstTimeContent();
        m_stack->addWidget(m_firstTimeContent);

        m_projectsContent = CreateProjectsContent();
        m_stack->addWidget(m_projectsContent);

        vLayout->addWidget(m_stack);

        connect(reinterpret_cast<ScreensCtrl*>(parent), &ScreensCtrl::NotifyBuildProject, this, &ProjectsScreen::SuggestBuildProject);

        // Will focus whatever button it finds so the Project tab is not focused on start-up
        QTimer::singleShot(0, this, [this]
            {
                QPushButton* foundButton = m_stack->currentWidget()->findChild<QPushButton*>();
                if (foundButton)
                {
                    foundButton->setFocus();
                }
            });
    }

    ProjectsScreen::~ProjectsScreen()
    {
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

            QLabel* titleLabel = new QLabel(tr("Ready? Set. Create!"), this);
            titleLabel->setObjectName("titleLabel");
            layout->addWidget(titleLabel);

            QLabel* introLabel = new QLabel(this);
            introLabel->setObjectName("introLabel");
            introLabel->setText(tr("Welcome to O3DE! Start something new by creating a project."));
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

    QFrame* ProjectsScreen::CreateProjectsContent()
    {
        QFrame* frame = new QFrame(this);
        frame->setObjectName("projectsContent");
        {
            QVBoxLayout* layout = new QVBoxLayout();
            layout->setAlignment(Qt::AlignTop);
            layout->setContentsMargins(0, 0, 0, 0);
            frame->setLayout(layout);

            QFrame* header = new QFrame(frame);
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

            QScrollArea* projectsScrollArea = new QScrollArea(this);
            QWidget* scrollWidget = new QWidget();

            m_projectsFlowLayout = new FlowLayout(0, s_spacerSize, s_spacerSize);
            scrollWidget->setLayout(m_projectsFlowLayout);

            projectsScrollArea->setWidget(scrollWidget);
            projectsScrollArea->setWidgetResizable(true);

            ResetProjectsContent();

            layout->addWidget(projectsScrollArea);
        }

        return frame;
    }

    ProjectButton* ProjectsScreen::CreateProjectButton(const ProjectInfo& project)
    {
        ProjectButton* projectButton = new ProjectButton(project, this);
        m_projectButtons.insert(QDir::toNativeSeparators(project.m_path), projectButton);
        m_projectsFlowLayout->addWidget(projectButton);

        connect(projectButton, &ProjectButton::OpenProject, this, &ProjectsScreen::HandleOpenProject);
        connect(projectButton, &ProjectButton::EditProject, this, &ProjectsScreen::HandleEditProject);
        connect(projectButton, &ProjectButton::CopyProject, this, &ProjectsScreen::HandleCopyProject);
        connect(projectButton, &ProjectButton::RemoveProject, this, &ProjectsScreen::HandleRemoveProject);
        connect(projectButton, &ProjectButton::DeleteProject, this, &ProjectsScreen::HandleDeleteProject);
        connect(projectButton, &ProjectButton::BuildProject, this, &ProjectsScreen::QueueBuildProject);
        connect(projectButton, &ProjectButton::OpenCMakeGUI, this, 
            [this](const ProjectInfo& projectInfo)
            {
                AZ::Outcome result = ProjectUtils::OpenCMakeGUI(projectInfo.m_path);
                if (!result)
                {
                    QMessageBox::critical(this, tr("Failed to open CMake GUI"), result.GetError(), QMessageBox::Ok);
                }
            });

        return projectButton;
    }

    void ProjectsScreen::ResetProjectsContent()
    {
        RemoveInvalidProjects();

        // Get all projects and create a vertical scrolling list of them
        // Sort building and queued projects first
        auto projectsResult = PythonBindingsInterface::Get()->GetProjects();
        if (projectsResult.IsSuccess() && !projectsResult.GetValue().isEmpty())
        {
            QVector<ProjectInfo> projectsVector = projectsResult.GetValue();
            // If a project path is in this set then the button for it will be kept
            QSet<QString> keepProject;
            for (const ProjectInfo& project : projectsVector)
            {
                keepProject.insert(QDir::toNativeSeparators(project.m_path));
            }

            // Clear flow and delete buttons for removed projects
            auto projectButtonsIter = m_projectButtons.begin();
            while (projectButtonsIter != m_projectButtons.end())
            {
                m_projectsFlowLayout->removeWidget(projectButtonsIter.value());

                if (!keepProject.contains(projectButtonsIter.key()))
                {
                    projectButtonsIter.value()->deleteLater();
                    projectButtonsIter = m_projectButtons.erase(projectButtonsIter);
                }
                else
                {
                    ++projectButtonsIter;
                }
            }

            QString buildProjectPath = "";
            if (m_currentBuilder)
            {
                buildProjectPath = QDir::toNativeSeparators(m_currentBuilder->GetProjectInfo().m_path);
            }

            // Put currently building project in front, then queued projects, then sorts alphabetically
            std::sort(projectsVector.begin(), projectsVector.end(), [buildProjectPath, this](const ProjectInfo& arg1, const ProjectInfo& arg2)
            {
                if (arg1.m_path == buildProjectPath)
                {
                    return true;
                }
                else if (arg2.m_path == buildProjectPath)
                {
                    return false;
                }

                bool arg1InBuildQueue = BuildQueueContainsProject(arg1.m_path);
                bool arg2InBuildQueue = BuildQueueContainsProject(arg2.m_path);
                if (arg1InBuildQueue && !arg2InBuildQueue)
                {
                    return true;
                }
                else if (!arg1InBuildQueue && arg2InBuildQueue)
                {
                    return false;
                }
                else
                {
                    return arg1.m_displayName.toLower() < arg2.m_displayName.toLower();
                }
            });

            // Add any missing project buttons and restore buttons to default state
            for (const ProjectInfo& project : projectsVector)
            {
                if (!m_projectButtons.contains(QDir::toNativeSeparators(project.m_path)))
                {
                    m_projectButtons.insert(QDir::toNativeSeparators(project.m_path), CreateProjectButton(project));
                }
                else
                {
                    auto projectButtonIter = m_projectButtons.find(QDir::toNativeSeparators(project.m_path));
                    if (projectButtonIter != m_projectButtons.end())
                    {
                        projectButtonIter.value()->RestoreDefaultState();
                        m_projectsFlowLayout->addWidget(projectButtonIter.value());
                    }
                }
            }

            // Setup building button again
            auto buildProjectIter = m_projectButtons.find(buildProjectPath);
            if (buildProjectIter != m_projectButtons.end())
            {
                m_currentBuilder->SetProjectButton(buildProjectIter.value());
            }

            for (const ProjectInfo& project : m_buildQueue)
            {
                auto projectIter = m_projectButtons.find(QDir::toNativeSeparators(project.m_path));
                if (projectIter != m_projectButtons.end())
                {
                    projectIter.value()->SetProjectButtonAction(
                        tr("Cancel Queued Build"),
                        [this, project]
                        {
                            UnqueueBuildProject(project);
                            SuggestBuildProjectMsg(project, false);
                        });
                }
            }

            for (const ProjectInfo& project : m_requiresBuild)
            {
                auto projectIter = m_projectButtons.find(QDir::toNativeSeparators(project.m_path));
                if (projectIter != m_projectButtons.end())
                {
                    if (project.m_buildFailed)
                    {
                        projectIter.value()->ShowBuildFailed(true, project.m_logUrl);
                    }
                    else
                    {
                        projectIter.value()->ShowBuildRequired();
                    }
                }
            }
        }

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

        const QSize winSize = size();
        const float pixmapRatio = (float)m_background.width() / m_background.height();
        const float windowRatio = (float)winSize.width() / winSize.height();

        QRect backgroundRect;
        if (pixmapRatio > windowRatio)
        {
            const int newWidth = (int)(winSize.height() * pixmapRatio);
            const int offset = (newWidth - winSize.width()) / -2;
            backgroundRect = QRect(offset, 0, newWidth, winSize.height());
        }
        else
        {
            const int newHeight = (int)(winSize.width() / pixmapRatio);
            backgroundRect = QRect(0, 0, winSize.width(), newHeight);
        }

        // Draw the background image.
        painter.drawPixmap(backgroundRect, m_background);

        // Draw a semi-transparent overlay to darken down the colors.
        // Use SourceOver, DestinationIn will make background transparent on Mac
        painter.setCompositionMode (QPainter::CompositionMode_SourceOver);
        const float overlayTransparency = 0.3f;
        painter.fillRect(backgroundRect, QColor(0, 0, 0, static_cast<int>(255.0f * overlayTransparency)));
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
                        [button]
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
    void ProjectsScreen::HandleCopyProject(const ProjectInfo& projectInfo)
    {
        if (!WarnIfInBuildQueue(projectInfo.m_path))
        {
            ProjectInfo newProjectInfo(projectInfo);

            // Open file dialog and choose location for copied project then register copy with O3DE
            if (ProjectUtils::CopyProjectDialog(projectInfo.m_path, newProjectInfo, this))
            {
                ResetProjectsContent();
                emit NotifyBuildProject(newProjectInfo);
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

    void ProjectsScreen::SuggestBuildProjectMsg(const ProjectInfo& projectInfo, bool showMessage)
    {
        if (RequiresBuildProjectIterator(projectInfo.m_path) == m_requiresBuild.end() || projectInfo.m_buildFailed)
        {
            m_requiresBuild.append(projectInfo);
        }
        ResetProjectsContent();

        if (showMessage)
        {
            QMessageBox::information(this,
                tr("Project Should be rebuilt."),
                projectInfo.GetProjectDisplayName() + tr(" project likely needs to be rebuilt."));
        }
    }

    void ProjectsScreen::SuggestBuildProject(const ProjectInfo& projectInfo)
    {
        SuggestBuildProjectMsg(projectInfo, true);
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
                // Projects Content is already reset in function
            }
            else
            {
                m_buildQueue.append(projectInfo);
                ResetProjectsContent();
            }
        }
    }

    void ProjectsScreen::UnqueueBuildProject(const ProjectInfo& projectInfo)
    {
        m_buildQueue.removeAll(projectInfo);
        ResetProjectsContent();
    }

    void ProjectsScreen::NotifyCurrentScreen()
    {
        if (ShouldDisplayFirstTimeContent())
        {
            m_background.load(":/Backgrounds/FtueBackground.jpg");
            m_stack->setCurrentWidget(m_firstTimeContent);
        }
        else
        {
            m_background.load(":/Backgrounds/DefaultBackground.jpg");
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

    bool ProjectsScreen::RemoveInvalidProjects()
    {
        return PythonBindingsInterface::Get()->RemoveInvalidProjects();
    }

    bool ProjectsScreen::StartProjectBuild(const ProjectInfo& projectInfo)
    {
        if (ProjectUtils::FindSupportedCompiler(this))
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
                connect(m_currentBuilder, &ProjectBuilderController::NotifyBuildProject, this, &ProjectsScreen::SuggestBuildProject);

                m_currentBuilder->Start();
            }
            else
            {
                SuggestBuildProjectMsg(projectInfo, false);
                return false;
            }

            return true;
        }

        return false;
    }

    void ProjectsScreen::ProjectBuildDone(bool success)
    {
        ProjectInfo currentBuilderProject;
        if (!success)
        {
            currentBuilderProject = m_currentBuilder->GetProjectInfo();
        }

        delete m_currentBuilder;
        m_currentBuilder = nullptr;

        if (!success)
        {
            SuggestBuildProjectMsg(currentBuilderProject, false);
        }

        if (!m_buildQueue.empty())
        {
            while (!StartProjectBuild(m_buildQueue.front()) && m_buildQueue.size() > 1)
            {
                m_buildQueue.pop_front();
            }
            m_buildQueue.pop_front();
        }

        ResetProjectsContent();
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
