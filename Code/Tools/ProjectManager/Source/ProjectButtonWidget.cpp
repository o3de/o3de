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

#include <ProjectButtonWidget.h>


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QMenu>
#include <QSpacerItem>

//#define SHOW_ALL_PROJECT_ACTIONS

namespace O3DE::ProjectManager
{
    inline constexpr static int s_projectImageWidth = 210;
    inline constexpr static int s_projectImageHeight = 280;

    LabelButton::LabelButton(QWidget* parent)
        : QLabel(parent)
    {
    }

    void LabelButton::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        emit triggered();
    }

    ProjectButton::ProjectButton(const ProjectInfo& projectInfo, QWidget* parent)
        : QFrame(parent)
        , m_projectInfo(projectInfo)
    {
        if (m_projectInfo.m_imagePath.isEmpty())
        {
            m_projectInfo.m_imagePath = ":/DefaultProjectImage.png";
        }

        Setup();
    }

    void ProjectButton::Setup()
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setSpacing(0);
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        m_projectImageLabel = new LabelButton(this);
        m_projectImageLabel->setFixedSize(s_projectImageWidth, s_projectImageHeight);
        vLayout->addWidget(m_projectImageLabel);

        m_projectImageLabel->setPixmap(
            QPixmap(m_projectInfo.m_imagePath).scaled(m_projectImageLabel->size(), Qt::KeepAspectRatioByExpanding));

        QMenu* newProjectMenu = new QMenu(this);
        m_editProjectAction = newProjectMenu->addAction(tr("Edit Project Settings..."));
        newProjectMenu->addSeparator();
        m_copyProjectAction = newProjectMenu->addAction(tr("Duplicate"));
        newProjectMenu->addSeparator();
        m_removeProjectAction = newProjectMenu->addAction(tr("Remove from O3DE"));
        m_deleteProjectAction = newProjectMenu->addAction(tr("Delete this Project"));

#ifdef SHOW_ALL_PROJECT_ACTIONS
        m_editProjectGemsAction = newProjectMenu->addAction(tr("Cutomize Gems..."));

#endif

        m_projectSettingsMenuButton = new QPushButton(this);
        m_projectSettingsMenuButton->setText(m_projectInfo.m_displayName);
        m_projectSettingsMenuButton->setMenu(newProjectMenu);
        m_projectSettingsMenuButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        m_projectSettingsMenuButton->setStyleSheet("font-size: 14px; text-align:left;");
        vLayout->addWidget(m_projectSettingsMenuButton);

        setFixedSize(s_projectImageWidth, s_projectImageHeight + m_projectSettingsMenuButton->height());

        connect(m_projectImageLabel, &LabelButton::triggered, [this]() { emit OpenProject(m_projectInfo.m_path); });
        connect(m_editProjectAction, &QAction::triggered, [this]() { emit EditProject(m_projectInfo.m_path); });
        connect(m_copyProjectAction, &QAction::triggered, [this]() { emit CopyProject(m_projectInfo.m_path); });
        connect(m_removeProjectAction, &QAction::triggered, [this]() { emit RemoveProject(m_projectInfo.m_path); });
        connect(m_deleteProjectAction, &QAction::triggered, [this]() { emit DeleteProject(m_projectInfo.m_path); });

#ifdef SHOW_ALL_PROJECT_ACTIONS
        connect(m_editProjectGemsAction, &QAction::triggered, [this]() { emit EditProjectGems(m_projectInfo.m_path); });
#endif
    }

} // namespace O3DE::ProjectManager
