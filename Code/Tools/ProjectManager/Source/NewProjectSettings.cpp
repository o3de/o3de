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

#include <Qt/NewProjectSettings.h>

#include <Qt/ui_NewProjectSettings.h>

namespace O3DE::ProjectManager
{
    NewProjectSettings::NewProjectSettings(ProjectManagerWindow* window)
        : ScreenWidget(window)
        , m_ui(new Ui::NewProjectSettingsClass())
    {
        m_ui->setupUi(this);

        ConnectSlotsAndSignals();
    }

    NewProjectSettings::~NewProjectSettings()
    {
    }

    void NewProjectSettings::ConnectSlotsAndSignals()
    {
        QObject::connect(m_ui->backButton, &QPushButton::pressed, this, &NewProjectSettings::HandleBackButton);
        QObject::connect(m_ui->nextButton, &QPushButton::pressed, this, &NewProjectSettings::HandleNextButton);
    }

    void NewProjectSettings::HandleBackButton()
    {
        m_projectManagerWindow->ChangeToScreen(ProjectManagerScreen::FirstTimeUse);
    }
    void NewProjectSettings::HandleNextButton()
    {
        m_projectManagerWindow->ChangeToScreen(ProjectManagerScreen::GemCatalog);
    }

} // namespace O3DE::ProjectManager
