/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectButtonWidget.h>
#include <ProjectManagerDefs.h>
#include <ProjectUtils.h>
#include <ProjectManager_Traits_Platform.h>
#include <ProjectExportController.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/PlatformId/PlatformId.h>

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
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMovie>

namespace O3DE::ProjectManager
{
    LabelButton::LabelButton(QWidget* parent)
        : QLabel(parent)
    {
        // Space for content excluding borders
        constexpr int contentSpaceWidth = ProjectPreviewImageWidth - 2;

        // The height of a third of the button split into 3 sections, top, middle and bottom
        constexpr int threeWaySplitHeight = (ProjectPreviewImageHeight - 2) / 3;

        setObjectName("labelButton");

        // Use GridLayout so widgets can be overlapped
        QGridLayout* overlayLayout = new QGridLayout();
        overlayLayout->setContentsMargins(0, 0, 0, 0);
        overlayLayout->setSpacing(0);
        setLayout(overlayLayout);

        m_darkenOverlay = new QLabel(this);
        m_darkenOverlay->setObjectName("labelButtonOverlay");
        m_darkenOverlay->setVisible(true);
        overlayLayout->addWidget(m_darkenOverlay, 0, 0);

        m_projectOverlayLayout = new QVBoxLayout();
        m_projectOverlayLayout->setContentsMargins(0, 0, 0, 0);
        m_projectOverlayLayout->setSpacing(0);
        m_projectOverlayLayout->setAlignment(Qt::AlignCenter);

        // Split the button into 3 fixed size sections so content alignment is easier to manage
        // If widgets in other sections are shown or hidden it will not offset alignment in other sections
        QWidget* topWidget = new QWidget();
        topWidget->setFixedSize(contentSpaceWidth, threeWaySplitHeight);
        QVBoxLayout* verticalMessageLayout = new QVBoxLayout();
        verticalMessageLayout->setContentsMargins(0, 0, 0, 0);
        verticalMessageLayout->setSpacing(4);
        verticalMessageLayout->setAlignment(Qt::AlignCenter);
        topWidget->setLayout(verticalMessageLayout);

        verticalMessageLayout->addSpacing(10);

        QHBoxLayout* horizontalWarningMessageLayout = new QHBoxLayout();
        horizontalWarningMessageLayout->setContentsMargins(0, 0, 0, 0);
        horizontalWarningMessageLayout->setSpacing(0);
        horizontalWarningMessageLayout->setAlignment(Qt::AlignRight | Qt::AlignTop);

        m_warningSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
        horizontalWarningMessageLayout->addSpacerItem(m_warningSpacer);

        horizontalWarningMessageLayout->addSpacing(10);
        m_warningIcon = new QLabel(this);
        m_warningIcon->setObjectName("projectWarningIconOverlay");
        m_warningIcon->setPixmap(QIcon(":/Warning.svg").pixmap(32, 32));
        m_warningIcon->setAlignment(Qt::AlignCenter);
        m_warningIcon->setVisible(false);
        horizontalWarningMessageLayout->addWidget(m_warningIcon);

        m_cloudIcon = new QLabel(this);
        m_cloudIcon->setObjectName("projectCloudIconOverlay");
        m_cloudIcon->setPixmap(QIcon(":/Download.svg").pixmap(32, 32));
        m_cloudIcon->setVisible(false);
        horizontalWarningMessageLayout->addWidget(m_cloudIcon);

        horizontalWarningMessageLayout->addSpacing(15);

        verticalMessageLayout->addLayout(horizontalWarningMessageLayout);

        m_messageLabel = new QLabel("", this);
        m_messageLabel->setObjectName("projectMessageOverlay");
        m_messageLabel->setAlignment(Qt::AlignCenter);
        m_messageLabel->setVisible(true);
        verticalMessageLayout->addWidget(m_messageLabel);

        m_subMessageLabel = new QLabel("", this);
        m_subMessageLabel->setObjectName("projectSubMessageOverlay");
        m_subMessageLabel->setAlignment(Qt::AlignCenter);
        m_subMessageLabel->setVisible(true);
        verticalMessageLayout->addWidget(m_subMessageLabel);

        verticalMessageLayout->addStretch();

        m_projectOverlayLayout->addWidget(topWidget);

        QWidget* middleWidget = new QWidget();
        middleWidget->setFixedSize(contentSpaceWidth, threeWaySplitHeight);
        QVBoxLayout* verticalCenterLayout = new QVBoxLayout();
        verticalCenterLayout->setContentsMargins(0, 0, 0, 0);
        verticalCenterLayout->setSpacing(0);
        verticalCenterLayout->setAlignment(Qt::AlignCenter);
        middleWidget->setLayout(verticalCenterLayout);

        m_buildingAnimation = new QLabel("", this);
        m_buildingAnimation->setObjectName("buildingAnimationOverlay");
        m_buildingAnimation->setAlignment(Qt::AlignCenter);
        m_buildingAnimation->setVisible(false);
        m_buildingAnimation->setMovie(new QMovie(":/SpinningGears.webp"));
        m_buildingAnimation->movie()->start();
        verticalCenterLayout->addWidget(m_buildingAnimation);

        // Download Progress
        QWidget* m_downloadProgress = new QWidget(this);
        m_progessBar = new QProgressBar(this);
        m_progessBar->setVisible(false);

        QVBoxLayout* downloadProgressLayout = new QVBoxLayout();
        QHBoxLayout* downloadProgressTextLayout = new QHBoxLayout();

        m_downloadMessageLabel = new QLabel(tr("Downloading Project"), this);
        m_downloadMessageLabel->setAlignment(Qt::AlignCenter);
        m_downloadMessageLabel->setVisible(false);
        verticalCenterLayout->addWidget(m_downloadMessageLabel);

        downloadProgressTextLayout->addSpacing(25);
        m_progressMessageLabel = new QLabel(tr("0%"), this);
        m_progressMessageLabel->setAlignment(Qt::AlignRight);
        m_progressMessageLabel->setVisible(false);
        downloadProgressTextLayout->addWidget(m_progressMessageLabel);
        downloadProgressTextLayout->addSpacing(25);
        verticalCenterLayout->addLayout(downloadProgressTextLayout);

        QHBoxLayout* progressbarLayout = new QHBoxLayout();
        downloadProgressLayout->addLayout(progressbarLayout);
        m_downloadProgress->setLayout(downloadProgressLayout);
        progressbarLayout->addSpacing(20);
        progressbarLayout->addWidget(m_progessBar);
        progressbarLayout->addSpacing(20);
        verticalCenterLayout->addWidget(m_downloadProgress);

        m_projectOverlayLayout->addWidget(middleWidget);

        QWidget* bottomWidget = new QWidget();
        bottomWidget->setFixedSize(contentSpaceWidth, threeWaySplitHeight);

        QVBoxLayout* verticalButtonLayout = new QVBoxLayout();
        verticalButtonLayout->setContentsMargins(0, 0, 0, 0);
        verticalButtonLayout->setSpacing(5);
        verticalButtonLayout->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        bottomWidget->setLayout(verticalButtonLayout);

        m_openEditorButton = new QPushButton(tr("Open Editor"), this);
        m_openEditorButton->setObjectName("openEditorButton");
        m_openEditorButton->setDefault(true);
        m_openEditorButton->setVisible(false);
        verticalButtonLayout->addWidget(m_openEditorButton);

        m_actionButton = new QPushButton(tr("Project Action"), this);
        m_actionButton->setObjectName("projectActionButton");
        m_actionButton->setVisible(false);
        verticalButtonLayout->addWidget(m_actionButton);

        // This button has seperate styling with a red button instead of a blue button as for m_actionButton
        // Seperate buttons are used to avoid stutter from reloading style after changing object name
        m_actionCancelButton = new QPushButton(tr("Cancel Project Action"), this);
        m_actionCancelButton->setObjectName("projectActionCancelButton");
        m_actionCancelButton->setProperty("danger", true);
        m_actionCancelButton->setVisible(false);
        verticalButtonLayout->addWidget(m_actionCancelButton);

        m_showLogsButton = new QPushButton(tr("Show logs"), this);
        m_showLogsButton->setObjectName("projectShowLogsButton");
        m_showLogsButton->setVisible(false);
        verticalButtonLayout->addWidget(m_showLogsButton);

        verticalButtonLayout->addSpacing(20);

        m_projectOverlayLayout->addWidget(bottomWidget);

        overlayLayout->addLayout(m_projectOverlayLayout, 0, 0);
    }

