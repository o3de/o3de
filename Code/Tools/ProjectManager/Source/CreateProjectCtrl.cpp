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

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

namespace O3DE::ProjectManager
{
    CreateProjectCtrl::CreateProjectCtrl(QWidget* parent)
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

        connect(m_backButton, &QPushButton::pressed, this, &CreateProjectCtrl::HandleBackButton);
        connect(m_nextButton, &QPushButton::pressed, this, &CreateProjectCtrl::HandleNextButton);

        m_screensOrder =
        {
            ProjectManagerScreen::NewProjectSettings,
            ProjectManagerScreen::GemCatalog
        };
        m_screensCtrl->BuildScreens(m_screensOrder);
        m_screensCtrl->ForceChangeToScreen(ProjectManagerScreen::NewProjectSettings, false);

        UpdateNextButtonText();
    }

    ProjectManagerScreen CreateProjectCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::CreateProject;
    }

    void CreateProjectCtrl::HandleBackButton()
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
    void CreateProjectCtrl::HandleNextButton()
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

        if (screenOrderIter != m_screensOrder.end())
        {
            m_screensCtrl->ChangeToScreen(*screenOrderIter);
            UpdateNextButtonText();
        }
        else
        {
            auto result = PythonBindingsInterface::Get()->CreateProject(m_projectTemplatePath, m_projectInfo);
            if (result.IsSuccess())
            {
                emit ChangeScreenRequest(ProjectManagerScreen::ProjectsHome);
            }
            else
            {
                QMessageBox::critical(this, tr("Project creation failed"), tr("Failed to create project."));
            }
        }
    }

    void CreateProjectCtrl::UpdateNextButtonText()
    {
        QString nextButtonText = tr("Next");
        if (m_screensCtrl->GetCurrentScreen()->GetScreenEnum() == ProjectManagerScreen::GemCatalog)
        {
            nextButtonText = tr("Create Project");
        }
        m_nextButton->setText(nextButtonText);
    }

} // namespace O3DE::ProjectManager
