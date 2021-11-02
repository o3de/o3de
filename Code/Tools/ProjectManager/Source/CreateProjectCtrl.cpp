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
#include <GemCatalog/GemCatalogScreen.h>
#include <ProjectUtils.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QStackedWidget>
#include <QLabel>
#include <QSizePolicy>

namespace O3DE::ProjectManager
{
    CreateProjectCtrl::CreateProjectCtrl(QWidget* parent)
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

        m_newProjectSettingsScreen = new NewProjectSettingsScreen(this);
        m_stack->addWidget(m_newProjectSettingsScreen);

        m_gemCatalogScreen = new GemCatalogScreen(this);
        m_stack->addWidget(m_gemCatalogScreen);
        vLayout->addWidget(m_stack);

        connect(m_gemCatalogScreen, &ScreenWidget::ChangeScreenRequest, this, &CreateProjectCtrl::OnChangeScreenRequest);

        // When there are multiple project templates present, we re-gather the gems when changing the selected the project template.
        connect(m_newProjectSettingsScreen, &NewProjectSettingsScreen::OnTemplateSelectionChanged, this, [=](int oldIndex, [[maybe_unused]] int newIndex)
            {
                const GemModel* gemModel = m_gemCatalogScreen->GetGemModel();
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

#ifdef TEMPLATE_GEM_CONFIGURATION_ENABLED
        connect(m_newProjectSettingsScreen, &ScreenWidget::ChangeScreenRequest, this, &CreateProjectCtrl::OnChangeScreenRequest);

        m_secondaryButton = buttons->addButton(tr("Back"), QDialogButtonBox::RejectRole);
        m_secondaryButton->setProperty("secondary", true);
        m_secondaryButton->setVisible(false);
        connect(m_secondaryButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandleSecondaryButton);

        Update();
#endif // TEMPLATE_GEM_CONFIGURATION_ENABLED

        m_primaryButton = buttons->addButton(tr("Create Project"), QDialogButtonBox::ApplyRole);
        connect(m_primaryButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandlePrimaryButton);

        setLayout(vLayout);
    }

    ProjectManagerScreen CreateProjectCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::CreateProject;
    }

    // Called when pressing "Create New Project"
    void CreateProjectCtrl::NotifyCurrentScreen()
    {
        ScreenWidget* currentScreen = reinterpret_cast<ScreenWidget*>(m_stack->currentWidget());
        if (currentScreen)
        {
            currentScreen->NotifyCurrentScreen();
        }

        // Gather the enabled gems from the default project template when starting the create new project workflow.
        ReinitGemCatalogForSelectedTemplate();
    }

    void CreateProjectCtrl::HandleBackButton()
    {
        if (m_stack->currentIndex() > 0)
        {
#ifdef TEMPLATE_GEM_CONFIGURATION_ENABLED
            PreviousScreen();
#endif // TEMPLATE_GEM_CONFIGURATION_ENABLED

        }
        else
        {
            emit GoToPreviousScreenRequest();
        }
    }

#ifdef TEMPLATE_GEM_CONFIGURATION_ENABLED
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
        if (m_stack->currentWidget() == m_gemCatalogScreen)
        {
            m_header->setSubTitle(tr("Configure project with Gems"));
            m_secondaryButton->setVisible(false);
        }
        else
        {
            m_header->setSubTitle(tr("Enter Project Details"));
            m_secondaryButton->setVisible(true);
            m_secondaryButton->setText(tr("Configure Gems"));
        }
    }

    void CreateProjectCtrl::OnChangeScreenRequest(ProjectManagerScreen screen)
    {
        if (screen == ProjectManagerScreen::GemCatalog)
        {
            HandleSecondaryButton();
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
            if(CurrentScreenIsValid())
            {
                m_stack->setCurrentIndex(m_stack->currentIndex() + 1);

                Update();
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
            Update();
        }
    }
#endif // TEMPLATE_GEM_CONFIGURATION_ENABLED

    void CreateProjectCtrl::HandlePrimaryButton()
    {
        CreateProject();
    }

    bool CreateProjectCtrl::CurrentScreenIsValid()
    {
        if (m_stack->currentWidget() == m_newProjectSettingsScreen)
        {
            return m_newProjectSettingsScreen->Validate();
        }

        return true;
    }

    void CreateProjectCtrl::CreateProject()
    {
        if (ProjectUtils::FindSupportedCompiler(this))
        {
            if (m_newProjectSettingsScreen->Validate())
            {
                ProjectInfo projectInfo = m_newProjectSettingsScreen->GetProjectInfo();
                QString projectTemplatePath = m_newProjectSettingsScreen->GetProjectTemplatePath();

                auto result = PythonBindingsInterface::Get()->CreateProject(projectTemplatePath, projectInfo);
                if (result.IsSuccess())
                {
                    // automatically register the project
                    PythonBindingsInterface::Get()->AddProject(projectInfo.m_path);

#ifdef TEMPLATE_GEM_CONFIGURATION_ENABLED
                    const GemCatalogScreen::EnableDisableGemsResult gemResult = m_gemCatalogScreen->EnableDisableGemsForProject(projectInfo.m_path);
                    if (gemResult == GemCatalogScreen::EnableDisableGemsResult::Failed)
                    {
                        QMessageBox::critical(this, tr("Failed to configure gems"), tr("Failed to configure gems for template."));
                    }
                    if (gemResult != GemCatalogScreen::EnableDisableGemsResult::Success)
                    {
                        return;
                    }
#endif // TEMPLATE_GEM_CONFIGURATION_ENABLED

                    projectInfo.m_needsBuild = true;
                    emit NotifyBuildProject(projectInfo);
                    emit ChangeScreenRequest(ProjectManagerScreen::Projects);
                }
                else
                {
                    QMessageBox::critical(this, tr("Project creation failed"), tr("Failed to create project."));
                }
            }
            else
            {
                QMessageBox::warning(
                    this, tr("Invalid project settings"), tr("Please correct the indicated project settings and try again."));
            }
        }
    }

    void CreateProjectCtrl::ReinitGemCatalogForSelectedTemplate()
    {
        const QString projectTemplatePath = m_newProjectSettingsScreen->GetProjectTemplatePath();
        m_gemCatalogScreen->ReinitForProject(projectTemplatePath + "/Template");
    }
} // namespace O3DE::ProjectManager
