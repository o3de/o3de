/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AddRemoteProjectDialog.h>
#include <FormFolderBrowseEditWidget.h>
#include <TextOverflowWidget.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <ProjectUtils.h>
#include <PythonBindingsInterface.h>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDir>
#include <QTimer>

namespace O3DE::ProjectManager
{
    AddRemoteProjectDialog::AddRemoteProjectDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Add a remote project"));
        setModal(true);
        setObjectName("addRemoteProjectDialog");
        setFixedSize(QSize(760, 600));

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(30, 30, 25, 10);
        vLayout->setSpacing(0);
        vLayout->setAlignment(Qt::AlignTop);
        setLayout(vLayout);

        QLabel* instructionTitleLabel = new QLabel(tr("Please enter a remote URL for your project"), this);
        instructionTitleLabel->setObjectName("remoteProjectDialogInstructionTitleLabel");
        instructionTitleLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(instructionTitleLabel);

        vLayout->addSpacing(10);

        m_repoPath = new FormLineEditWidget(tr("Remote URL"), "", this);
        m_repoPath->setMinimumSize(QSize(600, 0));
        m_repoPath->setErrorLabelText(tr("Not a valid remote source."));
        m_repoPath->lineEdit()->setPlaceholderText("https://github.com/o3de/example.git");
        vLayout->addWidget(m_repoPath);

        vLayout->addSpacing(10);

        QHBoxLayout* warningHLayout = new QHBoxLayout();
        QLabel* warningIcon = new QLabel();
        warningIcon->setPixmap(QIcon(":/Warning.svg").pixmap(32, 32));
        warningIcon->setAlignment(Qt::AlignCenter);
        warningIcon->setFixedSize(32, 32);
        warningHLayout->addWidget(warningIcon);

        warningHLayout->addSpacing(10);

        QLabel* warningLabel = new QLabel(tr("Online repositories may contain files that could potentially harm your computer,"
            " please ensure you understand the risks before downloading from third-party sources."), this);
        warningLabel->setObjectName("remoteProjectDialogWarningLabel");
        warningLabel->setWordWrap(true);
        warningLabel->setAlignment(Qt::AlignLeft);
        warningHLayout->addWidget(warningLabel);

        vLayout->addLayout(warningHLayout);

        vLayout->addSpacing(10);

        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setObjectName("horizontalSeparatingLine");
        vLayout->addWidget(hLine);

        vLayout->addSpacing(10);

        m_downloadProjectLabel = new QLabel(tr("Download Project..."), this);
        m_downloadProjectLabel->setObjectName("remoteProjectDialogDownloadProjectLabel");
        m_downloadProjectLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(m_downloadProjectLabel);

        m_installPath = new FormFolderBrowseEditWidget(tr("Local project directory"));
        m_installPath->setMinimumSize(QSize(600, 0));
        vLayout->addWidget(m_installPath);

        vLayout->addSpacing(10);

        QHBoxLayout* buildHLayout = new QHBoxLayout(this);
        buildHLayout->setContentsMargins(0, 0, 0, 0);
        buildHLayout->setAlignment(Qt::AlignLeft);