    void LabelButton::mousePressEvent(QMouseEvent* event)
    {
        emit triggered(event);
    }

    QLabel* LabelButton::GetMessageLabel()
    {
        return m_messageLabel;
    }

    QLabel* LabelButton::GetSubMessageLabel()
    {
        return m_subMessageLabel;
    }

    QLabel* LabelButton::GetWarningIcon()
    {
        return m_warningIcon;
    }

    QLabel* LabelButton::GetCloudIcon()
    {
        return m_cloudIcon;
    }

    QSpacerItem* LabelButton::GetWarningSpacer()
    {
        return m_warningSpacer;
    }

    QLabel* LabelButton::GetBuildingAnimationLabel()
    {
        return m_buildingAnimation;
    }

    QPushButton* LabelButton::GetOpenEditorButton()
    {
        return m_openEditorButton;
    }

    QPushButton* LabelButton::GetActionButton()
    {
        return m_actionButton;
    }

    QPushButton* LabelButton::GetActionCancelButton()
    {
        return m_actionCancelButton;
    }

    QPushButton* LabelButton::GetShowLogsButton()
    {
        return m_showLogsButton;
    }

    QLabel* LabelButton::GetDarkenOverlay()
    {
        return m_darkenOverlay;
    }

    QProgressBar* LabelButton::GetProgressBar()
    {
        return m_progessBar;
    }

