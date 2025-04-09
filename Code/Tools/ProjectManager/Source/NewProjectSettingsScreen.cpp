/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <NewProjectSettingsScreen.h>
#include <ProjectManagerDefs.h>
#include <PythonBindingsInterface.h>
#include <FormBrowseEditWidget.h>
#include <FormLineEditWidget.h>
#include <TemplateButtonWidget.h>
#include <PathValidator.h>
#include <EngineInfo.h>
#include <CreateProjectCtrl.h>
#include <TagWidget.h>
#include <ProjectUtils.h>
#include <AddRemoteTemplateDialog.h>
#include <DownloadRemoteTemplateDialog.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzQtComponents/Components/FlowLayout.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QSpacerItem>

#include <QFrame>
#include <QScrollArea>
#include <QAbstractButton>

namespace O3DE::ProjectManager
{
    constexpr const char* k_templateIndexProperty = "TemplateIndex";
    constexpr const char* k_templateNameProperty = "TemplateName";

    NewProjectSettingsScreen::NewProjectSettingsScreen(DownloadController* downloadController, QWidget* parent)
        : ProjectSettingsScreen(parent)
        , m_downloadController(downloadController)
    {
        const QString defaultName = GetDefaultProjectName();
        const QString defaultPath = QDir::toNativeSeparators(ProjectUtils::GetDefaultProjectPath() + "/" + defaultName);

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


            // we might have enough templates that we need to scroll
            QScrollArea* templatesScrollArea = new QScrollArea(this);
            QWidget* scrollWidget = new QWidget();

            m_templateFlowLayout = new FlowLayout(0, s_spacerSize, s_spacerSize);
            scrollWidget->setLayout(m_templateFlowLayout);

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
                        int projectTemplateIndex = button->property(k_templateIndexProperty).toInt();
                        if (m_selectedTemplateIndex != projectTemplateIndex)
                        {
                            const int oldIndex = m_selectedTemplateIndex;
                            m_selectedTemplateIndex = projectTemplateIndex;
                            UpdateTemplateDetails(m_templates.at(m_selectedTemplateIndex));
                            emit OnTemplateSelectionChanged(/*oldIndex=*/oldIndex, /*newIndex=*/m_selectedTemplateIndex);
                        }
                    }
                    else if (button == m_remoteTemplateButton)
                    {
                        AddRemoteTemplateDialog* addRemoteTemplateDialog = new AddRemoteTemplateDialog(this);
                        if (addRemoteTemplateDialog->exec() == QDialog::DialogCode::Accepted)
                        {
                            auto remoteTemplatesResult =
                                PythonBindingsInterface::Get()->GetProjectTemplatesForRepo(addRemoteTemplateDialog->GetRepoPath());
                            if (remoteTemplatesResult.IsSuccess() && !remoteTemplatesResult.GetValue().isEmpty())
                            {
                                // remove remote template button from layout so we can insert the new templates before it
                                m_templateFlowLayout->removeWidget(m_remoteTemplateButton);

                                int currentTemplateIndex = m_templates.size();
                                const QVector<ProjectTemplateInfo>& remoteTemplates = remoteTemplatesResult.GetValue();
                                for (const ProjectTemplateInfo& remoteTemplate : remoteTemplates)
                                {
                                    m_templates.push_back(remoteTemplate);

                                    // create template button
                                    QString projectPreviewPath = QDir(remoteTemplate.m_path).filePath(ProjectPreviewImagePath);
                                    QFileInfo doesPreviewExist(projectPreviewPath);
                                    if (!doesPreviewExist.exists() || !doesPreviewExist.isFile())
                                    {
                                        projectPreviewPath = ":/DefaultTemplate.png";
                                    }
                                    TemplateButton* templateButton =
                                        new TemplateButton(projectPreviewPath, remoteTemplate.m_displayName, this);
                                    templateButton->SetIsRemote(remoteTemplate.m_isRemote);
                                    templateButton->setCheckable(true);
                                    templateButton->setProperty(k_templateIndexProperty, currentTemplateIndex);
                                    templateButton->setProperty(k_templateNameProperty, remoteTemplate.m_name);

                                    m_projectTemplateButtonGroup->addButton(templateButton);
                                    m_templateFlowLayout->addWidget(templateButton);
                                    m_templateButtons.append(templateButton);
                                    ++currentTemplateIndex;
                                }

                                // add remote template button back to layout
                                m_templateFlowLayout->addWidget(m_remoteTemplateButton);
                            }
                        }
                    }
                });

            containerLayout->addWidget(templatesScrollArea);
        }
        projectTemplateWidget->setLayout(containerLayout);
        m_verticalLayout->addWidget(projectTemplateWidget);

        QFrame* projectTemplateDetails = CreateTemplateDetails(s_templateDetailsContentMargin);
        projectTemplateDetails->setObjectName("projectTemplateDetails");
        m_horizontalLayout->addWidget(projectTemplateDetails);

        connect(m_downloadController, &DownloadController::Done, this, &NewProjectSettingsScreen::HandleDownloadResult);
        connect(m_downloadController, &DownloadController::ObjectDownloadProgress, this, &NewProjectSettingsScreen::HandleDownloadProgress);
    }

    bool NewProjectSettingsScreen::IsDownloadingTemplate() const
    {
        if (m_selectedTemplateIndex < 0 || (m_selectedTemplateIndex > m_templates.size() - 1))
        {
            return false;
        }

        const ProjectTemplateInfo& templateInfo = m_templates.at(m_selectedTemplateIndex);
        return m_downloadController->IsDownloadingObject(templateInfo.m_name, DownloadController::Template);
    }

    void NewProjectSettingsScreen::HandleDownloadResult(const QString& templateName, bool succeeded)
    {
        auto foundButton = AZStd::ranges::find_if(
            m_templateButtons,
            [&templateName](const QAbstractButton* value)
            {
                return value->property(k_templateNameProperty) == templateName;
            });

        if (foundButton != m_templateButtons.end()  && succeeded)
        {
            // Convert button to point at the now downloaded template
            auto templatesResult = PythonBindingsInterface::Get()->GetProjectTemplates();
            if (templatesResult.IsSuccess() && !templatesResult.GetValue().isEmpty())
            {
                QVector<ProjectTemplateInfo> templates = templatesResult.GetValue();
                auto foundTemplate = AZStd::ranges::find_if(
                    templates,
                    [&templateName](const ProjectTemplateInfo& value)
                    {
                        return value.m_name == templateName;
                    });

                if (foundTemplate != templates.end())
                {
                    int templateIndex = (*foundButton)->property(k_templateIndexProperty).toInt();
                    m_templates[templateIndex] = (*foundTemplate);
                    (*foundButton)->SetIsRemote(false);
                }
            }
        }
        else if (foundButton != m_templateButtons.end())
        {
            (*foundButton)->ShowDownloadProgress(false);
        }
    }

    void NewProjectSettingsScreen::HandleDownloadProgress(const QString& templateName, DownloadController::DownloadObjectType objectType, int bytesDownloaded, int totalBytes)
    {
        if (objectType != DownloadController::DownloadObjectType::Template)
        {
            return;
        }

        auto foundButton = AZStd::ranges::find_if(
            m_templateButtons,
            [&templateName](const QAbstractButton* value)
            {
                return value->property(k_templateNameProperty) == templateName;
            });

        if (foundButton != m_templateButtons.end())
        {
            float percentage = static_cast<float>(bytesDownloaded) / totalBytes;
            (*foundButton)->SetProgressPercentage(percentage);
        }
    }

    QString NewProjectSettingsScreen::GetDefaultProjectName()
    {
        return "NewProject";
    }

    QString NewProjectSettingsScreen::GetProjectAutoPath()
    {
        const QString projectName = m_projectName->lineEdit()->text();
        return QDir::toNativeSeparators(ProjectUtils::GetDefaultProjectPath() + "/" + projectName);
    }

    ProjectManagerScreen NewProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::NewProjectSettings;
    }

    void NewProjectSettingsScreen::AddTemplateButtons()
    {
        auto templatesResult = PythonBindingsInterface::Get()->GetProjectTemplates();
        if (templatesResult.IsSuccess() && !templatesResult.GetValue().isEmpty())
        {
            m_templates = templatesResult.GetValue();

            // Add in remote templates
            auto remoteTemplatesResult = PythonBindingsInterface::Get()->GetProjectTemplatesForAllRepos();
            if (remoteTemplatesResult.IsSuccess() && !remoteTemplatesResult.GetValue().isEmpty())
            {
                const QVector<ProjectTemplateInfo>& remoteTemplates = remoteTemplatesResult.GetValue();
                for (const ProjectTemplateInfo& remoteTemplate : remoteTemplates)
                {
                    const auto found = AZStd::ranges::find_if(m_templates,
                        [remoteTemplate](const ProjectTemplateInfo& value)
                        {
                            return remoteTemplate.m_name == value.m_name;
                        });
                    if (found == m_templates.end())
                    {
                        m_templates.append(remoteTemplate);
                    }
                }
            }

            // sort alphabetically by display name (but putting Standard first) because they could be in any order
            std::sort(m_templates.begin(), m_templates.end(), [](const ProjectTemplateInfo& arg1, const ProjectTemplateInfo& arg2)
            {
                if (arg1.m_displayName == "Standard")
                {
                    return true;
                }
                else if (arg2.m_displayName == "Standard")
                {
                    return false;
                }
                else
                {
                    return arg1.m_displayName.toLower() < arg2.m_displayName.toLower();
                }
            });

            for (int index = 0; index < m_templates.size(); ++index)
            {
                ProjectTemplateInfo projectTemplate = m_templates.at(index);
                QString projectPreviewPath = QDir(projectTemplate.m_path).filePath(ProjectPreviewImagePath);
                QFileInfo doesPreviewExist(projectPreviewPath);
                if (!doesPreviewExist.exists() || !doesPreviewExist.isFile())
                {
                    projectPreviewPath = ":/DefaultTemplate.png";
                }
                TemplateButton* templateButton = new TemplateButton(projectPreviewPath, projectTemplate.m_displayName, this);
                templateButton->SetIsRemote(projectTemplate.m_isRemote);
                templateButton->setCheckable(true);
                templateButton->setProperty(k_templateIndexProperty, index);
                templateButton->setProperty(k_templateNameProperty, projectTemplate.m_name);
                
                m_projectTemplateButtonGroup->addButton(templateButton);
                m_templateFlowLayout->addWidget(templateButton);
                m_templateButtons.append(templateButton);
            }

            // Insert the add a remote template button
            m_remoteTemplateButton = new TemplateButton(":/DefaultTemplate.png", tr("Add remote Template"), this);
            m_projectTemplateButtonGroup->addButton(m_remoteTemplateButton);
            m_templateFlowLayout->addWidget(m_remoteTemplateButton);

            // Select the first project template (default selection).
            SelectProjectTemplate(0, /*blockSignals=*/true);
        }
    }

    void NewProjectSettingsScreen::NotifyCurrentScreen()
    {
        if (m_templates.isEmpty())
        {
            AddTemplateButtons();
        }

        if (!m_templates.isEmpty())
        {
            UpdateTemplateDetails(m_templates.first());
        }

        Validate();
    }

    QString NewProjectSettingsScreen::GetProjectTemplatePath()
    {
        AZ_Assert(m_selectedTemplateIndex == m_projectTemplateButtonGroup->checkedButton()->property(k_templateIndexProperty).toInt(),
            "Selected template index not in sync with the currently checked project template button.");
        const ProjectTemplateInfo& templateInfo = m_templates.at(m_selectedTemplateIndex);

        if (templateInfo.m_isRemote)
        {
            // if this is a remote template that has not been downloaded we cannot return a path
            return "";
        }

        return templateInfo.m_path;
    }

    QFrame* NewProjectSettingsScreen::CreateTemplateDetails(int margin)
    {
        QFrame* projectTemplateDetails = new QFrame(this);
        projectTemplateDetails->setObjectName("projectTemplateDetails");
        QVBoxLayout* templateDetailsLayout = new QVBoxLayout();
        templateDetailsLayout->setContentsMargins(margin, margin, margin, margin);
        templateDetailsLayout->setAlignment(Qt::AlignTop);
        {
            m_templateDisplayName = new QLabel(this);
            m_templateDisplayName->setObjectName("displayName");
            m_templateDisplayName->setWordWrap(true);
            templateDetailsLayout->addWidget(m_templateDisplayName);

            m_templateSummary = new QLabel(this);
            m_templateSummary->setObjectName("summary");
            m_templateSummary->setWordWrap(true);
            templateDetailsLayout->addWidget(m_templateSummary);

            QLabel* includedGemsTitle = new QLabel(tr("Included Gems"), this);
            includedGemsTitle->setObjectName("includedGemsTitle");
            templateDetailsLayout->addWidget(includedGemsTitle);

            m_templateIncludedGems = new TagContainerWidget(this);
            m_templateIncludedGems->setObjectName("includedGems");
            templateDetailsLayout->addWidget(m_templateIncludedGems);

            QLabel* moreGemsLabel = new QLabel(tr("Looking for more Gems?"), this);
            moreGemsLabel->setObjectName("moreGems");
            templateDetailsLayout->addWidget(moreGemsLabel);

            QLabel* browseCatalogLabel = new QLabel(tr("Browse the Gems Catalog to further customize your project."), this);
            browseCatalogLabel->setObjectName("browseCatalog");
            browseCatalogLabel->setWordWrap(true);
            templateDetailsLayout->addWidget(browseCatalogLabel);

            m_downloadTemplateButton = new QPushButton(tr("Download Template"), this);
            m_downloadTemplateButton->setVisible(false);
            templateDetailsLayout->addWidget(m_downloadTemplateButton);

            QPushButton* configureGemsButton = new QPushButton(tr("Configure with more Gems"), this);
            connect(configureGemsButton, &QPushButton::clicked, this, [=]()
                {
                    emit ChangeScreenRequest(ProjectManagerScreen::ProjectGemCatalog);
                });
            templateDetailsLayout->addWidget(configureGemsButton);
        }
        projectTemplateDetails->setLayout(templateDetailsLayout);
        return projectTemplateDetails;
    }

    void NewProjectSettingsScreen::StartTemplateDownload(const QString& templateName, const QString& destinationPath)
    {
        AZ_Assert(m_downloadController, "DownloadController must exist.");
        m_downloadController->AddObjectDownload(templateName, destinationPath, DownloadController::DownloadObjectType::Template);
        auto foundButton = AZStd::ranges::find_if(
            m_templateButtons,
            [&templateName](const QAbstractButton* value)
            {
                return value->property(k_templateNameProperty) == templateName;
            });

        if (foundButton != m_templateButtons.end())
        {
            (*foundButton)->ShowDownloadProgress(true);
        }
    }

    const ProjectTemplateInfo NewProjectSettingsScreen::GetSelectedProjectTemplateInfo() const
    {
       if(m_selectedTemplateIndex < 0 || m_selectedTemplateIndex >= m_templates.size())
       {
           return {};
       }

       return m_templates[m_selectedTemplateIndex];
    }

    void NewProjectSettingsScreen::ShowDownloadTemplateDialog(const ProjectTemplateInfo& templateInfo)
    {
        ProjectTemplateInfo resolvedTemplateInfo = templateInfo.IsValid() ? templateInfo : GetSelectedProjectTemplateInfo();
        if (!resolvedTemplateInfo.IsValid())
        {
            QMessageBox::critical(this, tr("Failed to find project template"), tr("The remote project template info for %1 could not be found or is invalid.\n\nPlease try refreshing the remote repository it came from, or download the template and register it through the o3de CLI.").arg(templateInfo.m_name));
            return;
        }

        DownloadRemoteTemplateDialog* dialog = new DownloadRemoteTemplateDialog(resolvedTemplateInfo, this);
        if (dialog->exec() == QDialog::DialogCode::Accepted)
        {
            StartTemplateDownload(resolvedTemplateInfo.m_name, dialog->GetInstallPath());
        }
    }

    void NewProjectSettingsScreen::UpdateTemplateDetails(const ProjectTemplateInfo& templateInfo)
    {
        m_templateDisplayName->setText(templateInfo.m_displayName);
        m_templateSummary->setText(templateInfo.m_summary);
        m_templateIncludedGems->Update(templateInfo.m_includedGems);

        m_downloadTemplateButton->setVisible(templateInfo.m_isRemote);
        m_downloadTemplateButton->disconnect();
        connect(m_downloadTemplateButton, &QPushButton::clicked, this, [&, templateInfo]()
        {
            ShowDownloadTemplateDialog(templateInfo);
        });
    }


    void NewProjectSettingsScreen::SelectProjectTemplate(int index, bool blockSignals)
    {
        const QList<QAbstractButton*> buttons = m_projectTemplateButtonGroup->buttons();
        if (index >= buttons.size())
        {
            return;
        }

        if (blockSignals)
        {
            m_projectTemplateButtonGroup->blockSignals(true);
        }

        QAbstractButton* button = buttons.at(index);
        button->setChecked(true);
        m_selectedTemplateIndex = button->property(k_templateIndexProperty).toInt();

        if (blockSignals)
        {
            m_projectTemplateButtonGroup->blockSignals(false);
        }
    }

    AZ::Outcome<void, QString> NewProjectSettingsScreen::Validate() const
    {
        if (m_selectedTemplateIndex != -1 && m_templates[m_selectedTemplateIndex].m_isRemote)
        {
            return AZ::Failure<QString>(tr("You cannot create a new project or configure gems with a template that has not been downloaded. Please download it before proceeding."));
        }

        return ProjectSettingsScreen::Validate();
    }

    void NewProjectSettingsScreen::OnProjectNameUpdated()
    {
        if (ValidateProjectName() && !m_userChangedProjectPath)
        {
            m_projectPath->setText(GetProjectAutoPath());
        }
    }

    void NewProjectSettingsScreen::OnProjectPathUpdated()
    {
        const QString defaultPath =
            QDir::toNativeSeparators(ProjectUtils::GetDefaultProjectPath() + "/" + GetDefaultProjectName());
        const QString autoPath = GetProjectAutoPath();
        const QString path = m_projectPath->lineEdit()->text();
        m_userChangedProjectPath = path != defaultPath && path != autoPath;

        ValidateProjectPath();
    }

} // namespace O3DE::ProjectManager
