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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpacerItem>

namespace O3DE::ProjectManager
{
    NewProjectSettingsScreen::NewProjectSettingsScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        this->setLayout(hLayout);

        QVBoxLayout* vLayout = new QVBoxLayout();

        QLabel* projectNameLabel = new QLabel(this);
        projectNameLabel->setText("Project Name");
        vLayout->addWidget(projectNameLabel);

        QLineEdit* projectNameLineEdit = new QLineEdit(this);
        vLayout->addWidget(projectNameLineEdit);

        QLabel* projectPathLabel = new QLabel(this);
        projectPathLabel->setText("Project Location");
        vLayout->addWidget(projectPathLabel);

        QLineEdit* projectPathLineEdit = new QLineEdit(this);
        vLayout->addWidget(projectPathLineEdit);

        QLabel* projectTemplateLabel = new QLabel(this);
        projectTemplateLabel->setText("Project Template");
        vLayout->addWidget(projectTemplateLabel);

        QHBoxLayout* templateLayout = new QHBoxLayout();
        vLayout->addItem(templateLayout);

        QRadioButton* projectTemplateStandardRadioButton = new QRadioButton(this);
        projectTemplateStandardRadioButton->setText("Standard(Recommened)");
        projectTemplateStandardRadioButton->setChecked(true);
        templateLayout->addWidget(projectTemplateStandardRadioButton);

        QRadioButton* projectTemplateEmptyRadioButton = new QRadioButton(this);
        projectTemplateEmptyRadioButton->setText("Empty");
        templateLayout->addWidget(projectTemplateEmptyRadioButton);

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
        return "Create Project";
    }

} // namespace O3DE::ProjectManager
