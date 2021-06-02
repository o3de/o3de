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

#include <GemCatalog/GemCatalogScreen.h>
#include <PythonBindingsInterface.h>
#include <ScreenHeaderWidget.h>
#include <ScreensCtrl.h>
#include <UpdateProjectCtrl.h>
#include <UpdateProjectSettingsScreen.h>

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    UpdateProjectCtrl::UpdateProjectCtrl(QWidget* parent)
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
        m_gemCatalogScreen = new GemCatalogScreen();

        m_stack = new QStackedWidget(this);
        m_stack->setObjectName("body");
        m_stack->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        vLayout->addWidget(m_stack);

        QFrame* m_tabFrameWidget = new QFrame(this);
        m_tabFrameWidget->setObjectName("projectSettingsTopFrame");
        QHBoxLayout* topBarHLayout = new QHBoxLayout();
        topBarHLayout->setContentsMargins(0, 0, 0, 0);
        m_tabFrameWidget->setLayout(topBarHLayout);

        QTabWidget* tabWidget = new QTabWidget();
        tabWidget->setObjectName("projectSettingsTab");
        tabWidget->tabBar()->setObjectName("projectSettingsTabBar");
        tabWidget->addTab(m_updateSettingsScreen, tr("General"));

        QPushButton* gemsButton = new QPushButton(tr("Add More Gems"), this);
        topBarHLayout->addWidget(gemsButton);
        tabWidget->setCornerWidget(gemsButton);

        topBarHLayout->addWidget(tabWidget);

        m_stack->addWidget(m_tabFrameWidget);
        m_stack->addWidget(m_gemCatalogScreen);

        QDialogButtonBox* backNextButtons = new QDialogButtonBox();
        backNextButtons->setObjectName("footer");
        vLayout->addWidget(backNextButtons);

        m_backButton = backNextButtons->addButton(tr("Back"), QDialogButtonBox::RejectRole);
        m_backButton->setProperty("secondary", true);
        m_nextButton = backNextButtons->addButton(tr("Next"), QDialogButtonBox::ApplyRole);

        connect(gemsButton, &QPushButton::clicked, this, &UpdateProjectCtrl::HandleGemsButton);
        connect(m_backButton, &QPushButton::clicked, this, &UpdateProjectCtrl::HandleBackButton);
        connect(m_nextButton, &QPushButton::clicked, this, &UpdateProjectCtrl::HandleNextButton);
        connect(reinterpret_cast<ScreensCtrl*>(parent), &ScreensCtrl::NotifyCurrentProject, this, &UpdateProjectCtrl::UpdateCurrentProject);

        Update();
        setLayout(vLayout);
    }

    ProjectManagerScreen UpdateProjectCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::UpdateProject;
    }

    void UpdateProjectCtrl::NotifyCurrentScreen()
    {
        m_stack->setCurrentWidget(m_tabFrameWidget);
        Update();
    }

    void UpdateProjectCtrl::HandleGemsButton()
    {
        m_stack->setCurrentWidget(m_gemCatalogScreen);
        Update();
    }

    void UpdateProjectCtrl::HandleBackButton()
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
    void UpdateProjectCtrl::HandleNextButton()
    {
        ScreenWidget* currentScreen = reinterpret_cast<ScreenWidget*>(m_stack->currentWidget());
        ProjectManagerScreen screenEnum = currentScreen->GetScreenEnum();

        if (screenEnum == ProjectManagerScreen::UpdateProjectSettings)
        {
            if (m_updateSettingsScreen)
            {
                if (!m_updateSettingsScreen->Validate())
                {
                    QMessageBox::critical(this, tr("Invalid project settings"), tr("Invalid project settings"));
                    return;
                }

                m_projectInfo = m_updateSettingsScreen->GetProjectInfo();

                bool result = PythonBindingsInterface::Get()->UpdateProject(m_projectInfo);
                if (!result)
                {
                    QMessageBox::critical(this, tr("Project update failed"), tr("Failed to update project."));
                }
            }
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
        ScreenWidget* currentScreen = reinterpret_cast<ScreenWidget*>(m_stack->currentWidget());
        if (currentScreen && currentScreen->GetScreenEnum() == ProjectManagerScreen::GemCatalog)
        {
            m_header->setSubTitle(QString(tr("Add More Gems to \"%1\"")).arg(m_projectInfo.m_projectName));
            m_nextButton->setText(tr("Confirm"));
        }
        else
        {
            m_header->setSubTitle(QString(tr("Edit Project Settings: \"%1\"")).arg(m_projectInfo.m_projectName));
            m_nextButton->setText(tr("Save"));
        }
    }

    void UpdateProjectCtrl::UpdateSettingsScreen()
    {
        m_updateSettingsScreen->SetProjectInfo(m_projectInfo);
    }

} // namespace O3DE::ProjectManager
