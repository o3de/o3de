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

#include <NewProjectSettingsScreen.h>
#include <PythonBindingsInterface.h>
#include <FormLineEditWidget.h>
#include <FormBrowseEditWidget.h>
#include <PathValidator.h>
#include <EngineInfo.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QSpacerItem>
#include <QStandardPaths>
#include <QFrame>

namespace O3DE::ProjectManager
{
    constexpr const char* k_pathProperty = "Path";

    NewProjectSettingsScreen::NewProjectSettingsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout(this);
        hLayout->setAlignment(Qt::AlignLeft);
        hLayout->setContentsMargins(0,0,0,0);

        // if we don't provide a parent for this box layout the stylesheet doesn't take
        // if we don't set this in a frame (just use a sub-layout) all the content will align incorrectly horizontally
        QFrame* projectSettingsFrame = new QFrame(this);
        projectSettingsFrame->setObjectName("projectSettings");
        QVBoxLayout* vLayout = new QVBoxLayout(this);

        // you cannot remove content margins in qss
        vLayout->setContentsMargins(0,0,0,0);
        vLayout->setAlignment(Qt::AlignTop);
        {
            const QString defaultName{ "NewProject" };
            const QString defaultPath = QDir::toNativeSeparators(GetDefaultProjectPath() + "/" + defaultName);

            m_projectName = new FormLineEditWidget(tr("Project name"), defaultName, this);
            connect(m_projectName->lineEdit(), &QLineEdit::textChanged, this, &NewProjectSettingsScreen::ValidateProjectPath);
            vLayout->addWidget(m_projectName);

            m_projectPath = new FormBrowseEditWidget(tr("Project Location"), defaultPath, this);
            m_projectPath->lineEdit()->setReadOnly(true);
            connect(m_projectPath->lineEdit(), &QLineEdit::textChanged, this, &NewProjectSettingsScreen::ValidateProjectPath);
            vLayout->addWidget(m_projectPath);

            // if we don't use a QFrame we cannot "contain" the widgets inside and move them around
            // as a group
            QFrame* projectTemplateWidget = new QFrame(this);
            projectTemplateWidget->setObjectName("projectTemplate");
            QVBoxLayout* containerLayout = new QVBoxLayout();
            containerLayout->setAlignment(Qt::AlignTop);
            {
                QLabel* projectTemplateLabel = new QLabel(tr("Select a Project Template"));
                projectTemplateLabel->setObjectName("projectTemplateLabel");
                containerLayout->addWidget(projectTemplateLabel);

                QLabel* projectTemplateDetailsLabel = new QLabel(tr("Project templates are pre-configured with relevant Gems that provide "
                                                                    "additional functionality and content to the project."));
                projectTemplateDetailsLabel->setWordWrap(true);
                projectTemplateDetailsLabel->setObjectName("projectTemplateDetailsLabel");
                containerLayout->addWidget(projectTemplateDetailsLabel);

                QHBoxLayout* templateLayout = new QHBoxLayout(this);
                containerLayout->addItem(templateLayout);

                m_projectTemplateButtonGroup = new QButtonGroup(this);
                m_projectTemplateButtonGroup->setObjectName("templateButtonGroup");
                auto templatesResult = PythonBindingsInterface::Get()->GetProjectTemplates();
                if (templatesResult.IsSuccess() && !templatesResult.GetValue().isEmpty())
                {
                    for (auto projectTemplate : templatesResult.GetValue())
                    {
                        QRadioButton* radioButton = new QRadioButton(projectTemplate.m_name, this);
                        radioButton->setProperty(k_pathProperty, projectTemplate.m_path);
                        m_projectTemplateButtonGroup->addButton(radioButton);

                        containerLayout->addWidget(radioButton);
                    }

                    m_projectTemplateButtonGroup->buttons().first()->setChecked(true);
                }
            }
            projectTemplateWidget->setLayout(containerLayout);
            vLayout->addWidget(projectTemplateWidget);
        }
        projectSettingsFrame->setLayout(vLayout);

        hLayout->addWidget(projectSettingsFrame);

        QWidget* projectTemplateDetails = new QWidget(this);
        projectTemplateDetails->setObjectName("projectTemplateDetails");
        hLayout->addWidget(projectTemplateDetails);

        this->setLayout(hLayout);
    }

    QString NewProjectSettingsScreen::GetDefaultProjectPath()
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

    ProjectManagerScreen NewProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::NewProjectSettings;
    }

    void NewProjectSettingsScreen::ValidateProjectPath()
    {
        Validate();    
    }

    void NewProjectSettingsScreen::NotifyCurrentScreen()
    {
        Validate();
    }

    ProjectInfo NewProjectSettingsScreen::GetProjectInfo()
    {
        ProjectInfo projectInfo;
        projectInfo.m_projectName = m_projectName->lineEdit()->text();
        projectInfo.m_path = m_projectPath->lineEdit()->text();
        return projectInfo;
    }

    QString NewProjectSettingsScreen::GetProjectTemplatePath()
    {
        return m_projectTemplateButtonGroup->checkedButton()->property(k_pathProperty).toString();
    }

    bool NewProjectSettingsScreen::Validate()
    {
        bool projectPathIsValid = true;
        if (m_projectPath->lineEdit()->text().isEmpty())
        {
            projectPathIsValid = false;
            m_projectPath->setErrorLabelText(tr("Please provide a valid path."));
        }

        bool projectNameIsValid = true;
        if (m_projectName->lineEdit()->text().isEmpty())
        {
            projectNameIsValid = false;
            m_projectName->setErrorLabelText(tr("Please provide a project name."));
        }
        else
        {
            QDir path(m_projectPath->lineEdit()->text());

            // path validation with multiple locals and platforms is ... difficult. Prevent the usual suspects.
            QRegExp illegalCharacterRegex("[^A-Za-z0-9_]");
            const int result = illegalCharacterRegex.indexIn(m_projectName->lineEdit()->text());
            if (result > -1)
            {
                projectNameIsValid = false;
                m_projectName->setErrorLabelText(tr("Project names must only contain alphanumeric characters or the '_' character"));
            }
            else if (path.exists() && !path.isEmpty())
            {
                projectPathIsValid = false;
                m_projectPath->setErrorLabelText(tr("This folder exists and isn't empty.  Please choose a different name or location."));
            }
        }

        m_projectName->setErrorLabelVisible(!projectNameIsValid);
        m_projectPath->setErrorLabelVisible(!projectPathIsValid);
        return projectNameIsValid && projectPathIsValid;
    }
} // namespace O3DE::ProjectManager
