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
#include <TemplateButtonWidget.h>
#include <PathValidator.h>
#include <EngineInfo.h>
#include <TagWidget.h>
#include <AzQtComponents/Components/FlowLayout.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QToolButton>
#include <QSpacerItem>
#include <QStandardPaths>
#include <QFrame>
#include <QScrollArea>
#include <QAbstractButton>

namespace O3DE::ProjectManager
{
    constexpr const char* k_templateIndexProperty = "TemplateIndex";

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

                // we might have enough templates that we need to scroll
                QScrollArea* templatesScrollArea = new QScrollArea(this);
                QWidget* scrollWidget = new QWidget();

                FlowLayout* flowLayout = new FlowLayout(0, s_spacerSize, s_spacerSize);
                scrollWidget->setLayout(flowLayout);

                templatesScrollArea->setWidget(scrollWidget);
                templatesScrollArea->setWidgetResizable(true);

                m_projectTemplateButtonGroup = new QButtonGroup(this);
                m_projectTemplateButtonGroup->setObjectName("templateButtonGroup");

                // QButtonGroup has overloaded buttonClicked methods so we need the QOverload
                connect(
                    m_projectTemplateButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this,
                    [=](QAbstractButton* button)
                    {
                        if (button && button->property(k_templateIndexProperty).isValid())
                        {
                            int projectIndex = button->property(k_templateIndexProperty).toInt();
                            UpdateTemplateSummary(m_templates.at(projectIndex));
                        }
                    });