    QLabel* LabelButton::GetProgressPercentage()
    {
        return m_progressMessageLabel;
    }

    QLabel* LabelButton::GetDownloadMessageLabel()
    {
        return m_downloadMessageLabel;
    }

    ProjectButton::ProjectButton(const ProjectInfo& projectInfo, const EngineInfo& engineInfo, QWidget* parent)
        : QFrame(parent)
        , m_engineInfo(engineInfo)
        , m_projectInfo(projectInfo)
        , m_isProjectBuilding(false)
    {
        setObjectName("projectButton");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setSpacing(0);
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        m_projectImageLabel = new LabelButton(this);
        m_projectImageLabel->setFixedSize(ProjectPreviewImageWidth, ProjectPreviewImageHeight);
        m_projectImageLabel->setAlignment(Qt::AlignCenter);
        vLayout->addWidget(m_projectImageLabel);

        QString projectPreviewPath = QDir(m_projectInfo.m_path).filePath(m_projectInfo.m_iconPath);
        QFileInfo doesPreviewExist(projectPreviewPath);
        if (!doesPreviewExist.exists() || !doesPreviewExist.isFile())
        {
            projectPreviewPath = ":/DefaultProjectImage.png";
        }
        m_projectImageLabel->setPixmap(QPixmap(projectPreviewPath).scaled(m_projectImageLabel->size(), Qt::KeepAspectRatioByExpanding));

        QFrame* projectFooter = new QFrame(this);
        QVBoxLayout* projectFooterLayout = new QVBoxLayout();
        projectFooterLayout->setContentsMargins(0, 0, 0, 0);
        projectFooter->setLayout(projectFooterLayout);
        {
            // row 1
            QHBoxLayout* hLayout = new QHBoxLayout();
            hLayout->setContentsMargins(0, 0, 0, 0);

            QString projectName = m_projectInfo.GetProjectDisplayName();
            if (!m_projectInfo.m_version.isEmpty())
            {
                projectName +=" " + m_projectInfo.m_version;
            }
            m_projectNameLabel = new AzQtComponents::ElidingLabel(projectName, this);
            m_projectNameLabel->setObjectName("projectNameLabel");
            m_projectNameLabel->setToolTip(m_projectInfo.m_path);
            m_projectNameLabel->refreshStyle();
            hLayout->addWidget(m_projectNameLabel);

            m_projectMenuButton = new QPushButton(this);
            m_projectMenuButton->setObjectName("projectMenuButton");
            m_projectMenuButton->setMenu(CreateProjectMenu());
            hLayout->addWidget(m_projectMenuButton);
            projectFooterLayout->addLayout(hLayout);

            // row 2
            m_engineNameLabel = new AzQtComponents::ElidingLabel(m_engineInfo.m_name + " " + m_engineInfo.m_version, this);
            SetEngine(m_engineInfo);
            projectFooterLayout->addWidget(m_engineNameLabel);
        }

        vLayout->addWidget(projectFooter);

        connect(m_projectImageLabel->GetOpenEditorButton(), &QPushButton::clicked, [this]() {
            emit OpenProject(m_projectInfo.m_path);
        });
        connect(m_projectImageLabel, &LabelButton::triggered, [this](QMouseEvent* event) {
            if (!m_isProjectBuilding && event->button() == Qt::RightButton)
            {
                m_projectMenuButton->menu()->move(event->globalPos());
                m_projectMenuButton->menu()->show();
            }
        });
        connect(m_projectImageLabel->GetShowLogsButton(), &QPushButton::pressed, this, &ProjectButton::ShowLogs);

        SetState(ProjectButtonState::ReadyToLaunch);
    }

