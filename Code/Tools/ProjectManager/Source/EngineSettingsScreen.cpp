/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EngineSettingsScreen.h>
#include <FormLineEditWidget.h>
#include <FormFolderBrowseEditWidget.h>
#include <PythonBindingsInterface.h>
#include <PathValidator.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>

namespace O3DE::ProjectManager
{
    EngineSettingsScreen::EngineSettingsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        auto* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);

        setObjectName("engineSettingsScreen");

        EngineInfo engineInfo;

        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            engineInfo = engineInfoResult.GetValue();
        }

        QLabel* formTitleLabel = new QLabel(tr("O3DE Settings"), this);
        formTitleLabel->setObjectName("formTitleLabel");
        layout->addWidget(formTitleLabel);

        m_engineVersion = new FormLineEditWidget(tr("Engine Version"), engineInfo.m_version, this);
        m_engineVersion->lineEdit()->setReadOnly(true);
        layout->addWidget(m_engineVersion);

        m_thirdParty = new FormFolderBrowseEditWidget(tr("3rd Party Software Folder"), engineInfo.m_thirdPartyPath, this);
        m_thirdParty->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
        m_thirdParty->lineEdit()->setReadOnly(true);
        m_thirdParty->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
        connect(m_thirdParty->lineEdit(), &QLineEdit::textChanged, this, &EngineSettingsScreen::OnTextChanged);
        layout->addWidget(m_thirdParty);

        m_defaultProjects = new FormFolderBrowseEditWidget(tr("Default Projects Folder"), engineInfo.m_defaultProjectsFolder, this);
        m_defaultProjects->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
        m_defaultProjects->lineEdit()->setReadOnly(true);
        m_defaultProjects->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
        connect(m_defaultProjects->lineEdit(), &QLineEdit::textChanged, this, &EngineSettingsScreen::OnTextChanged);
        layout->addWidget(m_defaultProjects);

        m_defaultGems = new FormFolderBrowseEditWidget(tr("Default Gems Folder"), engineInfo.m_defaultGemsFolder, this);
        m_defaultGems->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
        m_defaultGems->lineEdit()->setReadOnly(true);
        m_defaultGems->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
        connect(m_defaultGems->lineEdit(), &QLineEdit::textChanged, this, &EngineSettingsScreen::OnTextChanged);
        layout->addWidget(m_defaultGems);

        m_defaultProjectTemplates = new FormFolderBrowseEditWidget(tr("Default Project Templates Folder"), engineInfo.m_defaultTemplatesFolder, this);
        m_defaultProjectTemplates->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
        m_defaultProjectTemplates->lineEdit()->setReadOnly(true);
        m_defaultProjectTemplates->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
        connect(m_defaultProjectTemplates->lineEdit(), &QLineEdit::textChanged, this, &EngineSettingsScreen::OnTextChanged);
        layout->addWidget(m_defaultProjectTemplates);

        setLayout(layout);
    }

    ProjectManagerScreen EngineSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::EngineSettings;
    }

    void EngineSettingsScreen::OnTextChanged()
    {
        // save engine settings
        auto engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            EngineInfo engineInfo;
            engineInfo = engineInfoResult.GetValue();
            engineInfo.m_thirdPartyPath         = m_thirdParty->lineEdit()->text();
            engineInfo.m_defaultProjectsFolder  = m_defaultProjects->lineEdit()->text();
            engineInfo.m_defaultGemsFolder      = m_defaultGems->lineEdit()->text();
            engineInfo.m_defaultTemplatesFolder = m_defaultProjectTemplates->lineEdit()->text();

            bool result = PythonBindingsInterface::Get()->SetEngineInfo(engineInfo);
            if (!result)
            {
                QMessageBox::critical(this, tr("Engine Settings"), tr("Failed to save engine settings."));
            }
        }
        else
        {
            QMessageBox::critical(this, tr("Engine Settings"), tr("Failed to get engine settings."));
        }
    }
} // namespace O3DE::ProjectManager
