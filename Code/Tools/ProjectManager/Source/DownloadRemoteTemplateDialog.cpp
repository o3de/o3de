/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DownloadRemoteTemplateDialog.h>
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
    DownloadRemoteTemplateDialog::DownloadRemoteTemplateDialog(const ProjectTemplateInfo& projectTemplate, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Download a remote template"));
        setModal(true);
        setObjectName("downloadRemoteTempalteDialog");
        setFixedSize(QSize(760, 390));

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(30, 30, 25, 10);
        vLayout->setSpacing(0);
        vLayout->setAlignment(Qt::AlignTop);
        setLayout(vLayout);

        QLabel* warningLabel = new QLabel(tr("\"%1\" needs to be downloaded from the repository first, before using it as your template.").arg(projectTemplate.m_displayName), this);
        warningLabel->setObjectName("remoteTemplateDialogDownloadTemplateLabel");
        warningLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(warningLabel);

        vLayout->addSpacing(20);

        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setObjectName("horizontalSeparatingLine");
        vLayout->addWidget(hLine);

        vLayout->addSpacing(20);

        m_downloadTemplateLabel = new QLabel(tr("Choose the location for the template"), this);
        m_downloadTemplateLabel->setObjectName("remoteTemplateDialogDownloadTemplateLabel");
        m_downloadTemplateLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(m_downloadTemplateLabel);

        m_installPath = new FormFolderBrowseEditWidget(tr("Local template directory"));
        m_installPath->setMinimumSize(QSize(600, 0));
        m_installPath->lineEdit()->setText(
            QDir::toNativeSeparators(ProjectUtils::GetDefaultTemplatePath() + "/" + projectTemplate.m_name));
        vLayout->addWidget(m_installPath);

        vLayout->addSpacing(20);

        QGridLayout* extraInfoGridLayout = new QGridLayout(this);
        extraInfoGridLayout->setContentsMargins(0, 0, 0, 0);
        extraInfoGridLayout->setHorizontalSpacing(5);
        extraInfoGridLayout->setVerticalSpacing(15);
        extraInfoGridLayout->setAlignment(Qt::AlignLeft);

        m_requirementsTitleLabel = new QLabel(tr("Template Requirements"), this);
        m_requirementsTitleLabel->setObjectName("remoteTemplateDialogRequirementsTitleLabel");
        m_requirementsTitleLabel->setAlignment(Qt::AlignLeft);

        extraInfoGridLayout->addWidget(m_requirementsTitleLabel, 0, 0);

        m_licensesTitleLabel = new QLabel(tr("Licenses"), this);
        m_licensesTitleLabel->setObjectName("remoteTemplateDialogLicensesTitleLabel");
        m_licensesTitleLabel->setAlignment(Qt::AlignLeft);
        extraInfoGridLayout->addWidget(m_licensesTitleLabel, 0, 1);

        extraInfoGridLayout->setVerticalSpacing(15);

        m_requirementsContentLabel = new TextOverflowLabel(tr("Requirements"), projectTemplate.m_requirements);
        m_requirementsContentLabel->setObjectName("remoteTemplateDialogRequirementsContentLabel");
        m_requirementsContentLabel->setWordWrap(true);
        m_requirementsContentLabel->setAlignment(Qt::AlignLeft);
        m_requirementsContentLabel->setFixedWidth(350);
        extraInfoGridLayout->addWidget(m_requirementsContentLabel, 1, 0);

        m_licensesContentLabel = new TextOverflowLabel(tr("Licenses"), projectTemplate.m_license);
        m_licensesContentLabel->setObjectName("remoteTemplateDialogLicensesContentLabel");
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
        m_applyButton = m_dialogButtons->addButton(tr("Download"), QDialogButtonBox::ApplyRole);
        m_applyButton->setProperty("primary", true);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_applyButton, &QPushButton::clicked, this, &QDialog::accept);
    }

    QString DownloadRemoteTemplateDialog::GetInstallPath()
    {
        return m_installPath->lineEdit()->text();
    }
} // namespace O3DE::ProjectManager
