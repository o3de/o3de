/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoScreen.h>
#include <GemRepo/GemRepoItemDelegate.h>
#include <GemRepo/GemRepoListView.h>
#include <GemRepo/GemRepoModel.h>
#include <GemRepo/GemRepoAddDialog.h>
#include <GemRepo/GemRepoInspector.h>
#include <GemRepo/GemRepoProxyModel.h>
#include <PythonBindingsInterface.h>
#include <ProjectManagerDefs.h>
#include <ProjectUtils.h>
#include <AdjustableHeaderWidget.h>
#include <ProjectManagerDefs.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QFrame>
#include <QStackedWidget>
#include <QMessageBox>
#include <QItemSelectionModel>

namespace O3DE::ProjectManager
{
    GemRepoScreen::GemRepoScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        m_gemRepoModel = new GemRepoModel(this);
        m_gemRepoModel->setSortRole(GemRepoModel::UserRole::RoleName);
        connect(m_gemRepoModel, &GemRepoModel::ShowToastNotification, this, &GemRepoScreen::ShowStandardToastNotification);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        m_contentStack = new QStackedWidget(this);

        m_noRepoContent = CreateNoReposContent();
        m_contentStack->addWidget(m_noRepoContent);

        m_repoContent = CreateReposContent();
        m_contentStack->addWidget(m_repoContent);

        vLayout->addWidget(m_contentStack);

        m_notificationsView = AZStd::make_unique<AzToolsFramework::ToastNotificationsView>(this, AZ_CRC_CE("ReposNotificationsView"));
        m_notificationsView->SetOffset(QPoint(10, 10));
        m_notificationsView->SetMaxQueuedNotifications(1);
        m_notificationsView->SetRejectDuplicates(false); // we want to show notifications if a user repeats actions

