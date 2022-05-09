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
#include <QPushButton>

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
        previewExtrasLayout->setAlignment(Qt::AlignTop);
        previewExtrasLayout->setContentsMargins(30, 45, 30, 0);

        QLabel* projectPreviewLabel = new QLabel(tr("Project Preview"));
        previewExtrasLayout->addWidget(projectPreviewLabel);

        m_projectPreviewImage = new QLabel(this);
        m_projectPreviewImage->setFixedSize(ProjectPreviewImageWidth, ProjectPreviewImageHeight);
        m_projectPreviewImage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        previewExtrasLayout->addWidget(m_projectPreviewImage);

        QLabel* projectPreviewInfoLabel = new QLabel(tr("Select an image (PNG). Minimum %1 x %2 pixels.")
            .arg(QString::number(ProjectPreviewImageWidth), QString::number(ProjectPreviewImageHeight)));
        projectPreviewInfoLabel->setObjectName("projectSmallInfoLabel");
        projectPreviewInfoLabel->setWordWrap(true);
        previewExtrasLayout->addWidget(projectPreviewInfoLabel);

        m_horizontalLayout->addLayout(previewExtrasLayout);

        m_verticalLayout->addSpacing(10);

        // Collapse button
        QHBoxLayout* advancedCollapseLayout = new QHBoxLayout();
        advancedCollapseLayout->setContentsMargins(50, 0, 0, 0);

        m_advancedSettingsCollapseButton = new QPushButton();
        m_advancedSettingsCollapseButton->setCheckable(true);
        m_advancedSettingsCollapseButton->setChecked(true);
        m_advancedSettingsCollapseButton->setFlat(true);
        m_advancedSettingsCollapseButton->setFocusPolicy(Qt::NoFocus);
        m_advancedSettingsCollapseButton->setFixedWidth(s_collapseButtonSize);
        connect(m_advancedSettingsCollapseButton, &QPushButton::clicked, this, &UpdateProjectSettingsScreen::UpdateAdvancedSettingsCollapseState);
        advancedCollapseLayout->addWidget(m_advancedSettingsCollapseButton);

        // Category title
        QLabel* advancedLabel = new QLabel(tr("Advanced Settings"));
        advancedLabel->setObjectName("projectSettingsSectionTitle");
        advancedCollapseLayout->addWidget(advancedLabel);
        m_verticalLayout->addLayout(advancedCollapseLayout);

        m_verticalLayout->addSpacing(5);

        // Everything in the advanced settings widget will be collapsed/uncollapsed
        {
            m_advancedSettingWidget = new QWidget();
            m_verticalLayout->addWidget(m_advancedSettingWidget);

            QVBoxLayout* advancedSettingsLayout = new QVBoxLayout();
            advancedSettingsLayout->setMargin(0);
            advancedSettingsLayout->setAlignment(Qt::AlignTop);
            m_advancedSettingWidget->setLayout(advancedSettingsLayout);

            m_projectId = new FormLineEditWidget(tr("Project ID"), "", this);
            connect(m_projectId->lineEdit(), &QLineEdit::textChanged, this, &UpdateProjectSettingsScreen::OnProjectIdUpdated);
            advancedSettingsLayout->addWidget(m_projectId);
        }

        UpdateAdvancedSettingsCollapseState();
    }

    ProjectManagerScreen UpdateProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::UpdateProjectSettings;
    }

    ProjectInfo UpdateProjectSettingsScreen::GetProjectInfo()
    {
        m_projectInfo.m_displayName = m_projectName->lineEdit()->text();
        m_projectInfo.m_path = m_projectPath->lineEdit()->text();
        m_projectInfo.m_id = m_projectId->lineEdit()->text();

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
        m_projectId->lineEdit()->setText(projectInfo.m_id);

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
        return ProjectSettingsScreen::Validate() && ValidateProjectPreview() && ValidateProjectId();
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

    void UpdateProjectSettingsScreen::OnProjectIdUpdated()
    {
        ValidateProjectId();
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

    bool UpdateProjectSettingsScreen::ValidateProjectId()
    {
        bool projectIdIsValid = true;
        if (m_projectId->lineEdit()->text().isEmpty())
        {
            projectIdIsValid = false;
            m_projectId->setErrorLabelText(tr("Project ID cannot be empty."));
        }

        m_projectId->setErrorLabelVisible(!projectIdIsValid);
        return projectIdIsValid;
    }

    void UpdateProjectSettingsScreen::UpdateAdvancedSettingsCollapseState()
    {
        if (m_advancedSettingsCollapseButton->isChecked())
        {
            m_advancedSettingsCollapseButton->setIcon(QIcon(":/ArrowDownLine.svg"));
            m_advancedSettingWidget->hide();
        }
        else
        {
            m_advancedSettingsCollapseButton->setIcon(QIcon(":/ArrowUpLine.svg"));
            m_advancedSettingWidget->show();
        }
    }

} // namespace O3DE::ProjectManager
