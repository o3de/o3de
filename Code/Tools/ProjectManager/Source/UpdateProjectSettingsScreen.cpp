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
#include <QDir>

namespace O3DE::ProjectManager
{
    UpdateProjectSettingsScreen::UpdateProjectSettingsScreen(QWidget* parent)
        : ProjectSettingsScreen(parent)
    {
    }

    ProjectManagerScreen UpdateProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::UpdateProjectSettings;
    }

    void UpdateProjectSettingsScreen::SetProjectInfo(const ProjectInfo& projectInfo)
    {
        m_projectName->lineEdit()->setText(projectInfo.m_projectName);
        m_projectPath->lineEdit()->setText(projectInfo.m_path);
    }

    bool UpdateProjectSettingsScreen::ValidateProjectPath()
    {
        bool projectPathIsValid = true;
        if (m_projectPath->lineEdit()->text().isEmpty())
        {
            projectPathIsValid = false;
            m_projectPath->setErrorLabelText(tr("Please provide a valid location."));
        }

        m_projectPath->setErrorLabelVisible(!projectPathIsValid);
        return projectPathIsValid;
    }

} // namespace O3DE::ProjectManager
