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

        QWidget* header = new QWidget();
        header->setObjectName("header");
        {
            QHBoxLayout* headerLayout = new QHBoxLayout();
            headerLayout->setAlignment(Qt::AlignLeft);
            headerLayout->setContentsMargins(0,0,0,0);

            QPushButton* headerBackButton = new QPushButton();
            headerBackButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            connect(headerBackButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandleBackButton);
            headerLayout->addWidget(headerBackButton);

            QVBoxLayout* titleLayout = new QVBoxLayout();
            m_title = new QLabel(tr("Create a New Project"));
            m_title->setObjectName("headerTitle");
            titleLayout->addWidget(m_title);

            m_subtitle = new QLabel(tr("Enter Project Details"));
            m_subtitle->setObjectName("headerSubTitle");
            titleLayout->addWidget(m_subtitle);

            headerLayout->addLayout(titleLayout);

            // TODO add a progress widget
            header->setLayout(headerLayout);
        }
        vLayout->addWidget(header);

        m_stack = new QStackedWidget(this);
        m_stack->setObjectName("body");
        m_stack->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding));
        m_stack->addWidget(new NewProjectSettingsScreen());
        m_stack->addWidget(new GemCatalogScreen());
        vLayout->addWidget(m_stack);

        QDialogButtonBox* backNextButtons = new QDialogButtonBox();
        backNextButtons->setObjectName("footer");
        vLayout->addWidget(backNextButtons);

        m_backButton = backNextButtons->addButton(tr("Back"), QDialogButtonBox::RejectRole);
        m_backButton->setProperty("secondary", true);
        m_nextButton = backNextButtons->addButton(tr("Next"), QDialogButtonBox::ApplyRole);

        connect(m_backButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandleBackButton);
        connect(m_nextButton, &QPushButton::clicked, this, &CreateProjectCtrl::HandleNextButton);

        Update();
        setLayout(vLayout);
    }

    ProjectManagerScreen CreateProjectCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::CreateProject;
    }

    void CreateProjectCtrl::HandleBackButton()
    {
        if (m_stack->currentIndex() > 0)
        {
            m_stack->setCurrentIndex(m_stack->currentIndex() - 1);
            Update();
        }
        else
        {
            emit GotoPreviousScreenRequest();
        }
    }
    void CreateProjectCtrl::HandleNextButton()
    {
        ScreenWidget* currentScreen = reinterpret_cast<ScreenWidget*>(m_stack->currentWidget());
        ProjectManagerScreen screenEnum = currentScreen->GetScreenEnum();

        if (screenEnum == ProjectManagerScreen::NewProjectSettings)
        {
            auto newProjectScreen = reinterpret_cast<NewProjectSettingsScreen*>(currentScreen);
            if (newProjectScreen)
            {
                if (!newProjectScreen->Validate())
                {
                    QMessageBox::critical(this, tr("Invalid project settings"), tr("Invalid project settings"));
                    return;
                }

                m_projectInfo         = newProjectScreen->GetProjectInfo();
                m_projectTemplatePath = newProjectScreen->GetProjectTemplatePath();
            }
        }

        if (m_stack->currentIndex() != m_stack->count() - 1)
        {
            m_stack->setCurrentIndex(m_stack->currentIndex() + 1);
            Update();
        }
        else
        {
            auto result = PythonBindingsInterface::Get()->CreateProject(m_projectTemplatePath, m_projectInfo);
            if (result.IsSuccess())
            {
                // adding gems is not implemented yet because we don't know what targets to add or how to add them
                emit ChangeScreenRequest(ProjectManagerScreen::ProjectsHome);
            }
            else
            {
                QMessageBox::critical(this, tr("Project creation failed"), tr("Failed to create project."));
            }
        }
    }

    void CreateProjectCtrl::Update()
    {
        ScreenWidget* currentScreen = reinterpret_cast<ScreenWidget*>(m_stack->currentWidget());
        if (currentScreen && currentScreen->GetScreenEnum() == ProjectManagerScreen::GemCatalog)
        {
            m_title->setText(tr("Create Project"));
            m_subtitle->setText(tr("Configure project with Gems"));
            m_nextButton->setText(tr("Create Project"));
        }
        else
        {
            m_title->setText(tr("Create Project"));
            m_subtitle->setText(tr("Enter Project Details"));
            m_nextButton->setText(tr("Next"));
        }
    }

} // namespace O3DE::ProjectManager
