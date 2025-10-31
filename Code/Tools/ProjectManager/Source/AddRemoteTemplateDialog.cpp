/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AddRemoteTemplateDialog.h>
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
    AddRemoteTemplateDialog::AddRemoteTemplateDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Add a remote template"));
        setModal(true);
        setObjectName("addRemoteTemplateDialog");
        setFixedSize(QSize(760, 270));

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(30, 30, 25, 10);
        vLayout->setSpacing(0);
        vLayout->setAlignment(Qt::AlignTop);
        setLayout(vLayout);

        QLabel* instructionTitleLabel = new QLabel(tr("Please enter a remote URL for your template"), this);
        instructionTitleLabel->setObjectName("remoteTemplateDialogInstructionTitleLabel");
        instructionTitleLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(instructionTitleLabel);

        vLayout->addSpacing(10);

        m_repoPath = new FormLineEditWidget(tr("Remote URL"), "", this);
        m_repoPath->setMinimumSize(QSize(600, 0));
        m_repoPath->setErrorLabelText(tr("Not a valid remote template source."));
        m_repoPath->lineEdit()->setPlaceholderText("https://github.com/o3de/example.git");
        vLayout->addWidget(m_repoPath);

        vLayout->addSpacing(10);

        QLabel* warningLabel = new QLabel(tr("Online repositories may contain files that could potentially harm your computer,"
            " please ensure you understand the risks before downloading from third-party sources."), this);
        warningLabel->setObjectName("remoteProjectDialogWarningLabel");
        warningLabel->setWordWrap(true);
        warningLabel->setAlignment(Qt::AlignLeft);
        vLayout->addWidget(warningLabel);

        vLayout->addSpacing(20);

        vLayout->addStretch();

        m_dialogButtons = new QDialogButtonBox();
        m_dialogButtons->setObjectName("footer");
        vLayout->addWidget(m_dialogButtons);

        QPushButton* cancelButton = m_dialogButtons->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
        cancelButton->setProperty("secondary", true);
        m_applyButton = m_dialogButtons->addButton(tr("Add"), QDialogButtonBox::ApplyRole);
        m_applyButton->setProperty("primary", true);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_applyButton, &QPushButton::clicked, this, &AddRemoteTemplateDialog::AddTemplateSource);

        m_inputTimer = new QTimer(this);
        m_inputTimer->setSingleShot(true);
        connect(m_inputTimer, &QTimer::timeout, this, &AddRemoteTemplateDialog::ValidateURI);

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

    void AddRemoteTemplateDialog::ValidateURI()
    {
        // validate URI, if it's a valid repository set the add button as active
        bool validRepository = PythonBindingsInterface::Get()->ValidateRepository(m_repoPath->lineEdit()->text());
        SetDialogReady(validRepository);
        m_repoPath->SetValidationState(
            validRepository ? FormLineEditWidget::ValidationState::ValidationSuccess
                            : FormLineEditWidget::ValidationState::ValidationFailed);
        m_repoPath->setErrorLabelVisible(!validRepository);
    }

    void AddRemoteTemplateDialog::AddTemplateSource()
    {
        // Add the remote source
        const QString repoUri = m_repoPath->lineEdit()->text();
        auto addGemRepoResult = PythonBindingsInterface::Get()->AddGemRepo(repoUri);
        if (addGemRepoResult.IsSuccess())
        {
            emit QDialog::accept();
        }
        else
        {
            QString failureMessage = tr("Failed to add template source: %1.").arg(repoUri);
            ProjectUtils::DisplayDetailedError(failureMessage, addGemRepoResult, this);
            AZ_Error("Project Manager", false, failureMessage.toUtf8().constData());
        }
    }

    QString AddRemoteTemplateDialog::GetRepoPath()
    {
        return m_repoPath->lineEdit()->text();
    }

    void AddRemoteTemplateDialog::SetDialogReady(bool isReady)
    {
        // Reset
        m_applyButton->setEnabled(isReady);
    }
} // namespace O3DE::ProjectManager
