/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectButtonWidget.h>
#include <ProjectManagerDefs.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QMenu>
#include <QSpacerItem>
#include <QProgressBar>
#include <QDir>
#include <QFileInfo>

namespace O3DE::ProjectManager
{
    LabelButton::LabelButton(QWidget* parent)
        : QLabel(parent)
    {
        setObjectName("labelButton");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(5);

        setLayout(vLayout);
        m_overlayLabel = new QLabel("", this);
        m_overlayLabel->setObjectName("labelButtonOverlay");
        m_overlayLabel->setWordWrap(true);
        m_overlayLabel->setAlignment(Qt::AlignCenter);
        m_overlayLabel->setVisible(false);
        vLayout->addWidget(m_overlayLabel);

        m_buildButton = new QPushButton(tr("Build Project"), this);
        m_buildButton->setVisible(false);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setObjectName("labelButtonProgressBar");
        m_progressBar->setVisible(false);
        vLayout->addWidget(m_progressBar);
    }

    void LabelButton::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        if(m_enabled)
        {
            emit triggered();
        }
    }

    void LabelButton::SetEnabled(bool enabled)
    {
        m_enabled = enabled;
        m_overlayLabel->setVisible(!enabled);
    }

    void LabelButton::SetOverlayText(const QString& text)
    {
        m_overlayLabel->setText(text);
    }

    QLabel* LabelButton::GetOverlayLabel()
    {
        return m_overlayLabel;
    }

    QProgressBar* LabelButton::GetProgressBar()
    {
        return m_progressBar;
    }

    QPushButton* LabelButton::GetBuildButton()
    {
        return m_buildButton;
    }

    ProjectButton::ProjectButton(const ProjectInfo& projectInfo, QWidget* parent, bool processing)
        : QFrame(parent)
        , m_projectInfo(projectInfo)
    {
        BaseSetup();
        if (processing)
        {
            ProcessingSetup();
        }
        else
        {
            ReadySetup();
        }
    }

    void ProjectButton::BaseSetup()
    {
        setObjectName("projectButton");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setSpacing(0);
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        m_projectImageLabel = new LabelButton(this);
        m_projectImageLabel->setFixedSize(ProjectPreviewImageWidth, ProjectPreviewImageHeight);
        m_projectImageLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        connect(m_projectImageLabel, &LabelButton::triggered, [this]() { emit OpenProject(m_projectInfo.m_path); });
        vLayout->addWidget(m_projectImageLabel);

        QString projectPreviewPath = QDir(m_projectInfo.m_path).filePath(m_projectInfo.m_iconPath);
        QFileInfo doesPreviewExist(projectPreviewPath);
        if (!doesPreviewExist.exists() || !doesPreviewExist.isFile())
        {
            projectPreviewPath = ":/DefaultProjectImage.png";
        }
        m_projectImageLabel->setPixmap(QPixmap(projectPreviewPath).scaled(m_projectImageLabel->size(), Qt::KeepAspectRatioByExpanding));

        m_projectFooter = new QFrame(this);
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setContentsMargins(0, 0, 0, 0);
        m_projectFooter->setLayout(hLayout);
        {
            QLabel* projectNameLabel = new QLabel(m_projectInfo.GetProjectDisplayName(), this);
            hLayout->addWidget(projectNameLabel);
        }

        vLayout->addWidget(m_projectFooter);
    }

    void ProjectButton::ProcessingSetup()
    {
        m_projectImageLabel->GetOverlayLabel()->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        m_projectImageLabel->SetEnabled(false);
        m_projectImageLabel->SetOverlayText(tr("Processing...\n\n"));

        QProgressBar* progressBar = m_projectImageLabel->GetProgressBar();
        progressBar->setVisible(true);
        progressBar->setValue(0);
    }

    void ProjectButton::ReadySetup()
    {
        connect(m_projectImageLabel->GetBuildButton(), &QPushButton::clicked, [this](){ emit BuildProject(m_projectInfo); });

        QMenu* menu = new QMenu(this);
        menu->addAction(tr("Edit Project Settings..."), this, [this]() { emit EditProject(m_projectInfo.m_path); });
        menu->addAction(tr("Build"), this, [this]() { emit BuildProject(m_projectInfo); });
        menu->addSeparator();
        menu->addAction(tr("Open Project folder..."), this, [this]()
        { 
            AzQtComponents::ShowFileOnDesktop(m_projectInfo.m_path);
        });
        menu->addSeparator();
        menu->addAction(tr("Duplicate"), this, [this]() { emit CopyProject(m_projectInfo.m_path); });
        menu->addSeparator();
        menu->addAction(tr("Remove from O3DE"), this, [this]() { emit RemoveProject(m_projectInfo.m_path); });
        menu->addAction(tr("Delete this Project"), this, [this]() { emit DeleteProject(m_projectInfo.m_path); });

        QPushButton* projectMenuButton = new QPushButton(this);
        projectMenuButton->setObjectName("projectMenuButton");
        projectMenuButton->setMenu(menu);
        m_projectFooter->layout()->addWidget(projectMenuButton);
    }

    void ProjectButton::SetLaunchButtonEnabled(bool enabled)
    {
        m_projectImageLabel->SetEnabled(enabled);
    }

    void ProjectButton::ShowBuildButton(bool show)
    {
        QSpacerItem* buttonSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);

        m_projectImageLabel->layout()->addItem(buttonSpacer);
        m_projectImageLabel->layout()->addWidget(m_projectImageLabel->GetBuildButton());
        m_projectImageLabel->GetBuildButton()->setVisible(show);
    }

    void ProjectButton::SetButtonOverlayText(const QString& text)
    {
        m_projectImageLabel->SetOverlayText(text);
    }

    void ProjectButton::SetProgressBarValue(int progress)
    {
        m_projectImageLabel->GetProgressBar()->setValue(progress);
    }
} // namespace O3DE::ProjectManager
