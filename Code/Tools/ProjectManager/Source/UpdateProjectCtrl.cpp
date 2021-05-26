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

#include <UpdateProjectCtrl.h>
#include <ScreensCtrl.h>
#include <PythonBindingsInterface.h>
#include <ProjectSettingsScreen.h>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

namespace O3DE::ProjectManager
{
    UpdateProjectCtrl::UpdateProjectCtrl(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        setLayout(vLayout);

        m_screensCtrl = new ScreensCtrl();
        vLayout->addWidget(m_screensCtrl);

        QDialogButtonBox* backNextButtons = new QDialogButtonBox();
        vLayout->addWidget(backNextButtons);

        m_backButton = backNextButtons->addButton(tr("Back"), QDialogButtonBox::RejectRole);
        m_nextButton = backNextButtons->addButton(tr("Next"), QDialogButtonBox::ApplyRole);

        connect(m_backButton, &QPushButton::pressed, this, &UpdateProjectCtrl::HandleBackButton);
        connect(m_nextButton, &QPushButton::pressed, this, &UpdateProjectCtrl::HandleNextButton);
        connect(reinterpret_cast<ScreensCtrl*>(parent), &ScreensCtrl::NotifyCurrentProject, this, &UpdateProjectCtrl::UpdateCurrentProject);

        m_screensOrder =
        {
            ProjectManagerScreen::ProjectSettings,
            ProjectManagerScreen::GemCatalog
        };
        m_screensCtrl->BuildScreens(m_screensOrder);
        m_screensCtrl->ForceChangeToScreen(ProjectManagerScreen::ProjectSettings, false);

        UpdateNextButtonText();

    }

    ProjectManagerScreen UpdateProjectCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::UpdateProject;
    }

    void UpdateProjectCtrl::HandleBackButton()
    {
        if (!m_screensCtrl->GotoPreviousScreen())
        {
            emit GotoPreviousScreenRequest();
        }
        else
        {
            UpdateNextButtonText();
        }
    }
    void UpdateProjectCtrl::HandleNextButton()
    {
        ScreenWidget* currentScreen = m_screensCtrl->GetCurrentScreen();
        ProjectManagerScreen screenEnum = currentScreen->GetScreenEnum();
        auto screenOrderIter = m_screensOrder.begin();
        for (; screenOrderIter != m_screensOrder.end(); ++screenOrderIter)
        {
            if (*screenOrderIter == screenEnum)
            {
                ++screenOrderIter;
                break;
            }
        }

        if (screenEnum == ProjectManagerScreen::ProjectSettings)
        {
            auto projectScreen = reinterpret_cast<ProjectSettingsScreen*>(currentScreen);
            if (projectScreen)
            {
                if (!projectScreen->Validate())
                {
                    QMessageBox::critical(this, tr("Invalid project settings"), tr("Invalid project settings"));
                    return;
                }

                m_projectInfo = projectScreen->GetProjectInfo();
            }
        }

        if (screenOrderIter != m_screensOrder.end())
        {
            m_screensCtrl->ChangeToScreen(*screenOrderIter);
            UpdateNextButtonText();
        }
        else
        {
            auto result = PythonBindingsInterface::Get()->UpdateProject(m_projectInfo);
            if (result)
            {
                emit ChangeScreenRequest(ProjectManagerScreen::ProjectsHome);
            }
            else
            {
                QMessageBox::critical(this, tr("Project update failed"), tr("Failed to update project."));
            }
        }
    }

    void UpdateProjectCtrl::UpdateCurrentProject(const QString& projectPath)
    {
        auto projectResult = PythonBindingsInterface::Get()->GetProject(projectPath);
        if (projectResult.IsSuccess())
        {
            m_projectInfo = projectResult.GetValue();
        }
    }

    void UpdateProjectCtrl::UpdateNextButtonText()
    {
        QString nextButtonText = tr("Continue");
        if (m_screensCtrl->GetCurrentScreen()->GetScreenEnum() == ProjectManagerScreen::GemCatalog)
        {
            nextButtonText = tr("Update Project");
        }
        m_nextButton->setText(nextButtonText);
    }

} // namespace O3DE::ProjectManager
