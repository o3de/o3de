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

#include <ProjectSettingsScreen.h>
#include <FormBrowseEditWidget.h>
#include <FormLineEditWidget.h>
#include <PathValidator.h>

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>

namespace O3DE::ProjectManager
{
    ProjectSettingsScreen::ProjectSettingsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        m_horizontalLayout = new QHBoxLayout(this);
        m_horizontalLayout->setAlignment(Qt::AlignLeft);
        m_horizontalLayout->setContentsMargins(0, 0, 0, 0);

        // if we don't provide a parent for this box layout the stylesheet doesn't take
        // if we don't set this in a frame (just use a sub-layout) all the content will align incorrectly horizontally
        QFrame* projectSettingsFrame = new QFrame(this);
        projectSettingsFrame->setObjectName("projectSettings");
        m_verticalLayout = new QVBoxLayout(this);

        // you cannot remove content margins in qss
        m_verticalLayout->setContentsMargins(0, 0, 0, 0);
        m_verticalLayout->setAlignment(Qt::AlignTop);
        {
            m_projectName = new FormLineEditWidget(tr("Project name"), "", this);
            m_projectName->setErrorLabelText(
                tr("A project with this name already exists at this location. Please choose a new name or location."));
            m_verticalLayout->addWidget(m_projectName);

            m_projectPath =
                new FormBrowseEditWidget(tr("Project Location"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), this);
            m_projectPath->lineEdit()->setReadOnly(true);
            m_projectPath->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
            m_projectPath->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
            m_verticalLayout->addWidget(m_projectPath);
        }
        projectSettingsFrame->setLayout(m_verticalLayout);

        m_horizontalLayout->addWidget(projectSettingsFrame);

        setLayout(m_horizontalLayout);
    }

    ProjectManagerScreen ProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::Invalid;
    }

    ProjectInfo ProjectSettingsScreen::GetProjectInfo()
    {
        ProjectInfo projectInfo;
        projectInfo.m_projectName = m_projectName->lineEdit()->text();
        projectInfo.m_path        = QDir::toNativeSeparators(m_projectPath->lineEdit()->text() + "/" + projectInfo.m_projectName);
        return projectInfo;
    }

    bool ProjectSettingsScreen::Validate()
    {
        bool projectNameIsValid = true;
        if (m_projectName->lineEdit()->text().isEmpty())
        {
            projectNameIsValid = false;
        }

        bool projectPathIsValid = true;
        if (m_projectPath->lineEdit()->text().isEmpty())
        {
            projectPathIsValid = false;
        }

        QDir path(QDir::toNativeSeparators(m_projectPath->lineEdit()->text() + "/" + m_projectName->lineEdit()->text()));
        if (path.exists() && !path.isEmpty())
        {
            projectPathIsValid = false;
        }

        return projectNameIsValid && projectPathIsValid;
    }
} // namespace O3DE::ProjectManager