                auto templatesResult = PythonBindingsInterface::Get()->GetProjectTemplates();
                if (templatesResult.IsSuccess() && !templatesResult.GetValue().isEmpty())
                {
                    m_templates = templatesResult.GetValue();

                    // sort alphabetically by display name because they could be in any order
                    std::sort(
                        m_templates.begin(), m_templates.end(),
                        [](const ProjectTemplateInfo& arg1, const ProjectTemplateInfo& arg2)
                        {
                            return arg1.m_displayName.toLower() < arg2.m_displayName.toLower();
                        });

                    for (int index = 0; index < m_templates.size(); ++index)
                    {
                        ProjectTemplateInfo projectTemplate = m_templates.at(index);
                        QString projectPreviewPath = projectTemplate.m_path + "/Template/preview.png";
                        QFileInfo doesPreviewExist(projectPreviewPath);
                        if (!doesPreviewExist.exists() || !doesPreviewExist.isFile())
                        {
                            projectPreviewPath = ":/DefaultTemplate.png";
                        }
                        TemplateButton* templateButton = new TemplateButton(projectPreviewPath, projectTemplate.m_displayName, this);
                        templateButton->setCheckable(true);
                        templateButton->setProperty(k_templateIndexProperty, index);
                        
                        m_projectTemplateButtonGroup->addButton(templateButton);

                        flowLayout->addWidget(templateButton);
                    }

                    m_projectTemplateButtonGroup->buttons().first()->setChecked(true);
                }
                containerLayout->addWidget(templatesScrollArea);
            }

            projectTemplateWidget->setLayout(containerLayout);
            vLayout->addWidget(projectTemplateWidget);
        }
        projectSettingsFrame->setLayout(vLayout);

        hLayout->addWidget(projectSettingsFrame);

        QFrame* projectTemplateDetails = new QFrame(this);
        projectTemplateDetails->setObjectName("projectTemplateDetails");
        QVBoxLayout* templateDetailsLayout = new QVBoxLayout();
        templateDetailsLayout->setContentsMargins(s_templateDetailsContentMargin, s_templateDetailsContentMargin, s_templateDetailsContentMargin, s_templateDetailsContentMargin);
        templateDetailsLayout->setAlignment(Qt::AlignTop);
        {
            m_templateDisplayName = new QLabel(this);
            m_templateDisplayName->setObjectName("displayName");
            templateDetailsLayout->addWidget(m_templateDisplayName);

            m_templateSummary = new QLabel(this);
            m_templateSummary->setObjectName("summary");
            m_templateSummary->setWordWrap(true);
            templateDetailsLayout->addWidget(m_templateSummary);

            QLabel* includedGemsTitle = new QLabel(tr("Included Gems"), this);
            includedGemsTitle->setObjectName("includedGemsTitle");
            templateDetailsLayout->addWidget(includedGemsTitle);

            m_templateIncludedGems = new TagContainerWidget();
            m_templateIncludedGems->setObjectName("includedGems");
            templateDetailsLayout->addWidget(m_templateIncludedGems);

            QLabel* moreGemsLabel = new QLabel(tr("Looking for more Gems?"), this);
            moreGemsLabel->setObjectName("moreGems");
            templateDetailsLayout->addWidget(moreGemsLabel);

            QLabel* browseCatalogLabel = new QLabel(tr("Browse the  Gems Catalog to further customize your project."), this);
            browseCatalogLabel->setObjectName("browseCatalog");
            browseCatalogLabel->setWordWrap(true);
            templateDetailsLayout->addWidget(browseCatalogLabel);

            QPushButton* configureGemsButton = new QPushButton(tr("Configure with more Gems"), this);
            connect(configureGemsButton, &QPushButton::clicked, this, [=]()
                    {
                        emit ChangeScreenRequest(ProjectManagerScreen::GemCatalog);
                    });
            templateDetailsLayout->addWidget(configureGemsButton);

            templateDetailsLayout->addWidget(m_templateIncludedGems);
        }
        projectTemplateDetails->setLayout(templateDetailsLayout);
        hLayout->addWidget(projectTemplateDetails);

        if (!m_templates.isEmpty())
        {
            UpdateTemplateSummary(m_templates.first());
        }

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
        const int templateIndex = m_projectTemplateButtonGroup->checkedButton()->property(k_templateIndexProperty).toInt();
        return m_templates.at(templateIndex).m_path;
    }

    bool NewProjectSettingsScreen::Validate()
    {
        bool projectPathIsValid = true;
        if (m_projectPath->lineEdit()->text().isEmpty())
        {
            projectPathIsValid = false;
            m_projectPath->setErrorLabelText(tr("Please provide a valid location."));
        }
        else
        {
            QDir path(m_projectPath->lineEdit()->text());
            if (path.exists() && !path.isEmpty())
            {
                projectPathIsValid = false;
                m_projectPath->setErrorLabelText(tr("This folder exists and isn't empty.  Please choose a different location."));
            }
        }

        bool projectNameIsValid = true;
        if (m_projectName->lineEdit()->text().isEmpty())
        {
            projectNameIsValid = false;
            m_projectName->setErrorLabelText(tr("Please provide a project name."));
        }
        else
        {
            // this validation should roughly match the utils.validate_identifier which the cli 
            // uses to validate project names
            QRegExp validProjectNameRegex("[A-Za-z][A-Za-z0-9_-]{0,63}");
            const bool result = validProjectNameRegex.exactMatch(m_projectName->lineEdit()->text());
            if (!result)
            {
                projectNameIsValid = false;
                m_projectName->setErrorLabelText(tr("Project names must start with a letter and consist of up to 64 letter, number, '_' or '-' characters"));
            }

        }

        m_projectName->setErrorLabelVisible(!projectNameIsValid);
        m_projectPath->setErrorLabelVisible(!projectPathIsValid);
        return projectNameIsValid && projectPathIsValid;
    }

    void NewProjectSettingsScreen::UpdateTemplateSummary(const ProjectTemplateInfo& templateInfo)
    {
        m_templateDisplayName->setText(templateInfo.m_displayName);
        m_templateSummary->setText(templateInfo.m_summary);
        m_templateIncludedGems->Update(templateInfo.m_includedGems);
    }
} // namespace O3DE::ProjectManager