        m_autoBuild = new QCheckBox(this);
        m_autoBuild->setChecked(true);
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_autoBuild);
        buildHLayout->addWidget(m_autoBuild);

        buildHLayout->addSpacing(10);

        m_buildToggleLabel = new QLabel(tr("Automatically build project"), this);
        m_buildToggleLabel->setAlignment(Qt::AlignLeft);
        buildHLayout->addWidget(m_buildToggleLabel);

        vLayout->addLayout(buildHLayout);

        vLayout->addSpacing(20);

        QGridLayout* extraInfoGridLayout = new QGridLayout(this);
        extraInfoGridLayout->setContentsMargins(0, 0, 0, 0);
        extraInfoGridLayout->setHorizontalSpacing(5);
        extraInfoGridLayout->setVerticalSpacing(15);
        extraInfoGridLayout->setAlignment(Qt::AlignLeft);
        

        m_requirementsTitleLabel = new QLabel(tr("Project Requirements"), this);
        m_requirementsTitleLabel->setObjectName("remoteProjectDialogRequirementsTitleLabel");
        m_requirementsTitleLabel->setAlignment(Qt::AlignLeft);

        extraInfoGridLayout->addWidget(m_requirementsTitleLabel, 0, 0);

        m_licensesTitleLabel = new QLabel(tr("Licenses"), this);
        m_licensesTitleLabel->setObjectName("remoteProjectDialogLicensesTitleLabel");
        m_licensesTitleLabel->setAlignment(Qt::AlignLeft);
        extraInfoGridLayout->addWidget(m_licensesTitleLabel, 0, 1);

        extraInfoGridLayout->setVerticalSpacing(15);

        m_requirementsContentLabel = new TextOverflowLabel(tr("Requirements"));
        m_requirementsContentLabel->setObjectName("remoteProjectDialogRequirementsContentLabel");
        m_requirementsContentLabel->setWordWrap(true);
        m_requirementsContentLabel->setAlignment(Qt::AlignLeft);
        m_requirementsContentLabel->setFixedWidth(350);
        extraInfoGridLayout->addWidget(m_requirementsContentLabel, 1, 0);

        m_licensesContentLabel = new TextOverflowLabel(tr("Licenses"));
        m_licensesContentLabel->setObjectName("remoteProjectDialogLicensesContentLabel");
        m_licensesContentLabel->setWordWrap(true);
        m_licensesContentLabel->setAlignment(Qt::AlignLeft);
        m_licensesContentLabel->setFixedWidth(350);
        extraInfoGridLayout->addWidget(m_licensesContentLabel, 1, 1);

        vLayout->addLayout(extraInfoGridLayout);

        vLayout->addStretch();

        m_dialogButtons = new QDialogButtonBox();
        m_dialogButtons->setObjectName("footer");
        vLayout->addWidget(m_dialogButtons);

        QPushButton* cancelButton = m_dialogButtons->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
        cancelButton->setProperty("secondary", true);
        m_applyButton = m_dialogButtons->addButton(tr("Download && Build"), QDialogButtonBox::ApplyRole);
        m_applyButton->setProperty("primary", true);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_applyButton, &QPushButton::clicked, this, &AddRemoteProjectDialog::DownloadObject);

        m_inputTimer = new QTimer(this);
        m_inputTimer->setSingleShot(true);
        connect(m_inputTimer, &QTimer::timeout, this, &AddRemoteProjectDialog::ValidateURI);

        connect(
            m_autoBuild, &QCheckBox::clicked, [this](bool checked)
            {
                if (checked)
                {
                    m_applyButton->setText(tr("Download && Build"));
                }
                else
                {
                    m_applyButton->setText(tr("Download"));
                }
            }
        );

        connect(
            m_repoPath->lineEdit(), &QLineEdit::textEdited,
            [this]([[maybe_unused]] const QString& text)
            {
                // wait for a second before attempting to validate so we're less likely to do it per keypress
                m_inputTimer->start(1000);
                m_repoPath->SetValidationState(FormLineEditWidget::ValidationState::Validating);
            });

        SetDialogReady(false);
    }

    void AddRemoteProjectDialog::ValidateURI()
    {
        // validate URI, if it's a valid repository, get the project info and set the dialog as ready
        bool validRepository = PythonBindingsInterface::Get()->ValidateRepository(m_repoPath->lineEdit()->text());
        bool containsProjects = false;

        if (validRepository)
        {
            auto repoProjectsResult = PythonBindingsInterface::Get()->GetProjectsForRepo(m_repoPath->lineEdit()->text());
            if (repoProjectsResult.IsSuccess())
            {
                const auto repoProjects = repoProjectsResult.GetValue();
                if (!repoProjects.isEmpty())
                {
                    // only get the first one for now
                    const ProjectInfo& project = repoProjects.at(0);
                    SetCurrentProject(project);

                    containsProjects = true;
                }
            }
        }

        const bool isValidProjectRepo = validRepository && containsProjects;
        m_repoPath->SetValidationState(
            isValidProjectRepo ? FormLineEditWidget::ValidationState::ValidationSuccess
                               : FormLineEditWidget::ValidationState::ValidationFailed);
        m_repoPath->setErrorLabelVisible(!isValidProjectRepo);
        SetDialogReady(isValidProjectRepo);
    }

    void AddRemoteProjectDialog::DownloadObject()
    {
        // Add Repo:
        const QString repoUri = m_repoPath->lineEdit()->text();
        auto addGemRepoResult = PythonBindingsInterface::Get()->AddGemRepo(repoUri);
        if (addGemRepoResult.IsSuccess())
        {
            // Send download to project screen to initiate download
            emit StartObjectDownload(m_currentProject.m_projectName, GetInstallPath(), ShouldBuild());
            emit QDialog::accept();
        }
        else
        {
            QString failureMessage = tr("Failed to add gem repo: %1.").arg(repoUri);
            ProjectUtils::DisplayDetailedError(failureMessage, addGemRepoResult, this);
            AZ_Error("Project Manager", false, failureMessage.toUtf8().constData());
        }
    }

    QString AddRemoteProjectDialog::GetRepoPath()
    {
        return m_repoPath->lineEdit()->text();
    }
    QString AddRemoteProjectDialog::GetInstallPath()
    {
        return m_installPath->lineEdit()->text();
    }
    bool AddRemoteProjectDialog::ShouldBuild()
    {
        return m_autoBuild->isChecked();
    }

    void AddRemoteProjectDialog::SetCurrentProject(const ProjectInfo& projectInfo)
    {
        m_currentProject = projectInfo;

        m_downloadProjectLabel->setText(tr("Download Project %1").arg(projectInfo.m_displayName));
        m_installPath->lineEdit()->setText(QDir::toNativeSeparators(ProjectUtils::GetDefaultProjectPath() + "/" + projectInfo.m_projectName));
        m_requirementsContentLabel->setText(projectInfo.m_requirements);
        m_licensesContentLabel->setText(projectInfo.m_license);
    }

    void AddRemoteProjectDialog::SetDialogReady(bool isReady)
    {
        // Reset
        if (!isReady)
        {
            m_downloadProjectLabel->setText(tr("Download Project..."));
            m_installPath->setText("");
        }

        m_downloadProjectLabel->setEnabled(isReady);
        m_installPath->setEnabled(isReady);
        m_autoBuild->setEnabled(isReady);
        m_buildToggleLabel->setEnabled(isReady);
        m_requirementsTitleLabel->setEnabled(isReady);
        m_licensesTitleLabel->setEnabled(isReady);
        m_requirementsContentLabel->setEnabled(isReady);
        m_licensesContentLabel->setEnabled(isReady);
        m_applyButton->setEnabled(isReady);
    }
} // namespace O3DE::ProjectManager