    ProjectButton::~ProjectButton() = default;

    QMenu* ProjectButton::CreateProjectMenu()
    {
        QMenu* menu = new QMenu(this);
        menu->addAction(tr("Edit Project Settings..."), this, [this]() { emit EditProject(m_projectInfo.m_path); });
        menu->addAction(tr("Configure Gems..."), this, [this]() { emit EditProjectGems(m_projectInfo.m_path); });
        menu->addAction(tr("Build"), this, [this]() { emit BuildProject(m_projectInfo); });
        menu->addSeparator();
        QMenu* exportMenu = menu->addMenu(tr("Export Launcher"));
        exportMenu->addAction(AZ_TRAIT_PROJECT_MANAGER_HOST_PLATFORM_NAME , this, [this](){ emit ExportProject(m_projectInfo, "export_source_built_project.py");});
        exportMenu->addAction(tr("Android"), this, [this](){ emit ExportProject(m_projectInfo, "export_source_android.py"); });
        menu->addAction(tr("Open Export Settings..."), this, [this]() { emit OpenProjectExportSettings(m_projectInfo.m_path); });
        menu->addSeparator();
        menu->addAction(tr("Open CMake GUI..."), this, [this]() { emit OpenCMakeGUI(m_projectInfo); });
        menu->addAction(tr("Open Android Project Generator..."), this, [this]() { emit OpenAndroidProjectGenerator(m_projectInfo.m_path); });
        menu->addSeparator();
        menu->addAction(tr("Open Project folder..."), this, [this]()
        { 
            AzQtComponents::ShowFileOnDesktop(m_projectInfo.m_path);
        });

#if AZ_TRAIT_PROJECT_MANAGER_CREATE_DESKTOP_SHORTCUT
        menu->addAction(tr("Create Editor desktop shortcut..."), this, [this]()
        {
            AZ::IO::FixedMaxPath editorExecutablePath = ProjectUtils::GetEditorExecutablePath(m_projectInfo.m_path.toUtf8().constData());

            const QString shortcutName = QString("%1 Editor").arg(m_projectInfo.m_displayName); 
            const QString arg = QString("--regset=\"/Amazon/AzCore/Bootstrap/project_path=%1\"").arg(m_projectInfo.m_path);

            auto result = ProjectUtils::CreateDesktopShortcut(shortcutName, editorExecutablePath.c_str(), { arg });
            if(result.IsSuccess())
            {
                QMessageBox::information(this, tr("Desktop Shortcut Created"), result.GetValue());
            }
            else
            {
                QMessageBox::critical(this, tr("Failed to create shortcut"), result.GetError());
            }
        });
#endif // AZ_TRAIT_PROJECT_MANAGER_CREATE_DESKTOP_SHORTCUT

        menu->addSeparator();
        menu->addAction(tr("Duplicate"), this, [this]() { emit CopyProject(m_projectInfo); });
        menu->addSeparator();
        menu->addAction(tr("Remove from O3DE"), this, [this]() { emit RemoveProject(m_projectInfo.m_path); });
        menu->addAction(tr("Delete this Project"), this, [this]() { emit DeleteProject(m_projectInfo.m_path); });

        return menu;
    }

