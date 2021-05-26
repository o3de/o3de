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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>
#include <QSpacerItem>
#include <QPainter>
#include <QPaintEvent>

namespace O3DE::ProjectManager
{
    FirstTimeUseScreen::FirstTimeUseScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        setLayout(vLayout);
        vLayout->setContentsMargins(s_contentMargins, s_contentMargins, s_contentMargins, s_contentMargins);

        setObjectName("firstTimeScreen");

        m_background.load(":/Backgrounds/FirstTimeBackgroundImage.jpg");

        QLabel* titleLabel = new QLabel(this);
        titleLabel->setText(tr("Ready. Set. Create."));
        titleLabel->setObjectName("titleLabel");
        vLayout->addWidget(titleLabel);

        QLabel* introLabel = new QLabel(this);
        introLabel->setObjectName("introLabel");
        introLabel->setText(tr("Welcome to O3DE! Start something new by creating a project. Not sure what to create? \nExplore what's available by downloading our sample project."));
        vLayout->addWidget(introLabel);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(s_buttonSpacing);

        m_createProjectButton = new QPushButton(tr("Create Project"), this);
        m_createProjectButton->setObjectName("createProjectButton");
        buttonLayout->addWidget(m_createProjectButton);

        m_addProjectButton = new QPushButton(tr("Add a Project"), this);
        m_addProjectButton->setObjectName("addProjectButton");
        buttonLayout->addWidget(m_addProjectButton);

        QSpacerItem* buttonSpacer = new QSpacerItem(s_spacerSize, s_spacerSize, QSizePolicy::Expanding, QSizePolicy::Minimum);
        buttonLayout->addItem(buttonSpacer);

        vLayout->addItem(buttonLayout);

        QSpacerItem* verticalSpacer = new QSpacerItem(s_spacerSize, s_spacerSize, QSizePolicy::Minimum, QSizePolicy::Expanding);
        vLayout->addItem(verticalSpacer);

        connect(m_createProjectButton, &QPushButton::pressed, this, &FirstTimeUseScreen::HandleNewProjectButton);
        connect(m_addProjectButton, &QPushButton::pressed, this, &FirstTimeUseScreen::HandleAddProjectButton);
    }

    void FirstTimeUseScreen::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        QPainter painter(this);

        auto winSize = size();
        auto pixmapRatio = (float)m_background.width() / m_background.height();
        auto windowRatio = (float)winSize.width() / winSize.height();

        if (pixmapRatio > windowRatio)
        {
            auto newWidth = (int)(winSize.height() * pixmapRatio);
            auto offset = (newWidth - winSize.width()) / -2;
            painter.drawPixmap(offset, 0, newWidth, winSize.height(), m_background);
        }
        else
        {
            auto newHeight = (int)(winSize.width() / pixmapRatio);
            painter.drawPixmap(0, 0, winSize.width(), newHeight, m_background);
        }
    }

    bool FirstTimeUseScreen::IsTab()
    {
        return true;
    }

    ProjectManagerScreen FirstTimeUseScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::FirstTimeUse;
    }

    QString FirstTimeUseScreen::GetTabText()
    {
        return tr("");
    }

    void FirstTimeUseScreen::HandleNewProjectButton()
    {
        emit ResetScreenRequest(ProjectManagerScreen::CreateProject);
        emit ChangeScreenRequest(ProjectManagerScreen::CreateProject);
    }
    void FirstTimeUseScreen::HandleAddProjectButton()
    {
        emit ChangeScreenRequest(ProjectManagerScreen::ProjectsHome);
    }
} // namespace O3DE::ProjectManager
