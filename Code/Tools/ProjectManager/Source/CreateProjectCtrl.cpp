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

#include <CreateProjectCtrl.h>
#include <ScreensCtrl.h>
#include <PythonBindingsInterface.h>
#include <NewProjectSettingsScreen.h>
#include <ScreenHeaderWidget.h>
#include <GemCatalog/GemCatalogScreen.h>

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
        connect(m_newProjectSettingsScreen, &ScreenWidget::ChangeScreenRequest, this, &CreateProjectCtrl::OnChangeScreenRequest);
        m_stack->addWidget(m_newProjectSettingsScreen);

        m_gemCatalogScreen = new GemCatalogScreen(this);
        m_stack->addWidget(m_gemCatalogScreen);
        vLayout->addWidget(m_stack);

        QDialogButtonBox* buttons = new QDialogButtonBox();
        buttons->setObjectName("footer");
        vLayout->addWidget(buttons);

        m_secondaryButton = buttons->addButton(tr("Back"), QDialogButtonBox::RejectRole);
        m_secondaryButton->setProperty("secondary", true);
        m_primaryButton = buttons->addButton(tr("Create Project"), QDialogButtonBox::ApplyRole);

        connect(m_secondaryButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandleSecondaryButton);
        connect(m_primaryButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandlePrimaryButton);

        Update();
        setLayout(vLayout);
    }

    ProjectManagerScreen CreateProjectCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::CreateProject;
    }

    void CreateProjectCtrl::NotifyCurrentScreen()
    {
        ScreenWidget* currentScreen = reinterpret_cast<ScreenWidget*>(m_stack->currentWidget());
        if (currentScreen)
        {
            currentScreen->NotifyCurrentScreen();
        }
    }

    void CreateProjectCtrl::HandleBackButton()
    {
        if (m_stack->currentIndex() > 0)
        {
            PreviousScreen();
        }
        else
        {
            emit GotoPreviousScreenRequest();
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

    void CreateProjectCtrl::HandlePrimaryButton()
    {
        CreateProject();
    }

    void CreateProjectCtrl::Update()
    {
        if (m_stack->currentWidget() == m_gemCatalogScreen)
        {
            m_header->setSubTitle(tr("Configure project with Gems"));
            m_secondaryButton->setText(tr("Back"));
        }
        else
        {
            m_header->setSubTitle(tr("Enter Project Details"));
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
        if (CurrentScreenIsValid() && m_stack->currentIndex() < m_stack->count())
        {
            m_stack->setCurrentIndex(m_stack->currentIndex() + 1);
            Update();
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
        if (m_newProjectSettingsScreen->Validate())
        {
            ProjectInfo projectInfo = m_newProjectSettingsScreen->GetProjectInfo();
            QString projectTemplatePath = m_newProjectSettingsScreen->GetProjectTemplatePath();

            auto result = PythonBindingsInterface::Get()->CreateProject(projectTemplatePath, projectInfo);
            if (result.IsSuccess())
            {
                // automatically register the project
                PythonBindingsInterface::Get()->AddProject(projectInfo.m_path);

                // adding gems is not implemented yet because we don't know what targets to add or how to add them
                emit ChangeScreenRequest(ProjectManagerScreen::Projects);
            }
            else
            {
                QMessageBox::critical(this, tr("Project creation failed"), tr("Failed to create project."));
            }
        }
        else
        {
            QMessageBox::warning(this, tr("Invalid project settings"), tr("Please correct the indicated project settings and try again."));
        }
    }

} // namespace O3DE::ProjectManager
