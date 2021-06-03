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
        : ProjectSettingsScreen(parent)
    {
        const QString defaultName{ "NewProject" };
        const QString defaultPath = QDir::toNativeSeparators(GetDefaultProjectPath() + "/" + defaultName);

        m_projectName->lineEdit()->setText(defaultName);
        m_projectPath->lineEdit()->setText(defaultPath);

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
                for (const ProjectTemplateInfo& projectTemplate : templatesResult.GetValue())
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
        m_verticalLayout->addWidget(projectTemplateWidget);

        QWidget* projectTemplateDetails = new QWidget(this);
        projectTemplateDetails->setObjectName("projectTemplateDetails");
        m_horizontalLayout->addWidget(projectTemplateDetails);
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

    void NewProjectSettingsScreen::NotifyCurrentScreen()
    {
        Validate();
    }

    QString NewProjectSettingsScreen::GetProjectTemplatePath()
    {
        return m_projectTemplateButtonGroup->checkedButton()->property(k_pathProperty).toString();
    }
} // namespace O3DE::ProjectManager