    const ProjectInfo& ProjectButton::GetProjectInfo() const
    {
        return m_projectInfo;
    }

    void ProjectButton::ShowLogs()
    {
        if (!QDesktopServices::openUrl(m_projectInfo.m_logUrl))
        {
            qDebug() << "QDesktopServices::openUrl failed to open " << m_projectInfo.m_logUrl.toString() << "\n";
        }
    }

    void ProjectButton::SetEngine(const EngineInfo& engine)
    {
        m_engineInfo = engine;

        if (m_engineInfo.m_name.isEmpty() && !m_projectInfo.m_engineName.isEmpty())
        {
            // this project wants to use an engine that wasn't found, display the qualifier
            m_engineInfo.m_name = m_projectInfo.m_engineName;
            m_engineInfo.m_version = "";
        }

        m_engineNameLabel->SetText(m_engineInfo.m_name + " " + m_engineInfo.m_version);
        m_engineNameLabel->update();
        m_engineNameLabel->setObjectName(m_engineInfo.m_thisEngine ? "thisEngineLabel" : "otherEngineLabel");
        m_engineNameLabel->setToolTip(m_engineInfo.m_name + " " + m_engineInfo.m_version + " " + m_engineInfo.m_path);
        m_engineNameLabel->refreshStyle(); // important for styles to work correctly
    }

    void ProjectButton::SetProject(const ProjectInfo& project)
    {
        m_projectInfo = project;
        if (!m_projectInfo.m_version.isEmpty())
        {
            m_projectNameLabel->SetText(m_projectInfo.GetProjectDisplayName() + " " + m_projectInfo.m_version);
        }
        else
        {
            m_projectNameLabel->SetText(m_projectInfo.GetProjectDisplayName());
        }
        m_projectNameLabel->update();
        m_projectNameLabel->setToolTip(m_projectInfo.m_path);
        m_projectNameLabel->refreshStyle(); // important for styles to work correctly
    }

    void ProjectButton::SetState(ProjectButtonState state)
    {
        m_currentState = state;
        ResetButtonWidgets();

        switch (state)
        {
        default:
        case ProjectButtonState::ReadyToLaunch:
            ShowReadyState();
            break;
        case ProjectButtonState::Launching:
            ShowLaunchingState();
            break;
        case ProjectButtonState::NeedsToBuild:
            ShowBuildRequiredState();
            break;
        case ProjectButtonState::Building:
            ShowBuildingState();
            break;
        case ProjectButtonState::BuildFailed:
            ShowBuildFailedState();
            break;
        case ProjectButtonState::Exporting:
            ShowExportingState();
            break;
        case ProjectButtonState::ExportFailed:
            ShowExportFailedState();
            break;
        case ProjectButtonState::NotDownloaded:
            ShowNotDownloadedState();
            break;
        case ProjectButtonState::DownloadingBuildQueued:
        case ProjectButtonState::Downloading:
            ShowDownloadingState();
            break;
        }
    }

    void ProjectButton::ShowReadyState()
    {
        HideContextualLabelButtonWidgets();

        if (m_actionButtonConnection)
        {
            disconnect(m_actionButtonConnection);
        }

        m_projectMenuButton->setVisible(true);

        SetLaunchingEnabled(true);
        SetProjectBuilding(false);
    }

    void ProjectButton::ShowLaunchingState()
    {
        // Hide button in-case it is still showing
        m_projectImageLabel->GetOpenEditorButton()->hide();

        SetLaunchingEnabled(false);

        ShowMessage(tr("Opening Editor..."));
    }

    void ProjectButton::ShowBuildRequiredState()
    {
        ShowBuildButton();

        SetProjectBuilding(false);

        ShowWarning(tr("Project build required."));
    }

