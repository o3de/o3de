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
#include <QEvent>
#include <QResizeEvent>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QMenu>
#include <QSpacerItem>
#include <QProgressBar>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>

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
        m_overlayLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        m_overlayLabel->setVisible(false);
        connect(m_overlayLabel, &QLabel::linkActivated, this, &LabelButton::OnLinkActivated);
        vLayout->addWidget(m_overlayLabel);

        m_buildOverlayLayout = new QVBoxLayout();
        m_buildOverlayLayout->addSpacing(10);

        QHBoxLayout* horizontalMessageLayout = new QHBoxLayout();

        horizontalMessageLayout->addSpacing(10);
        m_warningIcon = new QLabel(this);
        m_warningIcon->setPixmap(QIcon(":/Warning.svg").pixmap(20, 20));
        m_warningIcon->setAlignment(Qt::AlignTop);
        m_warningIcon->setVisible(false);
        horizontalMessageLayout->addWidget(m_warningIcon);

        horizontalMessageLayout->addSpacing(10);

        m_warningText = new QLabel("", this);
        m_warningText->setObjectName("projectWarningOverlay");
        m_warningText->setWordWrap(true);
        m_warningText->setAlignment(Qt::AlignLeft);
        m_warningText->setVisible(false);
        connect(m_warningText, &QLabel::linkActivated, this, &LabelButton::OnLinkActivated);
        horizontalMessageLayout->addWidget(m_warningText);

        QSpacerItem* textSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
        horizontalMessageLayout->addSpacerItem(textSpacer);

        m_buildOverlayLayout->addLayout(horizontalMessageLayout);

        QSpacerItem* buttonSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_buildOverlayLayout->addSpacerItem(buttonSpacer);

        QHBoxLayout* horizontalOpenEditorButtonLayout = new QHBoxLayout();
        horizontalOpenEditorButtonLayout->addSpacing(34);
        m_openEditorButton = new QPushButton(tr("Open Editor"), this);
        m_openEditorButton->setObjectName("openEditorButton");
        m_openEditorButton->setDefault(true);
        m_openEditorButton->setVisible(false);
        horizontalOpenEditorButtonLayout->addWidget(m_openEditorButton);
        horizontalOpenEditorButtonLayout->addSpacing(34);
        m_buildOverlayLayout->addLayout(horizontalOpenEditorButtonLayout);

        QHBoxLayout* horizontalButtonLayout = new QHBoxLayout();
        horizontalButtonLayout->addSpacing(34);
        m_actionButton = new QPushButton(tr("Project Action"), this);
        m_actionButton->setVisible(false);
        horizontalButtonLayout->addWidget(m_actionButton);
        horizontalButtonLayout->addSpacing(34);

        m_buildOverlayLayout->addLayout(horizontalButtonLayout);
        m_buildOverlayLayout->addSpacing(16);

        vLayout->addItem(m_buildOverlayLayout);

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

    void LabelButton::SetLogUrl(const QUrl& url)
    {
        m_logUrl = url;
    }

    QProgressBar* LabelButton::GetProgressBar()
    {
        return m_progressBar;
    }

    QPushButton* LabelButton::GetOpenEditorButton()
    {
        return m_openEditorButton;
    }

    QPushButton* LabelButton::GetActionButton()
    {
        return m_actionButton;
    }

    QLabel* LabelButton::GetWarningLabel()
    {
        return m_warningText;
    }

    QLabel* LabelButton::GetWarningIcon()
    {
        return m_warningIcon;
    }

    void LabelButton::OnLinkActivated(const QString& /*link*/)
    {
        QDesktopServices::openUrl(m_logUrl);
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
        m_projectImageLabel->SetEnabled(false);
        m_projectImageLabel->SetOverlayText(tr("Processing...\n\n"));

        QProgressBar* progressBar = m_projectImageLabel->GetProgressBar();
        progressBar->setVisible(true);
        progressBar->setValue(0);
    }

    void ProjectButton::ReadySetup()
    {
        connect(m_projectImageLabel->GetOpenEditorButton(), &QPushButton::clicked, [this](){ emit OpenProject(m_projectInfo.m_path); });

        QMenu* menu = new QMenu(this);
        menu->addAction(tr("Edit Project Settings..."), this, [this]() { emit EditProject(m_projectInfo.m_path); });
        menu->addAction(tr("Build"), this, [this]() { emit BuildProject(m_projectInfo); });
        menu->addSeparator();
        menu->addAction(tr("Open Project folder..."), this, [this]()
        { 
            AzQtComponents::ShowFileOnDesktop(m_projectInfo.m_path);
        });
        menu->addSeparator();
        menu->addAction(tr("Duplicate"), this, [this]() { emit CopyProject(m_projectInfo); });
        menu->addSeparator();
        menu->addAction(tr("Remove from O3DE"), this, [this]() { emit RemoveProject(m_projectInfo.m_path); });
        menu->addAction(tr("Delete this Project"), this, [this]() { emit DeleteProject(m_projectInfo.m_path); });

        QPushButton* projectMenuButton = new QPushButton(this);
        projectMenuButton->setObjectName("projectMenuButton");
        projectMenuButton->setMenu(menu);
        m_projectFooter->layout()->addWidget(projectMenuButton);
    }

    void ProjectButton::SetProjectButtonAction(const QString& text, AZStd::function<void()> lambda)
    {
        QPushButton* projectActionButton = m_projectImageLabel->GetActionButton();
        if (!m_actionButtonConnection)
        {
            projectActionButton->setVisible(true);
        }
        else
        {
            disconnect(m_actionButtonConnection);
        }

        projectActionButton->setText(text);
        m_actionButtonConnection = connect(projectActionButton, &QPushButton::clicked, lambda);
    }

    void ProjectButton::SetProjectBuildButtonAction()
    {
        m_projectImageLabel->GetWarningLabel()->setText(tr("Building project required."));
        m_projectImageLabel->GetWarningIcon()->setVisible(true);
        m_projectImageLabel->GetWarningLabel()->setVisible(true);
        SetProjectButtonAction(tr("Build Project"), [this]() { emit BuildProject(m_projectInfo); });
    }

    void ProjectButton::SetBuildLogsLink(const QUrl& logUrl)
    {
        m_projectImageLabel->SetLogUrl(logUrl);
    }

    void ProjectButton::ShowBuildFailed(bool show, const QUrl& logUrl)
    {
        if (!logUrl.isEmpty())
        {
            m_projectImageLabel->GetWarningLabel()->setText(tr("Failed to build. Click to <a href=\"logs\">view logs</a>."));
        }
        else
        {
            m_projectImageLabel->GetWarningLabel()->setText(tr("Project failed to build."));
        }

        m_projectImageLabel->GetWarningLabel()->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        m_projectImageLabel->GetWarningIcon()->setVisible(show);
        m_projectImageLabel->GetWarningLabel()->setVisible(show);
        m_projectImageLabel->SetLogUrl(logUrl);
        SetProjectButtonAction(tr("Build Project"), [this]() { emit BuildProject(m_projectInfo); });
    }

    void ProjectButton::BuildThisProject()
    {
        emit BuildProject(m_projectInfo);
    }

    void ProjectButton::SetLaunchButtonEnabled(bool enabled)
    {
        m_projectImageLabel->SetEnabled(enabled);
    }

    void ProjectButton::SetButtonOverlayText(const QString& text)
    {
        m_projectImageLabel->SetOverlayText(text);
    }

    void ProjectButton::SetProgressBarValue(int progress)
    {
        m_projectImageLabel->GetProgressBar()->setValue(progress);
    }

    void ProjectButton::enterEvent(QEvent* /*event*/)
    {
        m_projectImageLabel->GetOpenEditorButton()->setVisible(true);
    }

    void ProjectButton::leaveEvent(QEvent* /*event*/)
    {
        m_projectImageLabel->GetOpenEditorButton()->setVisible(false);
    }

    LabelButton* ProjectButton::GetLabelButton()
    {
        return m_projectImageLabel;
    }
} // namespace O3DE::ProjectManager
