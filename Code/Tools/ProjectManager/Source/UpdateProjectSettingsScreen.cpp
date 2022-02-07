/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <UpdateProjectSettingsScreen.h>
#include <ProjectManagerDefs.h>
#include <FormImageBrowseEditWidget.h>
#include <FormLineEditWidget.h>

#include <QVBoxLayout>
#include <QLineEdit>
#include <QDir>
#include <QLabel>
#include <QFileInfo>

namespace O3DE::ProjectManager
{
    UpdateProjectSettingsScreen::UpdateProjectSettingsScreen(QWidget* parent)
        : ProjectSettingsScreen(parent)
        , m_userChangedPreview(false)
    {
        m_projectPreview = new FormImageBrowseEditWidget(tr("Project Preview"), "", this);
        m_projectPreview->lineEdit()->setReadOnly(true);
        connect(m_projectPreview->lineEdit(), &QLineEdit::textChanged, this, &ProjectSettingsScreen::Validate);
        connect(m_projectPreview->lineEdit(), &QLineEdit::textChanged, this, &UpdateProjectSettingsScreen::PreviewPathChanged);
        connect(m_projectPath->lineEdit(), &QLineEdit::textChanged, this, &UpdateProjectSettingsScreen::UpdateProjectPreviewPath);
        m_verticalLayout->addWidget(m_projectPreview);

        QVBoxLayout* previewExtrasLayout = new QVBoxLayout(this);
        previewExtrasLayout->setAlignment(Qt::AlignLeft);
        previewExtrasLayout->setContentsMargins(50, 0, 0, 0);

        QLabel* projectPreviewLabel = new QLabel(tr("Select an image (PNG). Minimum %1 x %2 pixels.")
            .arg(QString::number(ProjectPreviewImageWidth), QString::number(ProjectPreviewImageHeight)));
        projectPreviewLabel->setObjectName("projectPreviewLabel");
        previewExtrasLayout->addWidget(projectPreviewLabel);

        m_projectPreviewImage = new QLabel(this);
        m_projectPreviewImage->setFixedSize(ProjectPreviewImageWidth, ProjectPreviewImageHeight);
        m_projectPreviewImage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        previewExtrasLayout->addWidget(m_projectPreviewImage);

        m_verticalLayout->addLayout(previewExtrasLayout);
    }

    ProjectManagerScreen UpdateProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::UpdateProjectSettings;
    }

    ProjectInfo UpdateProjectSettingsScreen::GetProjectInfo()
    {
        m_projectInfo.m_displayName = m_projectName->lineEdit()->text();
        m_projectInfo.m_path = m_projectPath->lineEdit()->text();

        if (m_userChangedPreview)
        {
            m_projectInfo.m_iconPath = ProjectPreviewImagePath;
            m_projectInfo.m_newPreviewImagePath = m_projectPreview->lineEdit()->text();
        }
        return m_projectInfo;
    }

    void UpdateProjectSettingsScreen::SetProjectInfo(const ProjectInfo& projectInfo)
    {
        m_projectInfo = projectInfo;

        m_projectName->lineEdit()->setText(projectInfo.GetProjectDisplayName());

        m_projectPath->lineEdit()->setText(projectInfo.m_path);
        UpdateProjectPreviewPath();
    }

    void UpdateProjectSettingsScreen::UpdateProjectPreviewPath()
    {
        if (!m_userChangedPreview)
        {
            m_projectPreview->lineEdit()->setText(QDir(m_projectPath->lineEdit()->text()).filePath(m_projectInfo.m_iconPath));
            // Setting the text sets m_userChangedPreview to true
            // Set it back to false because it should only be true when changed by user
            m_userChangedPreview = false;
        }
    }

    bool UpdateProjectSettingsScreen::Validate()
    {
        return ProjectSettingsScreen::Validate() && ValidateProjectPreview();
    }

    void UpdateProjectSettingsScreen::ResetProjectPreviewPath()
    {
        m_userChangedPreview = false;
        UpdateProjectPreviewPath();
    }

    void UpdateProjectSettingsScreen::PreviewPathChanged()
    {
        m_userChangedPreview = true;

        // Update with latest image
        m_projectPreviewImage->setPixmap(
            QPixmap(m_projectPreview->lineEdit()->text()).scaled(m_projectPreviewImage->size(), Qt::KeepAspectRatioByExpanding));
    }

    bool UpdateProjectSettingsScreen::ValidateProjectPath()
    {
        bool projectPathIsValid = true;
        QDir path(m_projectPath->lineEdit()->text());
        if (!path.isAbsolute())
        {
            projectPathIsValid = false;
            m_projectPath->setErrorLabelText(tr("Please provide an absolute path for the project location."));
        }

        m_projectPath->setErrorLabelVisible(!projectPathIsValid);
        return projectPathIsValid;
    }

    bool UpdateProjectSettingsScreen::ValidateProjectPreview()
    {
        bool projectPreviewIsValid = true;

        if (m_projectPreview->lineEdit()->text().isEmpty())
        {
            projectPreviewIsValid = false;
            m_projectPreview->setErrorLabelText(tr("Please select a file."));
        }
        else
        {
            if (m_userChangedPreview)
            {
                QFileInfo previewFile(m_projectPreview->lineEdit()->text());
                if (!previewFile.exists() || !previewFile.isFile())
                {
                    projectPreviewIsValid = false;
                    m_projectPreview->setErrorLabelText(tr("Please select a valid png file."));
                }
                else
                {
                    QString fileType = previewFile.completeSuffix().toLower();
                    if (fileType != "png")
                    {
                        projectPreviewIsValid = false;
                        m_projectPreview->setErrorLabelText(tr("Please select a png image."));
                    }
                }
            }
        }

        m_projectPreview->setErrorLabelVisible(!projectPreviewIsValid);
        return projectPreviewIsValid;
    }

} // namespace O3DE::ProjectManager
