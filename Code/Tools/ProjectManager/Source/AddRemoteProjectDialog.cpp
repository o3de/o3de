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
#include <ToggleCheckbox.h>
#include <ProjectUtils.h>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDir>

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
        vLayout->addWidget(m_repoPath);

        vLayout->addSpacing(10);

        QLabel* warningLabel = new QLabel(tr("Online repositories may contain files that could potentially harm your computer,"
            " please ensure you understand the risks before downloading from third-party sources."), this);
        warningLabel->setObjectName("remoteProjectDialogWarningLabel");
        warningLabel->setWordWrap(true);
        warningLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(warningLabel);

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

        m_installPath = new FormFolderBrowseEditWidget(tr("Local project directory"), "C:/Projects/Test", this);
        m_installPath->setMinimumSize(QSize(600, 0));
        vLayout->addWidget(m_installPath);

        vLayout->addSpacing(10);

        QHBoxLayout* buildHLayout = new QHBoxLayout(this);
        buildHLayout->setContentsMargins(0, 0, 0, 0);
        buildHLayout->setAlignment(Qt::AlignLeft);

        m_autoBuild = new ToggleCheckbox(this);
        m_autoBuild->setChecked(true);
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
        

        m_requirementsTitleLabel = new QLabel(tr("Requirements"), this);
        m_requirementsTitleLabel->setObjectName("remoteProjectDialogRequirementsTitleLabel");
        m_requirementsTitleLabel->setAlignment(Qt::AlignLeft);

        extraInfoGridLayout->addWidget(m_requirementsTitleLabel, 0, 0);

        m_licensesTitleLabel = new QLabel(tr("Licenses"), this);
        m_licensesTitleLabel->setObjectName("remoteProjectDialogLicensesTitleLabel");
        m_licensesTitleLabel->setAlignment(Qt::AlignLeft);
        extraInfoGridLayout->addWidget(m_licensesTitleLabel, 0, 1);

        extraInfoGridLayout->setVerticalSpacing(15);

        m_requirementsContentLabel = new TextOverflowLabel(tr("Requirements"), "Users will need to download Wwise from the <a href='https://www.audiokinetic.com/download/'>Audiokinetic Web Site</a>. Lorem ipsum dolor sit amet, consectetur adipiscing elit. In gravida et ipsum quis fermentum. Sed faucibus lectus a sodales vestibulum. Maecenas porttitor felis eu lorem ullamcorper, id commodo metus dictum. Quisque a odio at nisl ullamcorper hendrerit quis dignissim nulla. Vivamus pharetra hendrerit dolor vitae placerat. Cras eros libero, bibendum ac lorem id, sollicitudin egestas mi. Cras varius metus quis dolor dictum tempus. In rhoncus aliquet sapien ac egestas. Fusce at ullamcorper massa. Nullam fringilla velit vitae libero lobortis, vel viverra ligula tempus. Morbi euismod tellus at ante euismod, nec dignissim felis rutrum. Ut blandit, mi id sagittis vestibulum, felis mi porttitor quam, a consectetur orci tortor a nisi. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Vivamus eget metus orci. In non risus elementum, accumsan est et, ullamcorper neque. In tellus velit, maximus ac lorem ut, fermentum vestibulum justo. Pellentesque fringilla elit et ex vulputate tristique. Aliquam ex erat, porta at rhoncus vel, placerat sed nisi. Vivamus volutpat mollis mauris, quis tempor nunc finibus id. Quisque accumsan leo nec elit placerat lacinia. Morbi eget ipsum ac mauris fringilla pellentesque sed non dui. Vivamus et faucibus lacus. Phasellus tincidunt ac nibh eget mollis. Integer ullamcorper, nibh vel posuere rutrum, arcu nibh mollis tortor, a blandit ante metus et erat. Vestibulum vel varius arcu. Nam facilisis nec lorem luctus ullamcorper. Nulla a tortor venenatis, dictum purus a, mattis ipsum. Proin vel rutrum dui, bibendum fringilla sem. Integer nunc turpis, lacinia ut purus in, vulputate fringilla dui. Phasellus egestas tellus a ante vulputate, et tempor ante posuere. Quisque vel ipsum congue, feugiat nunc at, imperdiet orci. Sed ac eleifend elit. Etiam facilisis odio nisl, nec mollis velit ultrices a. Suspendisse rhoncus eros vitae ante pharetra, eu malesuada justo egestas. Suspendisse libero dolor, vestibulum sed aliquet a, consequat vitae libero. Sed ornare, quam id volutpat efficitur, lorem ante dictum ipsum, viverra egestas ipsum enim vel nulla. In hac habitasse platea dictumst. Aliquam blandit lacus tortor, a efficitur nisi dictum nec. Aliquam hendrerit tempor lorem in blandit. Suspendisse potenti. Duis risus velit, rutrum quis mi sed, iaculis condimentum dui. Morbi luctus tincidunt erat, vitae condimentum risus semper ac. Integer vitae quam pretium, suscipit quam eu, lacinia diam. Pellentesque diam justo, commodo convallis finibus vel, dictum ut turpis. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Morbi sed porta ligula, a interdum leo. Proin quis aliquam enim. Aliquam malesuada condimentum consequat. Nam finibus rutrum metus. Praesent cursus eget felis eu maximus. Donec semper iaculis dolor sit amet venenatis. Maecenas cursus posuere turpis, sit amet sagittis ante efficitur nec. Suspendisse in leo et enim blandit efficitur a sit amet neque. Mauris diam metus, venenatis id laoreet vitae, suscipit sit amet orci. Vestibulum sodales arcu diam, sit amet volutpat nibh luctus quis. In gravida et ipsum quis fermentum. Sed faucibus lectus a sodales vestibulum. Maecenas porttitor felis eu lorem ullamcorper, id commodo metus dictum. Quisque a odio at nisl ullamcorper hendrerit quis dignissim nulla. Vivamus pharetra hendrerit dolor vitae placerat. Cras eros libero, bibendum ac lorem id, sollicitudin egestas mi. Cras varius metus quis dolor dictum tempus. In rhoncus aliquet sapien ac egestas. Fusce at ullamcorper massa. Nullam fringilla velit vitae libero lobortis, vel viverra ligula tempus. Morbi euismod tellus at ante euismod, nec dignissim felis rutrum. In gravida et ipsum quis fermentum. Sed faucibus lectus a sodales vestibulum. Maecenas porttitor felis eu lorem ullamcorper, id commodo metus dictum. Quisque a odio at nisl ullamcorper hendrerit quis dignissim nulla. Vivamus pharetra hendrerit dolor vitae placerat. Cras eros libero, bibendum ac lorem id, sollicitudin egestas mi. Cras varius metus quis dolor dictum tempus. In rhoncus aliquet sapien ac egestas. Fusce at ullamcorper massa. Nullam fringilla velit vitae libero lobortis, vel viverra ligula tempus. Morbi euismod tellus at ante euismod, nec dignissim felis rutrum. In gravida et ipsum quis fermentum. Sed faucibus lectus a sodales vestibulum. Maecenas porttitor felis eu lorem ullamcorper, id commodo metus dictum. Quisque a odio at nisl ullamcorper hendrerit quis dignissim nulla. Vivamus pharetra hendrerit dolor vitae placerat. Cras eros libero, bibendum ac lorem id, sollicitudin egestas mi. Cras varius metus quis dolor dictum tempus. In rhoncus aliquet sapien ac egestas. Fusce at ullamcorper massa. Nullam fringilla velit vitae libero lobortis, vel viverra ligula tempus. Morbi euismod tellus at ante euismod, nec dignissim felis rutrum.", this);
        m_requirementsContentLabel->setObjectName("remoteProjectDialogRequirementsContentLabel");
        m_requirementsContentLabel->setWordWrap(true);
        m_requirementsContentLabel->setAlignment(Qt::AlignLeft);
        m_requirementsContentLabel->setFixedWidth(350);
        extraInfoGridLayout->addWidget(m_requirementsContentLabel, 1, 0);

        m_licensesContentLabel = new TextOverflowLabel(tr("Licenses"), "<a href='https://github.com/o3de/o3de/blob/development/LICENSE.txt'>Apache-2.0 Or MIT</a>", this);
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

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_applyButton, &QPushButton::clicked, this, &QDialog::accept);

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

        // Simulate repo being entered and UI enabling
        connect(
            m_repoPath->lineEdit(), &QLineEdit::textEdited,
            [this](const QString& text)
            {
                SetDialogReady(!text.isEmpty());
            });

        SetDialogReady(false);
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

        m_downloadProjectLabel->setText(projectInfo.m_displayName);
        m_installPath->lineEdit()->setText(QDir::toNativeSeparators(ProjectUtils::GetDefaultProjectPath() + "/" + projectInfo.m_projectName));
    }

    void AddRemoteProjectDialog::SetDialogReady(bool isReady)
    {
        m_downloadProjectLabel->setEnabled(isReady);
        m_installPath->setEnabled(isReady);
        m_autoBuild->setEnabled(isReady);
        m_buildToggleLabel->setEnabled(isReady);
        m_requirementsTitleLabel->setEnabled(isReady);
        m_licensesTitleLabel->setEnabled(isReady);
        m_requirementsContentLabel->setEnabled(isReady);
        m_licensesContentLabel->setEnabled(isReady);
        m_dialogButtons->setEnabled(isReady);
    }
} // namespace O3DE::ProjectManager
