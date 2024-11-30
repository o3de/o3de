/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectGemCatalogScreen.h>
#include <ProjectUtils.h>
#include <GemRepo/GemRepoScreen.h>
#include <CreateAGemScreen.h>
#include <ProjectManagerDefs.h>
#include <PythonBindingsInterface.h>
#include <ScreenHeaderWidget.h>
#include <ScreensCtrl.h>
#include <UpdateProjectCtrl.h>
#include <UpdateProjectSettingsScreen.h>
#include <ProjectUtils.h>
#include <DownloadController.h>
#include <SettingsInterface.h>

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QDir>

namespace O3DE::ProjectManager
{
    UpdateProjectCtrl::UpdateProjectCtrl(DownloadController* downloadController, QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);

        m_header = new ScreenHeader(this);
        m_header->setTitle(tr(""));
        m_header->setSubTitle(tr("Edit Project Settings:"));
        connect(m_header->backButton(), &QPushButton::clicked, this, &UpdateProjectCtrl::HandleBackButton);
        vLayout->addWidget(m_header);

        m_updateSettingsScreen = new UpdateProjectSettingsScreen();
        m_projectGemCatalogScreen = new ProjectGemCatalogScreen(downloadController, this);
        m_gemRepoScreen = new GemRepoScreen(this);

        connect(m_projectGemCatalogScreen, &ScreenWidget::ChangeScreenRequest, this, &UpdateProjectCtrl::OnChangeScreenRequest);
        connect(static_cast<ScreensCtrl*>(parent), &ScreensCtrl::NotifyProjectRemoved, m_projectGemCatalogScreen, &GemCatalogScreen::NotifyProjectRemoved);

        m_stack = new QStackedWidget(this);
        m_stack->setObjectName("body");
        m_stack->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        vLayout->addWidget(m_stack);

        QFrame* topBarFrameWidget = new QFrame(this);
        topBarFrameWidget->setObjectName("projectSettingsTopFrame");
        QHBoxLayout* topBarHLayout = new QHBoxLayout();
        topBarHLayout->setContentsMargins(0, 0, 0, 0);
        topBarFrameWidget->setLayout(topBarHLayout);

        QTabWidget* tabWidget = new QTabWidget();
        tabWidget->setObjectName("projectSettingsTab");
        tabWidget->tabBar()->setObjectName("projectSettingsTabBar");
        tabWidget->tabBar()->setFocusPolicy(Qt::TabFocus);
        tabWidget->addTab(m_updateSettingsScreen, tr("General"));

        QPushButton* gemsButton = new QPushButton(tr("Configure Gems"), this);
        gemsButton->setProperty("secondary", true);
        topBarHLayout->addWidget(gemsButton);
        tabWidget->setCornerWidget(gemsButton);

        topBarHLayout->addWidget(tabWidget);

        m_stack->addWidget(topBarFrameWidget);
        m_stack->addWidget(m_projectGemCatalogScreen);
        m_stack->addWidget(m_gemRepoScreen);

        QDialogButtonBox* backNextButtons = new QDialogButtonBox();
        backNextButtons->setObjectName("footer");
        vLayout->addWidget(backNextButtons);

        m_backButton = backNextButtons->addButton(tr("Back"), QDialogButtonBox::RejectRole);
        m_backButton->setProperty("secondary", true);
        m_nextButton = backNextButtons->addButton(tr("Next"), QDialogButtonBox::ApplyRole);
        m_nextButton->setProperty("primary", true);

        connect(gemsButton, &QPushButton::clicked, this, &UpdateProjectCtrl::HandleGemsButton);
        connect(m_backButton, &QPushButton::clicked, this, &UpdateProjectCtrl::HandleBackButton);
        connect(m_nextButton, &QPushButton::clicked, this, &UpdateProjectCtrl::HandleNextButton);
        connect(static_cast<ScreensCtrl*>(parent), &ScreensCtrl::NotifyCurrentProject, this, &UpdateProjectCtrl::UpdateCurrentProject);

