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

#include <ProjectSettingsCtrl.h>
#include <ScreensCtrl.h>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>

namespace O3DE::ProjectManager
{
    ProjectSettingsCtrl::ProjectSettingsCtrl(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        setLayout(vLayout);

        m_screensCtrl = new ScreensCtrl();
        vLayout->addWidget(m_screensCtrl);

        QDialogButtonBox* backNextButtons = new QDialogButtonBox();
        vLayout->addWidget(backNextButtons);

        m_backButton = backNextButtons->addButton("Back", QDialogButtonBox::RejectRole);
        m_nextButton = backNextButtons->addButton("Next", QDialogButtonBox::ApplyRole);

        QObject::connect(m_backButton, &QPushButton::pressed, this, &ProjectSettingsCtrl::HandleBackButton);
        QObject::connect(m_nextButton, &QPushButton::pressed, this, &ProjectSettingsCtrl::HandleNextButton);

        m_screensOrder =
        {
            ProjectManagerScreen::NewProjectSettings,
            ProjectManagerScreen::GemCatalog
        };
        m_screensCtrl->BuildScreens(m_screensOrder);
        m_screensCtrl->ForceChangeToScreen(ProjectManagerScreen::NewProjectSettings);
        UpdateNextButtonText();
    }

    ProjectManagerScreen ProjectSettingsCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::NewProjectSettingsCore;
    }

    void ProjectSettingsCtrl::HandleBackButton()
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
    void ProjectSettingsCtrl::HandleNextButton()
    {
        ProjectManagerScreen screenEnum = m_screensCtrl->GetCurrentScreen()->GetScreenEnum();
        auto screenOrderIter = m_screensOrder.begin();
        for (; screenOrderIter != m_screensOrder.end(); ++screenOrderIter)
        {
            if (*screenOrderIter == screenEnum)
            {
                ++screenOrderIter;
                break;
            }
        }

        if (screenOrderIter != m_screensOrder.end())
        {
            m_screensCtrl->ChangeToScreen(*screenOrderIter);
            UpdateNextButtonText();
        }
        else
        {
            emit ChangeScreenRequest(ProjectManagerScreen::ProjectsHome);
        }
    }

    void ProjectSettingsCtrl::UpdateNextButtonText()
    {
        m_nextButton->setText(m_screensCtrl->GetCurrentScreen()->GetNextButtonText());
    }

} // namespace O3DE::ProjectManager
