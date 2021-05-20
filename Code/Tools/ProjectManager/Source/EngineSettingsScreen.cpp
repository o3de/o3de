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

#include <EngineSettingsScreen.h>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include "FormLineEditWidget.h"
#include "FormBrowseEditWidget.h"
#include "PythonBindingsInterface.h"
#include "PathValidator.h"

namespace O3DE::ProjectManager
{
    EngineSettingsScreen::EngineSettingsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);

        setObjectName("engineSettingsScreen");

        EngineInfo engineInfo;

        auto engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            engineInfo = engineInfoResult.GetValue();
        }

        QLabel* formTitleLabel = new QLabel(tr("O3DE Settings"), this);
        formTitleLabel->setObjectName("formTitleLabel");
        layout->addWidget(formTitleLabel);

        m_engineVersionLineEdit = new FormLineEditWidget(tr("Engine Version"), engineInfo.m_version, this);
        m_engineVersionLineEdit->lineEdit()->setReadOnly(true);
        layout->addWidget(m_engineVersionLineEdit);

        m_thirdPartyPathLineEdit = new FormBrowseEditWidget(tr("3rd Party Software Folder"), engineInfo.m_thirdPartyPath, this);
        m_thirdPartyPathLineEdit->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
        m_thirdPartyPathLineEdit->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
        layout->addWidget(m_thirdPartyPathLineEdit);

        m_restrictedPathLineEdit = new FormBrowseEditWidget(tr("Restricted Folder"), engineInfo.m_defaultRestrictedFolder, this);
        m_restrictedPathLineEdit->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
        m_restrictedPathLineEdit->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
        layout->addWidget(m_restrictedPathLineEdit);

        m_defaultGemsPathLineEdit = new FormBrowseEditWidget(tr("Default Gems Folder"), engineInfo.m_defaultGemsFolder, this);
        m_defaultGemsPathLineEdit->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
        m_defaultGemsPathLineEdit->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
        layout->addWidget(m_defaultGemsPathLineEdit);

        m_defaultProjectTemplatesPathLineEdit = new FormBrowseEditWidget(tr("Default Project Templates Folder"), engineInfo.m_defaultTemplatesFolder, this);
        m_defaultProjectTemplatesPathLineEdit->lineEdit()->setValidator(new PathValidator(PathValidator::PathMode::ExistingFolder, this));
        m_defaultProjectTemplatesPathLineEdit->setErrorLabelText(tr("Please provide a valid path to a folder that exists"));
        layout->addWidget(m_defaultProjectTemplatesPathLineEdit);

        setLayout(layout);
    }

    ProjectManagerScreen EngineSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::EngineSettings;
    }
} // namespace O3DE::ProjectManager