        ScreensCtrl* screensCtrl = GetScreensCtrl(this);
        if (screensCtrl)
        {
            connect(this, &GemRepoScreen::NotifyRemoteContentRefreshed, screensCtrl, &ScreensCtrl::NotifyRemoteContentRefreshed);
        }
    }

    void GemRepoScreen::NotifyCurrentScreen()
    {
        constexpr bool downloadMissingOnly = true;
        PythonBindingsInterface::Get()->RefreshAllGemRepos(downloadMissingOnly);
        Reinit();

        // we might have downloading missing data so make sure to update the GemCatalog
        emit NotifyRemoteContentRefreshed();
    }

    void GemRepoScreen::Reinit()
    {
        QString selectedRepoUri;
        QPersistentModelIndex selectedIndex = m_selectionModel->currentIndex();
        if (selectedIndex.isValid())
        {
            selectedIndex = m_sortProxyModel->mapToSource(selectedIndex);
            selectedRepoUri = GemRepoModel::GetRepoUri(selectedIndex);
        }

        disconnect(m_gemRepoModel, &GemRepoModel::dataChanged, this, &GemRepoScreen::OnModelDataChanged);

        m_gemRepoModel->clear();
        FillModel();

        connect(m_gemRepoModel, &GemRepoModel::dataChanged, this, &GemRepoScreen::OnModelDataChanged);

        // If model contains any data show the repos
        if (m_gemRepoModel->rowCount())
        {
            m_contentStack->setCurrentWidget(m_repoContent);

            QPersistentModelIndex modelIndex;
            if (!selectedRepoUri.isEmpty())
            {
                // attempt to re-select the row with the unique RepoURI if it still exists
                modelIndex = m_gemRepoModel->FindModelIndexByRepoUri(selectedRepoUri);
                modelIndex = m_sortProxyModel->mapFromSource(modelIndex);
            }

            if (!modelIndex.isValid())
            {
                // fallback to selecting the first item in the list
                modelIndex = m_sortProxyModel->index(0, 0);
            }

            m_gemRepoListView->selectionModel()->setCurrentIndex(modelIndex, QItemSelectionModel::ClearAndSelect);
        }
        else
        {
            m_contentStack->setCurrentWidget(m_noRepoContent);
        }
    }

    void GemRepoScreen::HandleAddRepoButton()
    {
        GemRepoAddDialog* repoAddDialog = new GemRepoAddDialog(this);

        if (repoAddDialog->exec() == QDialog::DialogCode::Accepted)
        {
            QString repoUri = repoAddDialog->GetRepoPath();
            if (repoUri.isEmpty())
            {
                QMessageBox::warning(this, tr("No Input"), tr("Please provide a repo Uri."));
                return;
            }

            auto addGemRepoResult = PythonBindingsInterface::Get()->AddGemRepo(repoUri);
            if (addGemRepoResult.IsSuccess())
            {
                ShowStandardToastNotification(tr("Repo added successfully!"));

                Reinit();
                emit NotifyRemoteContentRefreshed();
            }
            else
            {
                QString failureMessage = tr("Failed to add gem repo: %1.").arg(repoUri);
                ProjectUtils::DisplayDetailedError(failureMessage, addGemRepoResult, this);
                AZ_Error("Project Manager", false, failureMessage.toUtf8());
            }
        }
    }

    void GemRepoScreen::HandleRemoveRepoButton(const QModelIndex& modelIndex)
    {
        QString repoName = m_gemRepoModel->GetName(modelIndex);

        QMessageBox::StandardButton warningResult = QMessageBox::warning(
            this, tr("Remove Repo"), tr("Are you sure you would like to remove gem repo: %1?").arg(repoName),
            QMessageBox::No | QMessageBox::Yes);

        if (warningResult == QMessageBox::Yes)
        {
            QString repoUri = m_gemRepoModel->GetRepoUri(modelIndex);
            bool removeGemRepoResult = PythonBindingsInterface::Get()->RemoveGemRepo(repoUri);
            if (removeGemRepoResult)
            {
                ShowStandardToastNotification(tr("Repo removed"));

                Reinit();
                emit NotifyRemoteContentRefreshed();
            }
            else
            {
                QString failureMessage = tr("Failed to remove gem repo: %1.").arg(repoUri);
                QMessageBox::critical(this, tr("Operation failed"), failureMessage);
                AZ_Error("Project Manger", false, failureMessage.toUtf8());
            }
        }
    }

    void GemRepoScreen::HandleRefreshAllButton()
    {
        // re-download everything when the user presses the refresh all button
        constexpr bool downloadMissingOnly = false;
        bool refreshResult = PythonBindingsInterface::Get()->RefreshAllGemRepos(downloadMissingOnly);
        Reinit();
        emit NotifyRemoteContentRefreshed();

        if (refreshResult)
        {
            ShowStandardToastNotification(tr("Repos updated"));
        }
        else
        {
            QMessageBox::critical(
                this, tr("Operation failed"), QString("Some repos failed to refresh."));
        }
    }

    void GemRepoScreen::HandleRefreshRepoButton(const QModelIndex& modelIndex)
    {
        const QString repoUri = m_gemRepoModel->GetRepoUri(modelIndex);
        const QString repoName = m_gemRepoModel->GetName(modelIndex);

        // re-download everything when the user presses the refresh all button
        constexpr bool downloadMissingOnly = false;
        AZ::Outcome<void, AZStd::string> refreshResult = PythonBindingsInterface::Get()->RefreshGemRepo(repoUri, downloadMissingOnly);
        if (refreshResult.IsSuccess())
        {
            Reinit();
            emit NotifyRemoteContentRefreshed();

            ShowStandardToastNotification(tr("%1 updated").arg(repoName));
        }
        else
        {
            QMessageBox::critical(
                this, tr("Operation failed"),
                QString("Failed to refresh gem repo %1<br>Error:<br>%2")
                    .arg(m_gemRepoModel->GetName(modelIndex), refreshResult.GetError().c_str()));
        }
    }

    void GemRepoScreen::FillModel()
    {
        AZ::Outcome<QVector<GemRepoInfo>, AZStd::string> allGemRepoInfosResult = PythonBindingsInterface::Get()->GetAllGemRepoInfos();
        if (allGemRepoInfosResult.IsSuccess())
        {
            // Add all available repos to the model
            const QVector<GemRepoInfo> allGemRepoInfos = allGemRepoInfosResult.GetValue();
            QDateTime oldestRepoUpdate;
            if (!allGemRepoInfos.isEmpty())
            {
                oldestRepoUpdate = allGemRepoInfos[0].m_lastUpdated;
            }
            for (const GemRepoInfo& gemRepoInfo : allGemRepoInfos)
            {
                m_gemRepoModel->AddGemRepo(gemRepoInfo);

                // Find least recently updated repo
                if (gemRepoInfo.m_lastUpdated < oldestRepoUpdate)
                {
                    oldestRepoUpdate = gemRepoInfo.m_lastUpdated;
                }
            }

            if (!allGemRepoInfos.isEmpty())
            {
                // get the month day and year in the preferred locale's format (QLocale defaults to the OS locale)
                QString monthDayYear = oldestRepoUpdate.toString(QLocale().dateFormat(QLocale::ShortFormat));

                // always show 12 hour + minutes + am/pm
                QString hourMinuteAMPM = oldestRepoUpdate.toString("h:mmap");

                QString repoUpdatedDate = QString("%1 %2").arg(monthDayYear, hourMinuteAMPM);

                m_lastAllUpdateLabel->setText(tr("Last Updated: %1").arg(repoUpdatedDate));
            }
            else
            {
                m_lastAllUpdateLabel->setText(tr("Last Updated: Never"));
            }

            m_sortProxyModel->sort(/*column*/0);
        }
        else
        {
            QMessageBox::critical(this, tr("Operation failed"), QString("Cannot retrieve gem repos for engine.<br>Error:<br>%2").arg(allGemRepoInfosResult.GetError().c_str()));
        }
    }

    void GemRepoScreen::OnModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
    {
        if (roles.isEmpty() || roles.at(0) == GemRepoModel::UserRole::RoleIsEnabled)
        {
            QItemSelection updatedItems(topLeft, bottomRight);
            for (const QModelIndex& modelIndex : updatedItems.indexes())
            {
                const bool isEnabled = GemRepoModel::IsEnabled(modelIndex);
                QString repoUri = GemRepoModel::GetRepoUri(modelIndex);
                PythonBindingsInterface::Get()->SetRepoEnabled(repoUri, isEnabled);

                const QString repoName = m_gemRepoModel->GetName(modelIndex);
                if (isEnabled)
                {
                    ShowStandardToastNotification(tr("%1 activated").arg(repoName));
                }
                else
                {
                    ShowStandardToastNotification(tr("%1 deactivated").arg(repoName));
                }
            }
        }
    }

    QFrame* GemRepoScreen::CreateNoReposContent()
    {
        QFrame* contentFrame = new QFrame(this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignHCenter);
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        contentFrame->setLayout(vLayout);

        vLayout->addStretch();

        QLabel* noRepoLabel = new QLabel(tr("No repositories have been added yet."), this);
        noRepoLabel->setObjectName("gemRepoNoReposLabel");
        vLayout->addWidget(noRepoLabel);
        vLayout->setAlignment(noRepoLabel, Qt::AlignHCenter);

        vLayout->addSpacing(20);

        // Size hint for button is wrong so horizontal layout with stretch is used to center it
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        hLayout->setSpacing(0);

        hLayout->addStretch();

        QPushButton* addRepoButton = new QPushButton(tr("Add Repository"), this);
        addRepoButton->setObjectName("gemRepoAddButton");
        addRepoButton->setProperty("secondary", true);
        addRepoButton->setMinimumWidth(120);
        hLayout->addWidget(addRepoButton);

        connect(addRepoButton, &QPushButton::clicked, this, &GemRepoScreen::HandleAddRepoButton);

        hLayout->addStretch();

        vLayout->addLayout(hLayout);

        vLayout->addStretch();

        return contentFrame;
    }

    QFrame* GemRepoScreen::CreateReposContent()
    {
        constexpr int inspectorWidth = 240;
        constexpr int middleLayoutIndent = 60;

        QFrame* contentFrame = new QFrame(this);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        hLayout->setSpacing(0);
        contentFrame->setLayout(hLayout);

        hLayout->addSpacing(middleLayoutIndent);

        QVBoxLayout* middleVLayout = new QVBoxLayout();
        middleVLayout->setMargin(0);
        middleVLayout->setSpacing(0);

        middleVLayout->addSpacing(30);

        QHBoxLayout* topMiddleHLayout = new QHBoxLayout();
        topMiddleHLayout->setMargin(0);
        topMiddleHLayout->setSpacing(0);

        m_lastAllUpdateLabel = new QLabel(tr("Last Updated: Never"), this);
        m_lastAllUpdateLabel->setObjectName("gemRepoHeaderLabel");
        topMiddleHLayout->addWidget(m_lastAllUpdateLabel);

        topMiddleHLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

        QPushButton* updateAllButton = new QPushButton(QIcon(":/Refresh.svg").pixmap(16, 16), tr("Update All"), this);
        updateAllButton->setObjectName("gemRepoAddButton");
        updateAllButton->setProperty("secondary", true);
        topMiddleHLayout->addWidget(updateAllButton);
        connect(updateAllButton, &QPushButton::clicked, this, &GemRepoScreen::HandleRefreshAllButton);

        topMiddleHLayout->addSpacing(10);

        QPushButton* addRepoButton = new QPushButton(tr("Add Repository"), this);
        addRepoButton->setObjectName("gemRepoAddButton");
        addRepoButton->setProperty("secondary", true);
        topMiddleHLayout->addWidget(addRepoButton);

        connect(addRepoButton, &QPushButton::clicked, this, &GemRepoScreen::HandleAddRepoButton);

        middleVLayout->addLayout(topMiddleHLayout);

        middleVLayout->addSpacing(30);

        constexpr int minHeaderSectionWidth = 80;

        m_gemRepoHeaderTable = new AdjustableHeaderWidget(
            QStringList{ tr("Repository Name"), tr("Creator"), "", tr("Updated Date"), tr("Status") },
            QVector<int>{
                GemRepoItemDelegate::s_nameDefaultWidth + GemRepoItemDelegate::s_contentMargins.left(),
                GemRepoItemDelegate::s_creatorDefaultWidth,
                GemRepoItemDelegate::s_badgeDefaultWidth,
                GemRepoItemDelegate::s_updatedDefaultWidth,
                // Include invisible header for delete button 
                GemRepoItemDelegate::s_buttonsDefaultWidth + GemRepoItemDelegate::s_contentMargins.right()
            },
            minHeaderSectionWidth,
            QVector<QHeaderView::ResizeMode>
            {
                QHeaderView::ResizeMode::Interactive,
                QHeaderView::ResizeMode::Stretch,
                QHeaderView::ResizeMode::Fixed,
                QHeaderView::ResizeMode::Fixed,
                QHeaderView::ResizeMode::Fixed
            },
            this);

        middleVLayout->addWidget(m_gemRepoHeaderTable);

        m_sortProxyModel = new GemRepoProxyModel(this);
        m_sortProxyModel->setSourceModel(m_gemRepoModel);
        m_sortProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        m_sortProxyModel->setSortRole(GemRepoModel::UserRole::RoleName);

        m_selectionModel = new QItemSelectionModel(m_sortProxyModel, this);
        m_gemRepoListView = new GemRepoListView(m_sortProxyModel, m_selectionModel, m_gemRepoHeaderTable, this);
        connect(m_gemRepoListView, &GemRepoListView::RefreshRepo, this, &GemRepoScreen::HandleRefreshRepoButton);
        middleVLayout->addWidget(m_gemRepoListView);

        hLayout->addLayout(middleVLayout);
        hLayout->addSpacing(middleLayoutIndent);

        m_gemRepoInspector = new GemRepoInspector(m_gemRepoModel, m_selectionModel, this);
        connect(m_gemRepoInspector, &GemRepoInspector::RemoveRepo, this, &GemRepoScreen::HandleRemoveRepoButton);
        connect(m_gemRepoInspector, &GemRepoInspector::ShowToastNotification, this, &GemRepoScreen::ShowStandardToastNotification);
        m_gemRepoInspector->setFixedWidth(inspectorWidth);
        hLayout->addWidget(m_gemRepoInspector);

        return contentFrame;
    }

    void GemRepoScreen::ShowStandardToastNotification(const QString& notification)
    {
        AzQtComponents::ToastConfiguration toastConfiguration(AzQtComponents::ToastType::Custom, notification, "");
        toastConfiguration.m_customIconImage = ":/Info.svg";
        toastConfiguration.m_borderRadius = 4;
        toastConfiguration.m_duration = AZStd::chrono::milliseconds(3000);
        m_notificationsView->ShowToastNotification(toastConfiguration);
    }

    ProjectManagerScreen GemRepoScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemRepos;
    }
} // namespace O3DE::ProjectManager