        Update();
        setLayout(vLayout);
    }

    ProjectManagerScreen UpdateProjectCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::UpdateProject;
    }

    bool UpdateProjectCtrl::ContainsScreen(ProjectManagerScreen screen)
    {
        // Do not include GemRepos because we don't want to advertise jumping to it from all other screens here
        return screen == GetScreenEnum() || screen == ProjectManagerScreen::ProjectGemCatalog;
    }

    void UpdateProjectCtrl::GoToScreen(ProjectManagerScreen screen)
    {
        OnChangeScreenRequest(screen);
    }

    // Called when pressing "Edit Project Settings..."
    void UpdateProjectCtrl::NotifyCurrentScreen()
    {
        m_stack->setCurrentIndex(ScreenOrder::Settings);
        Update();
    }

    void UpdateProjectCtrl::OnChangeScreenRequest(ProjectManagerScreen screen)
    {
        if (screen == ProjectManagerScreen::GemRepos)
        {
            m_stack->setCurrentWidget(m_gemRepoScreen);
            m_gemRepoScreen->NotifyCurrentScreen();
            Update();
        }
        else if (screen == ProjectManagerScreen::ProjectGemCatalog)
        {
            m_projectGemCatalogScreen->ReinitForProject(m_projectInfo.m_path);
            m_projectGemCatalogScreen->NotifyCurrentScreen();
            m_stack->setCurrentWidget(m_projectGemCatalogScreen);
            Update();
        }
        else if (screen == ProjectManagerScreen::UpdateProjectSettings)
        {
            m_stack->setCurrentWidget(m_updateSettingsScreen);
            m_updateSettingsScreen->NotifyCurrentScreen();
            Update();
        }
        else
        {
            emit ChangeScreenRequest(screen);
        }
    }

    void UpdateProjectCtrl::HandleGemsButton()
    {
        if (UpdateProjectSettings(true))
        {
            m_projectGemCatalogScreen->ReinitForProject(m_projectInfo.m_path);
            m_projectGemCatalogScreen->NotifyCurrentScreen();
            m_stack->setCurrentWidget(m_projectGemCatalogScreen);
            Update();
        }
    }

    void UpdateProjectCtrl::HandleBackButton()
    {
        if (m_stack->currentIndex() > 0)
        {
            m_stack->setCurrentIndex(m_stack->currentIndex() - 1);
            if (ScreenWidget* screenWidget = qobject_cast<ScreenWidget*>(m_stack->currentWidget()); screenWidget)
            {
                screenWidget->NotifyCurrentScreen();
            }
            Update();
        }
        else
        {
            if (UpdateProjectSettings(true))
            {
                emit GoToPreviousScreenRequest();
            }
        }
    }

    void UpdateProjectCtrl::HandleNextButton()
    {
        bool shouldRebuild = false;

        if (m_stack->currentIndex() == ScreenOrder::Settings && m_updateSettingsScreen)
        {
            if (!UpdateProjectSettings())
            {
                return;
            }
        }
        else if (m_stack->currentIndex() == ScreenOrder::Gems && m_projectGemCatalogScreen)
        {
            if (!m_projectGemCatalogScreen->GetDownloadController()->IsDownloadQueueEmpty())
            {
                QMessageBox::critical(this, tr("Gems downloading"), tr("You must wait for gems to finish downloading before continuing."));
                return;
            }

            // Enable or disable the gems that got adjusted in the gem catalog and apply them to the given project.
            const ProjectGemCatalogScreen::ConfiguredGemsResult result = m_projectGemCatalogScreen->ConfigureGemsForProject(m_projectInfo.m_path);
            if (result == ProjectGemCatalogScreen::ConfiguredGemsResult::Failed)
            {
                QMessageBox::critical(this, tr("Failed to configure gems"), tr("Failed to configure gems for project."));
            }
            if (result != ProjectGemCatalogScreen::ConfiguredGemsResult::Success)
            {
                return;
            }

            shouldRebuild = true;
        }

        if (shouldRebuild)
        {
            emit NotifyBuildProject(m_projectInfo);
        }

        emit ChangeScreenRequest(ProjectManagerScreen::Projects);
    }

    void UpdateProjectCtrl::UpdateCurrentProject(const QString& projectPath)
    {
        auto projectResult = PythonBindingsInterface::Get()->GetProject(projectPath);
        if (projectResult.IsSuccess())
        {
            m_projectInfo = projectResult.GetValue();
        }

        Update();
        UpdateSettingsScreen();
    }

    void UpdateProjectCtrl::Update()
    {
        if (m_stack->currentIndex() == ScreenOrder::GemRepos)
        {
            m_header->setTitle(QString(tr("Edit Project Settings: \"%1\"")).arg(m_projectInfo.GetProjectDisplayName()));
            m_header->setSubTitle(QString(tr("Remote Sources")));
            m_nextButton->setVisible(false);
        }
        else if (m_stack->currentIndex() == ScreenOrder::Gems)
        {

            m_header->setTitle(QString(tr("Edit Project Settings: \"%1\"")).arg(m_projectInfo.GetProjectDisplayName()));
            m_header->setSubTitle(QString(tr("Configure Gems")));
            m_nextButton->setText(tr("Save"));
            m_nextButton->setVisible(true);
        }
        else
        {
            m_header->setTitle("");
            m_header->setSubTitle(QString(tr("Edit Project Settings: \"%1\"")).arg(m_projectInfo.GetProjectDisplayName()));
            m_nextButton->setText(tr("Save"));
            m_nextButton->setVisible(true);
        }
    }

    void UpdateProjectCtrl::UpdateSettingsScreen()
    {
        m_updateSettingsScreen->SetProjectInfo(m_projectInfo);
    }

    bool UpdateProjectCtrl::UpdateProjectSettings(bool shouldConfirm)
    {
        AZ_Assert(m_updateSettingsScreen, "Update settings screen is nullptr.")

        ProjectInfo newProjectSettings = m_updateSettingsScreen->GetProjectInfo();

        if (m_projectInfo != newProjectSettings)
        {
            if (shouldConfirm)
            {
                QMessageBox::StandardButton questionResult = QMessageBox::question(
                    this,
                    QObject::tr("Unsaved changes"),
                    QObject::tr("Would you like to save your changes to project settings?"),
                    QMessageBox::No | QMessageBox::Yes
                );

                if (questionResult == QMessageBox::No)
                {
                    return true;
                }
            }

            if (!m_updateSettingsScreen->Validate())
            {
                QMessageBox::critical(this, tr("Invalid project settings"), tr("Invalid project settings"));
                return false;
            }

            // Move project first to avoid trying to update settings at the new location before it has been moved there
            if (QDir(newProjectSettings.m_path) != QDir(m_projectInfo.m_path))
            {
                if (!ProjectUtils::MoveProject(m_projectInfo.m_path, newProjectSettings.m_path, this))
                {
                    QMessageBox::critical(this, tr("Project move failed"), tr("Failed to move project."));
                    return false;
                }
            }

            // Check engine compatibility if a new engine was selected
            if (QDir(newProjectSettings.m_enginePath) != QDir(m_projectInfo.m_enginePath))
            {
                auto incompatibleObjectsResult = PythonBindingsInterface::Get()->GetProjectEngineIncompatibleObjects(newProjectSettings.m_path, newProjectSettings.m_enginePath);

                AZStd::string errorTitle, generalError, detailedError;
                if (!incompatibleObjectsResult)
                {
                    errorTitle = "Failed to check project compatibility";
                    generalError = incompatibleObjectsResult.GetError().first;
                    generalError.append("\nDo you still want to save your changes to project settings?");
                    detailedError = incompatibleObjectsResult.GetError().second;
                }
                else if (const auto& incompatibleObjects = incompatibleObjectsResult.GetValue(); !incompatibleObjects.isEmpty())
                {
                    // provide a couple more user friendly error messages for uncommon cases
                    if (incompatibleObjects.at(0).contains(ProjectUtils::EngineJsonFilename.data(), Qt::CaseInsensitive))
                    {
                        errorTitle = "Failed to read engine.json";
                        generalError = "The projects compatibility with the new engine could not be checked because the engine.json could not be read";
                    }
                    else if (incompatibleObjects.at(0).contains(ProjectUtils::ProjectJsonFilename.data(), Qt::CaseInsensitive))
                    {
                        errorTitle = "Invalid project, failed to read project.json";
                        generalError = "The projects compatibility with the new engine could not be checked because the project.json could not be read.";
                    }
                    else
                    {
                        // could be gems, apis or both
                        errorTitle = "Project may not be compatible with new engine";
                        generalError = incompatibleObjects.join("\n").toUtf8().constData();
                        generalError.append("\nDo you still want to save your changes to project settings?");
                    }
                }

                if (!generalError.empty())
                {
                    QMessageBox warningDialog(QMessageBox::Warning, errorTitle.c_str(), generalError.c_str(), QMessageBox::Yes | QMessageBox::No, this);
                    warningDialog.setDetailedText(detailedError.c_str());
                    if(warningDialog.exec() == QMessageBox::No)
                    {
                        return false;
                    }
                    AZ_Warning("ProjectManager", false, "Proceeding with saving project settings after engine compatibility check failed.");
                }
            }

            if (auto result = PythonBindingsInterface::Get()->UpdateProject(newProjectSettings); !result.IsSuccess())
            {
                QMessageBox::critical(this, tr("Project update failed"), tr(result.GetError().c_str()));
                return false;
            }

            if (QDir(newProjectSettings.m_path) != QDir(m_projectInfo.m_path) ||
                newProjectSettings.m_projectName != m_projectInfo.m_projectName ||
                QDir(newProjectSettings.m_enginePath) != QDir(m_projectInfo.m_enginePath))
            {
                // Remove project build successfully paths for both old and new projects
                // because a full rebuild is required when moving projects
                auto settings = SettingsInterface::Get();
                settings->SetProjectBuiltSuccessfully(m_projectInfo, false);
                settings->SetProjectBuiltSuccessfully(newProjectSettings, false);

                emit NotifyBuildProject(newProjectSettings);
            }

            if (!newProjectSettings.m_newPreviewImagePath.isEmpty())
            {
                if (!ProjectUtils::ReplaceProjectFile(
                        QDir(newProjectSettings.m_path).filePath(newProjectSettings.m_iconPath), newProjectSettings.m_newPreviewImagePath))
                {
                    QMessageBox::critical(this, tr("File replace failed"), tr("Failed to replace project preview image."));
                    return false;
                }
                m_updateSettingsScreen->ResetProjectPreviewPath();
            }

            m_projectInfo = newProjectSettings;
        }

        return true;
    }

} // namespace O3DE::ProjectManager
