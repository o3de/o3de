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

#include <UpdateProjectSettingsScreen.h>
#include <FormBrowseEditWidget.h>
#include <FormLineEditWidget.h>

#include <QLineEdit>

namespace O3DE::ProjectManager
{
    UpdateProjectSettingsScreen::UpdateProjectSettingsScreen(QWidget* parent)
        : ProjectSettingsScreen(parent)
    {
        //connect(m_ui->gemsButton, &QPushButton::pressed, this, &ProjectSettingsScreen::HandleGemsButton);
    }

    ProjectManagerScreen UpdateProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::UpdateProjectSettings;
    }

    QString UpdateProjectSettingsScreen::GetTabText()
    {
        return tr("General");
    }

    bool UpdateProjectSettingsScreen::IsTab()
    {
        return true;
    }

    void UpdateProjectSettingsScreen::SetProjectInfo(const ProjectInfo& projectInfo)
    {
        m_projectName->lineEdit()->setText(projectInfo.m_projectName);
        m_projectPath->lineEdit()->setText(projectInfo.m_path);
    }

} // namespace O3DE::ProjectManager
