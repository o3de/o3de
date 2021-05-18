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

#include <FirstTimeUseScreen.h>

#include <BigTallButtonWidget.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QSpacerItem>

namespace O3DE::ProjectManager
{
    FirstTimeUseScreen::FirstTimeUseScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        this->setLayout(vLayout);
        vLayout->setContentsMargins(s_contentMargins, s_contentMargins, s_contentMargins, s_contentMargins);

        QLabel* titleLabel = new QLabel(this);
        titleLabel->setText(tr("Ready. Set. Create!"));
        titleLabel->setStyleSheet("font-size: 60px; font-family: 'Open Sans';");
        vLayout->addWidget(titleLabel);

        QLabel* introLabel = new QLabel(this);
        introLabel->setTextFormat(Qt::AutoText);
        introLabel->setText(tr("<html><head/><body><p>Welcome to O3DE! Start something new by creating a project. Not sure what to create? </p><p>Explore what\342\200\231s available by downloading our sample project.</p></body></html>"));
        introLabel->setStyleSheet("font-size: 14px; font-family: 'Open Sans';");
        vLayout->addWidget(introLabel);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(s_buttonSpacing);

        QIcon createIcon;
        createIcon.addFile(QString::fromUtf8(":/Resources/Add.svg"));
        m_createProjectButton = new BigTallButton(createIcon, tr("Create Project"), this);
        m_createProjectButton->setIconSize(QSize(s_iconSize, s_iconSize));
        buttonLayout->addWidget(m_createProjectButton);

        QIcon addIcon;
        addIcon.addFile(QString::fromUtf8(":/Resources/Select_Folder.svg"));
        m_addProjectButton = new BigTallButton(addIcon, tr("Add a Project"), this);
        m_addProjectButton->setIconSize(QSize(s_iconSize, s_iconSize));
        buttonLayout->addWidget(m_addProjectButton);

        QSpacerItem* buttonSpacer = new QSpacerItem(s_spacerSize, s_spacerSize, QSizePolicy::Expanding, QSizePolicy::Minimum);
        buttonLayout->addItem(buttonSpacer);

        vLayout->addItem(buttonLayout);

        QSpacerItem* verticalSpacer = new QSpacerItem(s_spacerSize, s_spacerSize, QSizePolicy::Minimum, QSizePolicy::Expanding);
        vLayout->addItem(verticalSpacer);

        // Using border-image allows for scaling options background-image does not support
        setStyleSheet("O3DE--ProjectManager--ScreenWidget { border-image: url(:/Resources/FirstTimeBackgroundImage.jpg) repeat repeat; }");

        connect(m_createProjectButton, &QPushButton::pressed, this, &FirstTimeUseScreen::HandleNewProjectButton);
        connect(m_addProjectButton, &QPushButton::pressed, this, &FirstTimeUseScreen::HandleAddProjectButton);
    }

    ProjectManagerScreen FirstTimeUseScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::FirstTimeUse;
    }

    void FirstTimeUseScreen::HandleNewProjectButton()
    {
        emit ResetScreenRequest(ProjectManagerScreen::NewProjectSettingsCore);
        emit ChangeScreenRequest(ProjectManagerScreen::NewProjectSettingsCore);
    }
    void FirstTimeUseScreen::HandleAddProjectButton()
    {
        emit ChangeScreenRequest(ProjectManagerScreen::ProjectsHome);
    }

} // namespace O3DE::ProjectManager