    void ProjectButton::ShowBuildingState()
    {
        m_projectImageLabel->GetShowLogsButton()->show();

        // Setting project to building also disables launching
        SetProjectBuilding(true);

        ShowMessage(tr("Building Project..."));
    }

    void ProjectButton::ShowExportingState()
    {
        m_projectImageLabel->GetShowLogsButton()->show();

        SetProjectExporting(true);

        ShowMessage(tr("Exporting Project..."));
    }

    void ProjectButton::ShowBuildFailedState()
    {
        ShowBuildButton();

        SetProjectBuilding(false);

        // Show, show logs button if avaliable
        m_projectImageLabel->GetShowLogsButton()->setVisible(!m_projectInfo.m_logUrl.isEmpty());

        ShowWarning(tr("Failed to build"));
    }

    void ProjectButton::ShowExportFailedState()
    {
        ShowBuildButton();

        SetProjectExporting(false);

        m_projectImageLabel->GetShowLogsButton()->setVisible(!m_projectInfo.m_logUrl.isEmpty());

        ShowWarning(tr(ProjectExportController::LauncherExportFailedMessage));
    }

    void ProjectButton::ShowNotDownloadedState()
    {
        m_projectImageLabel->GetCloudIcon()->setVisible(true);
        m_projectImageLabel->GetWarningSpacer()->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_projectMenuButton->setVisible(false);

        SetLaunchingEnabled(false);
    }

    void ProjectButton::ShowDownloadingState()
    {
        m_projectImageLabel->GetCloudIcon()->setVisible(true);
        m_projectImageLabel->GetWarningSpacer()->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_projectMenuButton->setVisible(false);

        m_projectImageLabel->GetDownloadMessageLabel()->setVisible(true);
        m_projectImageLabel->GetProgressPercentage()->setVisible(true);
        m_projectImageLabel->GetProgressBar()->setVisible(true);

        SetLaunchingEnabled(false);
    }

    void ProjectButton::SetProjectButtonAction(const QString& text, AZStd::function<void()> lambda)
    {
        QPushButton* projectActionButton;
        QPushButton* projectOtherActionButton;

        if (text.contains("Cancel", Qt::CaseInsensitive))
        {
            // Use red button is action involves cancelling
            projectActionButton = m_projectImageLabel->GetActionCancelButton();
            projectOtherActionButton = m_projectImageLabel->GetActionButton();
        }
        else
        {
            projectActionButton = m_projectImageLabel->GetActionButton();
            projectOtherActionButton = m_projectImageLabel->GetActionCancelButton();
        }

        if (m_actionButtonConnection)
        {
            disconnect(m_actionButtonConnection);
        }

        projectActionButton->setVisible(true);
        projectOtherActionButton->setVisible(false);

        projectActionButton->setText(text);
        projectActionButton->setMenu(nullptr);
        m_actionButtonConnection = connect(projectActionButton, &QPushButton::clicked, lambda);
    }

    void ProjectButton::SetBuildLogsLink(const QUrl& logUrl)
    {
        m_projectInfo.m_logUrl = logUrl;
    }

    void ProjectButton::SetProgressBarPercentage(const float percent)
    {
        m_projectImageLabel->GetProgressBar()->setValue(static_cast<int>(percent*100));
        m_projectImageLabel->GetProgressPercentage()->setText(QString("%1%").arg(static_cast<int>(percent*100)));
    }

    void ProjectButton::SetContextualText(const QString& text)
    {
        if (m_currentState == ProjectButtonState::Building || m_currentState == ProjectButtonState::Exporting)
        {
            // Don't update for empty build progress messages
            if (!text.isEmpty())
            {
                // Show info about what's currently building
                ShowMessage({}, text);
            }
        }
        else
        {
            ShowMessage(text);
        }
    }

