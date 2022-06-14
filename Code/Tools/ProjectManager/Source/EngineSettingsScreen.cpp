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
#include <ProjectUtils.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>

namespace O3DE::ProjectManager
{
    EngineSettingsScreen::EngineSettingsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);

        QWidget* scrollWidget = new QWidget(this);
        scrollArea->setWidget(scrollWidget);

        QVBoxLayout* layout = new QVBoxLayout(scrollWidget);
        layout->setAlignment(Qt::AlignTop);
        scrollWidget->setLayout(layout);

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

        FormLineEditWidget* engineName = new FormLineEditWidget(tr("Engine Name"), engineInfo.m_name, this);
        engineName->lineEdit()->setReadOnly(true);
        layout->addWidget(engineName);

        FormLineEditWidget* engineVersion = new FormLineEditWidget(tr("Engine Version"), engineInfo.m_version, this);
        engineVersion->lineEdit()->setReadOnly(true);
        layout->addWidget(engineVersion);

        FormBrowseEditWidget* engineFolder = new FormBrowseEditWidget(tr("Engine Folder"), engineInfo.m_path, this);
        engineFolder->lineEdit()->setReadOnly(true);
        connect( engineFolder, &FormBrowseEditWidget::OnBrowse, [engineInfo]{ AzQtComponents::ShowFileOnDesktop(engineInfo.m_path); });
        layout->addWidget(engineFolder);

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

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setMargin(0);
        mainLayout->addWidget(scrollArea);
        setLayout(mainLayout);
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

            auto result = PythonBindingsInterface::Get()->SetEngineInfo(engineInfo);
            if (!result)
            {
                ProjectUtils::DisplayDetailedError(tr("Failed to save engine settings"), result, this);
            }
        }
        else
        {
            QMessageBox::critical(this, tr("Engine Settings"), tr("Failed to get engine settings."));
        }
    }
} // namespace O3DE::ProjectManager
