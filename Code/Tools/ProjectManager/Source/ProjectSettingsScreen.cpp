/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectSettingsScreen.h>
#include <FormFolderBrowseEditWidget.h>
#include <FormLineEditWidget.h>
#include <PathValidator.h>
#include <PythonBindingsInterface.h>

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QStandardPaths>
#include <QScrollArea>

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

        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);

        QWidget* scrollWidget = new QWidget(this);
        scrollArea->setWidget(scrollWidget);

        m_verticalLayout = new QVBoxLayout();
        m_verticalLayout->setMargin(0);
        m_verticalLayout->setAlignment(Qt::AlignTop);
        scrollWidget->setLayout(m_verticalLayout);

        m_projectName = new FormLineEditWidget(tr("Project name"), "", this);
        connect(m_projectName->lineEdit(), &QLineEdit::textChanged, this, &ProjectSettingsScreen::OnProjectNameUpdated);
        m_verticalLayout->addWidget(m_projectName);

        m_projectVersion = new FormLineEditWidget(tr("Project version"), "1.0.0", this);
        m_verticalLayout->addWidget(m_projectVersion);

        m_projectPath = new FormFolderBrowseEditWidget(tr("Project Location"), "", this);
        connect(m_projectPath->lineEdit(), &QLineEdit::textChanged, this, &ProjectSettingsScreen::OnProjectPathUpdated);
        m_verticalLayout->addWidget(m_projectPath);

        projectSettingsFrame->setLayout(m_verticalLayout);

        m_horizontalLayout->addWidget(projectSettingsFrame);

        setLayout(m_horizontalLayout);
    }

    ProjectManagerScreen ProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::Invalid;
    }

    QString ProjectSettingsScreen::GetDefaultProjectPath()
    {
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            QDir path(QDir::toNativeSeparators(engineInfoResult.GetValue().m_defaultProjectsFolder));
            if (path.exists())
            {
                defaultPath = path.absolutePath();
            }
        }
        return defaultPath;
    }

    ProjectInfo ProjectSettingsScreen::GetProjectInfo()
    {
        ProjectInfo projectInfo;
        projectInfo.m_projectName = m_projectName->lineEdit()->text();
        projectInfo.m_version = m_projectVersion->lineEdit()->text();
        // currently we don't have separate fields for changing the project name and display name 
        projectInfo.m_displayName = projectInfo.m_projectName;
        projectInfo.m_path = m_projectPath->lineEdit()->text();
        return projectInfo;
    }

    bool ProjectSettingsScreen::ValidateProjectName() const
    {
        bool projectNameIsValid = true;
        if (m_projectName->lineEdit()->text().isEmpty())
        {
            projectNameIsValid = false;
            m_projectName->setErrorLabelText(tr("Please provide a project name."));
        }
        else
        {
            // this validation should roughly match the utils.validate_identifier which the cli
            // uses to validate project names
            QRegExp validProjectNameRegex("[A-Za-z][A-Za-z0-9_-]{0,63}");
            const bool result = validProjectNameRegex.exactMatch(m_projectName->lineEdit()->text());
            if (!result)
            {
                projectNameIsValid = false;
                m_projectName->setErrorLabelText(
                    tr("Project names must start with a letter and consist of up to 64 letter, number, '_' or '-' characters"));
            }
        }

        m_projectName->setErrorLabelVisible(!projectNameIsValid);
        return projectNameIsValid;
    }

    bool ProjectSettingsScreen::ValidateProjectPath() const
    {
        bool projectPathIsValid = true;
        QDir path(m_projectPath->lineEdit()->text());
        if (!path.isAbsolute())
        {
            projectPathIsValid = false;
            m_projectPath->setErrorLabelText(tr("Please provide an absolute path for the project location."));
        }
        else if (path.exists() && !path.isEmpty())
        {
            projectPathIsValid = false;
            m_projectPath->setErrorLabelText(tr("This folder exists and isn't empty.  Please choose a different location."));
        }

        m_projectPath->setErrorLabelVisible(!projectPathIsValid);
        return projectPathIsValid;
    }

    void ProjectSettingsScreen::OnProjectNameUpdated()
    {
        ValidateProjectName();
    }

    void ProjectSettingsScreen::OnProjectPathUpdated()
    {
        ValidateProjectName() && ValidateProjectPath();
    }

    AZ::Outcome<void, QString> ProjectSettingsScreen::Validate() const
    {
        if (ValidateProjectName() && ValidateProjectPath())
        {
            return AZ::Success();
        }

        // Returning empty string to use the default error message
        return AZ::Failure<QString>("");
    }
} // namespace O3DE::ProjectManager
