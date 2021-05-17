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

namespace O3DE::ProjectManager
{
    constexpr const char* k_pathProperty = "Path";

    NewProjectSettingsScreen::NewProjectSettingsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        this->setLayout(hLayout);

        QVBoxLayout* vLayout = new QVBoxLayout(this);

        QLabel* projectNameLabel = new QLabel(tr("Project Name"), this);
        vLayout->addWidget(projectNameLabel);

        m_projectNameLineEdit = new QLineEdit(tr("New Project"), this);
        vLayout->addWidget(m_projectNameLineEdit);

        QLabel* projectPathLabel = new QLabel(tr("Project Location"), this);
        vLayout->addWidget(projectPathLabel);

        {
            QHBoxLayout* projectPathLayout = new QHBoxLayout(this);

            m_projectPathLineEdit = new QLineEdit(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),this);
            projectPathLayout->addWidget(m_projectPathLineEdit);

            QPushButton* browseButton = new QPushButton(tr("Browse"), this);
            connect(browseButton, &QPushButton::pressed, this, &NewProjectSettingsScreen::HandleBrowseButton);
            projectPathLayout->addWidget(browseButton);

            vLayout->addLayout(projectPathLayout);
        }

        QLabel* projectTemplateLabel = new QLabel(this);
        projectTemplateLabel->setText("Project Template");
        vLayout->addWidget(projectTemplateLabel);

        QHBoxLayout* templateLayout = new QHBoxLayout(this);
        vLayout->addItem(templateLayout);

        m_projectTemplateButtonGroup = new QButtonGroup(this);
        auto templatesResult = PythonBindingsInterface::Get()->GetProjectTemplates();
        if (templatesResult.IsSuccess() && !templatesResult.GetValue().isEmpty())
        {
            for (auto projectTemplate : templatesResult.GetValue())
            {
                QRadioButton* radioButton = new QRadioButton(projectTemplate.m_name, this);
                radioButton->setProperty(k_pathProperty, projectTemplate.m_path);
                m_projectTemplateButtonGroup->addButton(radioButton);

                templateLayout->addWidget(radioButton);
            }

            m_projectTemplateButtonGroup->buttons().first()->setChecked(true);
        }

        QSpacerItem* verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        vLayout->addItem(verticalSpacer);

        hLayout->addItem(vLayout);

        QWidget* gemsListPlaceholder = new QWidget(this);
        gemsListPlaceholder->setFixedWidth(250);
        hLayout->addWidget(gemsListPlaceholder);
    }

    ProjectManagerScreen NewProjectSettingsScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::NewProjectSettings;
    }

    QString NewProjectSettingsScreen::GetNextButtonText()
    {
        return tr("Next");
    }

    void NewProjectSettingsScreen::HandleBrowseButton()
    {
        QString defaultPath = m_projectPathLineEdit->text();
        if (defaultPath.isEmpty())
        {
            defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }

        QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("New project path"), defaultPath));
        if (!directory.isEmpty())
        {
            m_projectPathLineEdit->setText(directory);
        }
    }

    ProjectInfo NewProjectSettingsScreen::GetProjectInfo()
    {
        ProjectInfo projectInfo;
        projectInfo.m_projectName = m_projectNameLineEdit->text();
        projectInfo.m_path        = QDir::toNativeSeparators(m_projectPathLineEdit->text() + "/" + projectInfo.m_projectName);
        return projectInfo;
    }

    QString NewProjectSettingsScreen::GetProjectTemplatePath()
    {
        return m_projectTemplateButtonGroup->checkedButton()->property(k_pathProperty).toString();
    }

    bool NewProjectSettingsScreen::Validate()
    {
        bool projectNameIsValid = true;
        if (m_projectNameLineEdit->text().isEmpty())
        {
            projectNameIsValid = false;
        }

        bool projectPathIsValid = true;
        if (m_projectPathLineEdit->text().isEmpty())
        {
            projectPathIsValid = false;
        }

        QDir path(QDir::toNativeSeparators(m_projectPathLineEdit->text() + "/" + m_projectNameLineEdit->text()));
        if (path.exists() && !path.isEmpty())
        {
            projectPathIsValid = false;
        }

        return projectNameIsValid && projectPathIsValid;
    }
} // namespace O3DE::ProjectManager
