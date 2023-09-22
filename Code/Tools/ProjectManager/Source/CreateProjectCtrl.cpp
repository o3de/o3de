/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CreateProjectCtrl.h>
#include <ScreensCtrl.h>
#include <PythonBindingsInterface.h>
#include <NewProjectSettingsScreen.h>
#include <ScreenHeaderWidget.h>
#include <GemCatalog/GemModel.h>
#include <ProjectGemCatalogScreen.h>
#include <GemRepo/GemRepoScreen.h>
#include <ProjectUtils.h>
#include <DownloadController.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QStackedWidget>
#include <QLabel>
#include <QSizePolicy>
#include <QFileInfo>

namespace O3DE::ProjectManager
{
    CreateProjectCtrl::CreateProjectCtrl(DownloadController* downloadController, QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0,0,0,0);

        m_header = new ScreenHeader(this);
        m_header->setTitle(tr("Create a New Project"));
        m_header->setSubTitle(tr("Enter Project Details"));
        connect(m_header->backButton(), &QPushButton::clicked, this, &CreateProjectCtrl::HandleBackButton);
        vLayout->addWidget(m_header);

        m_stack = new QStackedWidget(this);
        m_stack->setObjectName("body");
        m_stack->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding));

        m_newProjectSettingsScreen = new NewProjectSettingsScreen(downloadController, this);
        m_stack->addWidget(m_newProjectSettingsScreen);

        m_projectGemCatalogScreen = new ProjectGemCatalogScreen(downloadController, this);
        m_stack->addWidget(m_projectGemCatalogScreen);

        m_gemRepoScreen = new GemRepoScreen(this);
        m_stack->addWidget(m_gemRepoScreen);

        vLayout->addWidget(m_stack);

        connect(m_projectGemCatalogScreen, &ScreenWidget::ChangeScreenRequest, this, &CreateProjectCtrl::OnChangeScreenRequest);
        connect(static_cast<ScreensCtrl*>(parent), &ScreensCtrl::NotifyProjectRemoved, m_projectGemCatalogScreen, &GemCatalogScreen::NotifyProjectRemoved);


        // When there are multiple project templates present, we re-gather the gems when changing the selected the project template.
        connect(m_newProjectSettingsScreen, &NewProjectSettingsScreen::OnTemplateSelectionChanged, this, [=](int oldIndex, [[maybe_unused]] int newIndex)
            {
                const GemModel* gemModel = m_projectGemCatalogScreen->GetGemModel();
                const QVector<QModelIndex> toBeAdded = gemModel->GatherGemsToBeAdded();
                const QVector<QModelIndex> toBeRemoved = gemModel->GatherGemsToBeRemoved();
                if (!toBeAdded.isEmpty() || !toBeRemoved.isEmpty())
                {
                    // In case the user enabled or disabled any gem and the current selection does not match the default from the
                    // // project template anymore, we need to ask the user if they want to proceed as their modifications will be lost.
                    const QString title = tr("Modifications will be lost");
                    const QString text = tr("You selected a new project template after modifying the enabled gems.\n\n"
                        "All modifications will be lost and the default from the new project template will be used.\n\n"
                        "Do you want to proceed?");
                    if (QMessageBox::warning(this, title, text, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
                    {
                        // The users wants to proceed. Reinitialize based on the newly selected project template.
                        ReinitGemCatalogForSelectedTemplate();
                    }
                    else
                    {
                        // Roll-back to the previously selected project template and
                        // block signals so that we don't end up in this same callback again.
                        m_newProjectSettingsScreen->SelectProjectTemplate(oldIndex, /*blockSignals=*/true);
                    }
                }
                else
                {
                    // In case the user did not enable or disable any gem and the currently enabled gems matches the previously selected
                    // ones from the project template, we can just reinitialize based on the newly selected project template.
                    ReinitGemCatalogForSelectedTemplate();
                }
            });

        QDialogButtonBox* buttons = new QDialogButtonBox();
        buttons->setObjectName("footer");
        vLayout->addWidget(buttons);

        m_primaryButton = buttons->addButton(tr("Create Project"), QDialogButtonBox::ApplyRole);
        m_primaryButton->setProperty("primary", true);
        connect(m_primaryButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandlePrimaryButton);
        connect(m_newProjectSettingsScreen, &ScreenWidget::ChangeScreenRequest, this, &CreateProjectCtrl::OnChangeScreenRequest);

        m_secondaryButton = buttons->addButton(tr("Back"), QDialogButtonBox::RejectRole);
        m_secondaryButton->setProperty("secondary", true);
        m_secondaryButton->setVisible(false);
        connect(m_secondaryButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandleSecondaryButton);

        Update();

        setLayout(vLayout);
    }

    ProjectManagerScreen CreateProjectCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::CreateProject;
    }

    // Called when pressing "Create New Project"
    void CreateProjectCtrl::NotifyCurrentScreen()
    {
        ScreenWidget* currentScreen = static_cast<ScreenWidget*>(m_stack->currentWidget());
        if (currentScreen)
        {
            currentScreen->NotifyCurrentScreen();
        }

        // Gather the enabled gems from the default project template when starting the create new project workflow.
        ReinitGemCatalogForSelectedTemplate();

        // make sure the gem repo has the latest details
        m_gemRepoScreen->Reinit();
    }

    void CreateProjectCtrl::HandleBackButton()
    {
        if (m_stack->currentIndex() > 0)
        {
            PreviousScreen();
        }
        else
        {
            emit GoToPreviousScreenRequest();
        }
    }

    void CreateProjectCtrl::HandleSecondaryButton()
    {
        if (m_stack->currentIndex() > 0)
        {
            // return to Project Settings page
            PreviousScreen();
        }
        else
        {
            // Configure Gems
            NextScreen();
        }
    }

    void CreateProjectCtrl::Update()
    {
        if (m_stack->currentWidget() == m_projectGemCatalogScreen)
        {
            m_header->setSubTitle(tr("Configure project with Gems"));
            m_secondaryButton->setVisible(false);
            m_primaryButton->setVisible(true);
        }
        else if (m_stack->currentWidget() == m_gemRepoScreen)
        {
            m_header->setSubTitle(tr("Remote Sources"));
            m_secondaryButton->setVisible(true);
            m_secondaryButton->setText(tr("Back"));
            m_primaryButton->setVisible(false);
        }
        else
        {
            m_header->setSubTitle(tr("Enter Project Details"));
            m_secondaryButton->setVisible(true);
            m_secondaryButton->setText(tr("Configure Gems"));
            m_primaryButton->setVisible(true);
        }
    }

    void CreateProjectCtrl::OnChangeScreenRequest(ProjectManagerScreen screen)
    {
        if (screen == ProjectManagerScreen::ProjectGemCatalog)
        {
            HandleSecondaryButton();
        }
        else if (screen == ProjectManagerScreen::GemRepos)
        {
            NextScreen();
        }
        else
        {
            emit ChangeScreenRequest(screen);
        }
    }

    void CreateProjectCtrl::NextScreen()
    {
        if (m_stack->currentIndex() < m_stack->count())
        {
            // special case where we need to download the template before proceeding
            if (m_stack->currentWidget() == m_newProjectSettingsScreen)
            {
                if (m_newProjectSettingsScreen->GetProjectTemplatePath().isEmpty() &&
                    !m_newProjectSettingsScreen->IsDownloadingTemplate())
                {
                    m_newProjectSettingsScreen->ShowDownloadTemplateDialog();
                    return;
                }
                else if (m_newProjectSettingsScreen->IsDownloadingTemplate())
                {
                    QMessageBox::warning(this, tr("Cannot configure gems"), tr("Cannot configure gems until the template has finished downloading."));
                    return;
                }
            }

            if(auto outcome = CurrentScreenIsValid(); outcome.IsSuccess())
            {
                m_stack->setCurrentIndex(m_stack->currentIndex() + 1);
                ScreenWidget* currentScreen = static_cast<ScreenWidget*>(m_stack->currentWidget());
                if (currentScreen)
                {
                    currentScreen->NotifyCurrentScreen();
                }
                Update();
            }
            else if (!outcome.GetError().isEmpty())
            {
                QMessageBox::warning(this, tr("Cannot continue"), outcome.GetError());
            }
            else 
            {
                QMessageBox::warning(this, tr("Invalid project settings"), tr("Please correct the indicated project settings and try again."));
            }
        }
    }

    void CreateProjectCtrl::PreviousScreen()
    {
        // we don't require the current screen to be valid when moving back
        if (m_stack->currentIndex() > 0)
        {
            m_stack->setCurrentIndex(m_stack->currentIndex() - 1);
            ScreenWidget* currentScreen = static_cast<ScreenWidget*>(m_stack->currentWidget());
            if (currentScreen)
            {
                currentScreen->NotifyCurrentScreen();
            }
            Update();
        }
    }

    void CreateProjectCtrl::HandlePrimaryButton()
    {
        CreateProject();
    }

    AZ::Outcome<void, QString> CreateProjectCtrl::CurrentScreenIsValid()
    {
        if (m_stack->currentWidget() == m_newProjectSettingsScreen)
        {
            return m_newProjectSettingsScreen->Validate();
        }

        return AZ::Success();
    }

    void CreateProjectCtrl::CreateProject()
    {
        AZ::Outcome<void, QString> settingsValidation = m_newProjectSettingsScreen->Validate();
        if (settingsValidation.IsSuccess())
        {
            if (!m_projectGemCatalogScreen->GetDownloadController()->IsDownloadQueueEmpty())
            {
                QMessageBox::critical(this, tr("Gems downloading"), tr("You must wait for gems to finish downloading before continuing."));
                return;
            }

            ProjectInfo projectInfo = m_newProjectSettingsScreen->GetProjectInfo();
            QString projectTemplatePath = m_newProjectSettingsScreen->GetProjectTemplatePath();

            // create in 2 steps for better error handling
            auto createResult = PythonBindingsInterface::Get()->CreateProject(projectTemplatePath, projectInfo, /*registerProject*/false);
            if (!createResult)
            {
                const IPythonBindings::ErrorPair& error = createResult.GetError();
                ProjectUtils::DisplayDetailedError(tr("Failed to create project"), error.first, error.second, this);
                return;
            }

            // RegisterProject will check compatibility and prompt user to continue if issues found
            // it will also handle detailed error messaging
            if(!ProjectUtils::RegisterProject(projectInfo.m_path, this))
            {
                // Since the project files were created during this workflow, but register project flow was cancelled or errored out,
                // clean up the created files here.
                [[maybe_unused]] bool filesDeleted = ProjectUtils::DeleteProjectFiles(projectInfo.m_path, /*force*/ true);
                AZ_Warning("O3DE", filesDeleted, "Unable to delete invalid new project files at %s", projectInfo.m_path.toUtf8().constData());
                return;
            }

            const ProjectGemCatalogScreen::ConfiguredGemsResult gemResult = m_projectGemCatalogScreen->ConfigureGemsForProject(projectInfo.m_path);
            if (gemResult == ProjectGemCatalogScreen::ConfiguredGemsResult::Failed)
            {
                QMessageBox::critical(this, tr("Failed to configure gems"), tr("Failed to configure gems for template."));
            }
            if (gemResult != ProjectGemCatalogScreen::ConfiguredGemsResult::Success)
            {
                return;
            }

            projectInfo.m_needsBuild = true;
            emit NotifyBuildProject(projectInfo);
            emit ChangeScreenRequest(ProjectManagerScreen::Projects);
        }
        else
        {
            const QString& errorMessage = settingsValidation.GetError();
            if (errorMessage.isEmpty())
            {
                QMessageBox::warning(
                    this, tr("Invalid project settings"), tr("Please correct the indicated project settings and try again."));
            }
            else
            {
                QMessageBox::warning(this, tr("Invalid project settings"), errorMessage);
            }
        }
    }

    void CreateProjectCtrl::ReinitGemCatalogForSelectedTemplate()
    {
        const QString projectTemplatePath = m_newProjectSettingsScreen->GetProjectTemplatePath();
        if (projectTemplatePath.isEmpty())
        {
            return;
        }

        m_projectGemCatalogScreen->ReinitForProject(projectTemplatePath + "/Template");
    }
} // namespace O3DE::ProjectManager