    void ProjectButton::ShowBuildButton()
    {
        QPushButton* projectActionButton = m_projectImageLabel->GetActionButton();
        projectActionButton->setVisible(true);
        projectActionButton->setText(tr("Build Project"));
        disconnect(m_actionButtonConnection);

        QMenu* menu = new QMenu(this);
        QAction* autoBuildAction = menu->addAction(tr("Build Now"));
        connect( autoBuildAction, &QAction::triggered, this, [this](){ emit BuildProject(m_projectInfo); });

        QAction* openCMakeAction = menu->addAction(tr("Open CMake GUI..."));
        connect( openCMakeAction, &QAction::triggered, this, [this](){ emit OpenCMakeGUI(m_projectInfo); });

        projectActionButton->setMenu(menu);
    }

    void ProjectButton::ResetButtonWidgets()
    {
        HideContextualLabelButtonWidgets();
        SetProjectBuilding(false);
        SetProgressBarPercentage(0);

        m_projectImageLabel->GetDownloadMessageLabel()->setVisible(false);
        m_projectImageLabel->GetProgressPercentage()->setVisible(false);
        m_projectImageLabel->GetProgressBar()->setVisible(false);
    }

    // Only setting message without setting submessage will hide submessage
    void ProjectButton::ShowMessage(const QString& message, const QString& submessage)
    {
        bool showMessage = !message.isEmpty();
        bool showSubmessage = !submessage.isEmpty();
        QLabel* messageLabel = m_projectImageLabel->GetMessageLabel();
        QLabel* submessageLabel = m_projectImageLabel->GetSubMessageLabel();

        if (showMessage || showSubmessage)
        {
            // Hide any warning text, we cannot show the warning and a message at the same time
            ShowWarning();
        }

        // Keep main message if only submessage is set
        if (showMessage || showMessage == showSubmessage)
        {
            messageLabel->setText(message);
        }

        submessageLabel->setText(submessage);

        // Darken background if there is a message to make it easier to read
        m_projectImageLabel->GetDarkenOverlay()->setVisible(showMessage || showSubmessage);

        messageLabel->setVisible(showMessage || showSubmessage);
        submessageLabel->setVisible(showSubmessage);
    }

    void ProjectButton::ShowWarning(const QString& warning)
    {
        bool show = !warning.isEmpty();
        QLabel* warningIcon = m_projectImageLabel->GetWarningIcon();

        if (show)
        {
            // Hide any message text, we cannot show the warning and a message at the same time
            ShowMessage();

            m_projectImageLabel->GetWarningSpacer()->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
        else
        {
            m_projectImageLabel->GetWarningSpacer()->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        warningIcon->setToolTip(warning);
        warningIcon->setVisible(show);
    }

    void ProjectButton::SetLaunchingEnabled(bool enabled)
    {
        m_canLaunch = enabled;
    }

    void ProjectButton::SetProjectBuilding(bool isBuilding)
    {
        m_isProjectBuilding = isBuilding;

        QLabel* buildingAnimation = m_projectImageLabel->GetBuildingAnimationLabel();

        if (isBuilding)
        {
            SetLaunchingEnabled(false);
            m_projectImageLabel->GetActionCancelButton()->show();
        }

        buildingAnimation->movie()->setPaused(!isBuilding);
        buildingAnimation->setVisible(isBuilding);

        m_projectMenuButton->setVisible(!isBuilding);
    }

    void ProjectButton::SetProjectExporting(bool isExporting)
    {
        m_isProjectExporting = isExporting;

        if (isExporting)
        {
            SetLaunchingEnabled(false);
            m_projectImageLabel->GetActionCancelButton()->show();
        }

        if (QLabel* exportingAnimation = m_projectImageLabel->GetBuildingAnimationLabel())
        {
            exportingAnimation->movie()->setPaused(!isExporting);
            exportingAnimation->setVisible(isExporting);
        }
        
        m_projectMenuButton->setVisible(!isExporting);
    }

    void ProjectButton::HideContextualLabelButtonWidgets()
    {
        ShowMessage();
        ShowWarning();

        m_projectImageLabel->GetActionButton()->hide();
        m_projectImageLabel->GetActionCancelButton()->hide();
        m_projectImageLabel->GetShowLogsButton()->hide();
    }

    void ProjectButton::enterEvent(QEvent* /*event*/)
    {
        if (m_canLaunch)
        {
            m_projectImageLabel->GetOpenEditorButton()->setVisible(true);
        }
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
