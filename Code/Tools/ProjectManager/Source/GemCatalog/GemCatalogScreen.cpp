/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemCatalogScreen.h>
#include <PythonBindingsInterface.h>
#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <GemCatalog/GemFilterWidget.h>
#include <GemCatalog/GemListView.h>
#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemListHeaderWidget.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <GemCatalog/GemUpdateDialog.h>
#include <GemCatalog/GemUninstallDialog.h>
#include <GemCatalog/GemItemDelegate.h>
#include <DownloadController.h>
#include <ProjectUtils.h>
#include <AdjustableHeaderWidget.h>
#include <ScreensCtrl.h>
#include <CreateAGemScreen.h>
#include <EditAGemScreen.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <PythonBindingsInterface.h>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QHash>
#include <QStackedWidget>

namespace O3DE::ProjectManager
{
    GemCatalogScreen::GemCatalogScreen(DownloadController* downloadController, bool readOnly, QWidget* parent)
        : ScreenWidget(parent)
        , m_readOnly(readOnly)
        , m_downloadController(downloadController)
    {
        // The width of either side panel (filters, inspector) in the catalog
        constexpr int sidePanelWidth = 240;
        // Querying qApp about styling reports the scroll bar being larger than it is so define it manually
        constexpr int verticalScrollBarWidth = 8;

        setObjectName("GemCatalogScreen");

        m_gemModel = new GemModel(this);
        m_proxyModel = new GemSortFilterProxyModel(m_gemModel, this);

        // default to sort by gem name
        m_proxyModel->setSortRole(GemModel::RoleName);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        m_headerWidget = new GemCatalogHeaderWidget(m_gemModel, m_proxyModel, m_downloadController);
        vLayout->addWidget(m_headerWidget);

        connect(m_gemModel, &GemModel::gemStatusChanged, this, &GemCatalogScreen::OnGemStatusChanged);
        connect(m_gemModel, &GemModel::dependencyGemStatusChanged, this, &GemCatalogScreen::OnDependencyGemStatusChanged);
        connect(m_gemModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, [this]{ ShowInspector(); });
        connect(m_headerWidget, &GemCatalogHeaderWidget::RefreshGems, this, &GemCatalogScreen::Refresh);
        connect(m_headerWidget, &GemCatalogHeaderWidget::OpenGemsRepo, this, &GemCatalogScreen::HandleOpenGemRepo);
        connect(m_headerWidget, &GemCatalogHeaderWidget::CreateGem, this, &GemCatalogScreen::HandleCreateGem);
        connect(m_headerWidget, &GemCatalogHeaderWidget::AddGem, this, &GemCatalogScreen::OnAddGemClicked);
        connect(m_headerWidget, &GemCatalogHeaderWidget::UpdateGemCart, this, &GemCatalogScreen::UpdateAndShowGemCart);
        connect(m_downloadController, &DownloadController::Done, this, &GemCatalogScreen::OnGemDownloadResult);

        m_screensControl = qobject_cast<ScreensCtrl*>(parent);
        if (m_screensControl)
        {
            ScreenWidget* createGemScreen = m_screensControl->FindScreen(ProjectManagerScreen::CreateGem);
            if (createGemScreen)
            {
                CreateGem* createGem = static_cast<CreateGem*>(createGemScreen);
                connect(createGem, &CreateGem::GemCreated, this, &GemCatalogScreen::HandleGemCreated);
            }

            ScreenWidget* editGemScreen = m_screensControl->FindScreen(ProjectManagerScreen::EditGem);
            if (editGemScreen)
            {
                EditGem* editGem = static_cast<EditGem*>(editGemScreen);
                connect(editGem, &EditGem::GemEdited, this, &GemCatalogScreen::HandleGemEdited);
            }
        }

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        vLayout->addLayout(hLayout);

        m_rightPanelStack = new QStackedWidget(this);
        m_rightPanelStack->setFixedWidth(sidePanelWidth);

        m_gemInspector = new GemInspector(m_gemModel, m_rightPanelStack);

        connect(m_gemInspector, &GemInspector::TagClicked, [=](const Tag& tag) { SelectGem(tag.id); });
        connect(m_gemInspector, &GemInspector::UpdateGem, this, &GemCatalogScreen::UpdateGem);
        connect(m_gemInspector, &GemInspector::UninstallGem, this, &GemCatalogScreen::UninstallGem);
        connect(m_gemInspector, &GemInspector::EditGem, this, &GemCatalogScreen::HandleEditGem);

        QWidget* filterWidget = new QWidget(this);
        filterWidget->setFixedWidth(sidePanelWidth);
        m_filterWidgetLayout = new QVBoxLayout();
        m_filterWidgetLayout->setMargin(0);
        m_filterWidgetLayout->setSpacing(0);
        filterWidget->setLayout(m_filterWidgetLayout);

        GemListHeaderWidget* catalogHeaderWidget = new GemListHeaderWidget(m_proxyModel);
        connect(catalogHeaderWidget, &GemListHeaderWidget::OnRefresh, this, &GemCatalogScreen::Refresh);

        constexpr int minHeaderSectionWidth = 100;
        AdjustableHeaderWidget* listHeaderWidget = new AdjustableHeaderWidget(
            QStringList{ tr("Gem Image"), tr("Gem Name"), tr("Gem Summary"), tr("Status") },
            QVector<int>{
                GemPreviewImageWidth + AdjustableHeaderWidget::s_headerTextIndent,
                -GemPreviewImageWidth - AdjustableHeaderWidget::s_headerTextIndent + GemItemDelegate::s_defaultSummaryStartX - 30,
                0, // Section is set to stretch to fit
                GemItemDelegate::s_statusIconSize + GemItemDelegate::s_statusButtonSpacing + GemItemDelegate::s_buttonWidth + GemItemDelegate::s_contentMargins.right()
            },
            minHeaderSectionWidth,
            QVector<QHeaderView::ResizeMode>
            {
                QHeaderView::ResizeMode::Fixed,
                QHeaderView::ResizeMode::Interactive,
                QHeaderView::ResizeMode::Stretch,
                QHeaderView::ResizeMode::Fixed
            },
            this);

        m_gemListView = new GemListView(m_proxyModel, m_proxyModel->GetSelectionModel(), listHeaderWidget, m_readOnly, this);

        QHBoxLayout* listHeaderLayout = new QHBoxLayout();
        listHeaderLayout->setMargin(0);
        listHeaderLayout->setSpacing(0);
        listHeaderLayout->addSpacing(GemItemDelegate::s_itemMargins.left());
        listHeaderLayout->addWidget(listHeaderWidget);
        listHeaderLayout->addSpacing(GemItemDelegate::s_itemMargins.right() + verticalScrollBarWidth);

        QVBoxLayout* middleVLayout = new QVBoxLayout();
        middleVLayout->setMargin(0);
        middleVLayout->setSpacing(0);
        middleVLayout->addWidget(catalogHeaderWidget);
        middleVLayout->addLayout(listHeaderLayout);
        middleVLayout->addWidget(m_gemListView);

        hLayout->addWidget(filterWidget);
        hLayout->addLayout(middleVLayout);

        hLayout->addWidget(m_rightPanelStack);
        m_rightPanelStack->addWidget(m_gemInspector);

        m_notificationsView = AZStd::make_unique<AzToolsFramework::ToastNotificationsView>(this, AZ_CRC("GemCatalogNotificationsView"));
        m_notificationsView->SetOffset(QPoint(10, 70));
        m_notificationsView->SetMaxQueuedNotifications(1);
    }

    void GemCatalogScreen::NotifyCurrentScreen()
    {
        if (m_readOnly && m_gemModel->rowCount() == 0)
        {
            // init the read only catalog the first time it is shown
            ReinitForProject(m_projectPath);
        }
    }

    void GemCatalogScreen::ReinitForProject(const QString& projectPath)
    {
        // avoid slow rebuilding, user can manually refresh if needed
        if (m_gemModel->rowCount() > 0 && QDir(projectPath) == QDir(m_projectPath))
        {
            return;
        }

        m_projectPath = projectPath;
        m_gemModel->Clear();
        m_gemsToRegisterWithProject.clear();

        if (m_filterWidget)
        {
            // disconnect so we don't update the status filter for every gem we add
            disconnect(m_gemModel, &GemModel::dataChanged, m_filterWidget, &GemFilterWidget::ResetGemStatusFilter);
        }

        FillModel(projectPath);

        m_proxyModel->ResetFilters(false);
        m_proxyModel->sort(/*column=*/0);

        if (m_filterWidget)
        {
            m_filterWidget->ResetAllFilters();
        }
        else
        {
            m_filterWidget = new GemFilterWidget(m_proxyModel);
            m_filterWidgetLayout->addWidget(m_filterWidget);
        }

        m_headerWidget->ReinitForProject();

        connect(m_gemModel, &GemModel::dataChanged, m_filterWidget, &GemFilterWidget::ResetGemStatusFilter);

        // Select the first entry after everything got correctly sized
        QTimer::singleShot(200, [=]{
            QModelIndex firstModelIndex = m_gemModel->index(0, 0);
            QModelIndex proxyIndex = m_proxyModel->mapFromSource(firstModelIndex);
            m_proxyModel->GetSelectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);
        });
    }

    void GemCatalogScreen::OnAddGemClicked()
    {
        EngineInfo engineInfo;
        QString defaultPath;

        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            engineInfo = engineInfoResult.GetValue();
            defaultPath = engineInfo.m_defaultGemsFolder;
        }

        if (defaultPath.isEmpty())
        {
            defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }

        QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Browse"), defaultPath));
        if (!directory.isEmpty())
        {
            // register the gem to the o3de_manifest.json and to the project after the user confirms
            // project creation/update
            auto registerResult = PythonBindingsInterface::Get()->RegisterGem(directory);
            if(!registerResult)
            {
                QMessageBox::critical(this, tr("Failed to add gem"), registerResult.GetError().c_str());
            }
            else
            {
                m_gemsToRegisterWithProject.insert(directory);
                AZ::Outcome<GemInfo> gemInfoResult = PythonBindingsInterface::Get()->GetGemInfo(directory);
                if (gemInfoResult)
                {
                    // We added this local gem so set it's status to downloaded
                    GemInfo& addedGemInfo = gemInfoResult.GetValue<GemInfo>();
                    addedGemInfo.m_downloadStatus = GemInfo::DownloadStatus::Downloaded;
                    AddToGemModel(addedGemInfo);
                }
            }
        }
    }

    void GemCatalogScreen::AddToGemModel(const GemInfo& gemInfo)
    {
        m_gemModel->AddGem(gemInfo);
        m_gemModel->UpdateGemDependencies();
        m_proxyModel->sort(/*column=*/0);
    }

    void GemCatalogScreen::Refresh()
    {
        QHash<QString, GemInfo> gemInfoHash;

        // create a hash with the gem name as key
        const AZ::Outcome<QVector<GemInfo>, AZStd::string>& allGemInfosResult = PythonBindingsInterface::Get()->GetAllGemInfos(m_projectPath);
        if (allGemInfosResult.IsSuccess())
        {
            const QVector<GemInfo>& gemInfos = allGemInfosResult.GetValue();
            for (const GemInfo& gemInfo : gemInfos)
            {
                // ${Name} is a special name used for templates and should be ignored
                // eventually we should handle this in Python instead of here
                if (gemInfo.m_name != "${Name}")
                {
                    gemInfoHash.insert(gemInfo.m_name, gemInfo);
                }
            }
        }

        // add all the gem repos into the hash
        const AZ::Outcome<QVector<GemInfo>, AZStd::string>& allRepoGemInfosResult = PythonBindingsInterface::Get()->GetGemInfosForAllRepos();
        if (allRepoGemInfosResult.IsSuccess())
        {
            const QVector<GemInfo>& allRepoGemInfos = allRepoGemInfosResult.GetValue();
            for (const GemInfo& gemInfo : allRepoGemInfos)
            {
                if (!gemInfoHash.contains(gemInfo.m_name))
                {
                    gemInfoHash.insert(gemInfo.m_name, gemInfo);
                }
            }
        }

        // remove gems from the model that no longer exist in the hash and are not project dependencies
        int i = 0;
        while (i < m_gemModel->rowCount())
        {
            QModelIndex index = m_gemModel->index(i,0);
            QString gemName = m_gemModel->GetName(index);
            const bool gemFound = gemInfoHash.contains(gemName);
            if (!gemFound && !m_gemModel->IsAdded(index) && !m_gemModel->IsAddedDependency(index))
            {
                m_gemModel->RemoveGem(index);
            }
            else
            {
                if (!gemFound && (m_gemModel->IsAdded(index) || m_gemModel->IsAddedDependency(index)))
                {
                    const QString error = tr("Gem %1 was removed or unregistered, but is still used by the project.").arg(gemName);
                    AZ_Warning("Project Manager", false, error.toUtf8().constData());
                    QMessageBox::warning(this, tr("Gem not found"), error.toUtf8().constData());
                }

                gemInfoHash.remove(gemName);
                i++;
            }
        }

        // add all gems remaining in the hash that were not removed
        for(auto iter = gemInfoHash.begin(); iter != gemInfoHash.end(); ++iter)
        {
            m_gemModel->AddGem(iter.value());
        }

        m_gemModel->UpdateGemDependencies();
        m_proxyModel->sort(/*column=*/0);

        // temporary, until we can refresh filter counts 
        m_proxyModel->ResetFilters(false);
        m_filterWidget->ResetAllFilters();

        // Reselect the same selection to proc UI updates
        m_proxyModel->GetSelectionModel()->setCurrentIndex(m_proxyModel->GetSelectionModel()->currentIndex(), QItemSelectionModel::Select);
    }

    void GemCatalogScreen::OnGemStatusChanged(const QString& gemName, uint32_t numChangedDependencies) 
    {
        if (m_notificationsEnabled)
        {
            QModelIndex modelIndex = m_gemModel->FindIndexByNameString(gemName);
            bool added = GemModel::IsAdded(modelIndex);
            bool dependency = GemModel::IsAddedDependency(modelIndex);

            bool gemStateChanged = (added && !dependency) || (!added && !dependency);
            if (!gemStateChanged && !numChangedDependencies)
            {
                // no actual changes made
                return;
            }

            QString notification;
            if (gemStateChanged)
            {
                notification = GemModel::GetDisplayName(modelIndex);
                if (numChangedDependencies > 0)
                {
                    notification += tr(" and ");
                }
                if (added && (GemModel::GetDownloadStatus(modelIndex) == GemInfo::DownloadStatus::NotDownloaded) ||
                    (GemModel::GetDownloadStatus(modelIndex) == GemInfo::DownloadStatus::DownloadFailed))
                {
                    m_downloadController->AddObjectDownload(GemModel::GetName(modelIndex), "", DownloadController::DownloadObjectType::Gem);
                    GemModel::SetDownloadStatus(*m_gemModel, modelIndex, GemInfo::DownloadStatus::Downloading);
                }
            }

            if (numChangedDependencies == 1)
            {
                notification += tr("1 Gem dependency");
            }
            else if (numChangedDependencies > 1)
            {
                notification += tr("%1 Gem %2").arg(numChangedDependencies).arg(tr("dependencies"));
            }
            notification += (added ? tr(" activated") : tr(" deactivated"));

            ShowStandardToastNotification(notification);
        }
    }

    void GemCatalogScreen::ShowStandardToastNotification(const QString& notification)
    {
        AzQtComponents::ToastConfiguration toastConfiguration(AzQtComponents::ToastType::Custom, notification, "");
        toastConfiguration.m_customIconImage = ":/gem.svg";
        toastConfiguration.m_borderRadius = 4;
        toastConfiguration.m_duration = AZStd::chrono::milliseconds(3000);
        m_notificationsView->ShowToastNotification(toastConfiguration);
    }
    

    void GemCatalogScreen::OnDependencyGemStatusChanged(const QString& gemName)
    {
        QModelIndex modelIndex = m_gemModel->FindIndexByNameString(gemName);
        bool added = GemModel::IsAddedDependency(modelIndex);
        if (added && (GemModel::GetDownloadStatus(modelIndex) == GemInfo::DownloadStatus::NotDownloaded) ||
            (GemModel::GetDownloadStatus(modelIndex) == GemInfo::DownloadStatus::DownloadFailed))
        {
            m_downloadController->AddObjectDownload(GemModel::GetName(modelIndex), "" , DownloadController::DownloadObjectType::Gem);
            GemModel::SetDownloadStatus(*m_gemModel, modelIndex, GemInfo::DownloadStatus::Downloading);
        }
    }

    void GemCatalogScreen::SelectGem(const QString& gemName)
    {
        QModelIndex modelIndex = m_gemModel->FindIndexByNameString(gemName);
        if (!m_proxyModel->filterAcceptsRow(modelIndex.row(), QModelIndex()))
        {
            m_proxyModel->ResetFilters();
            m_filterWidget->ResetAllFilters();
        }

        QModelIndex proxyIndex = m_proxyModel->mapFromSource(modelIndex);
        m_proxyModel->GetSelectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);
        m_gemListView->scrollTo(proxyIndex);

        ShowInspector();
    }

    void GemCatalogScreen::UpdateGem(const QModelIndex& modelIndex)
    {
        const QString selectedGemName = m_gemModel->GetName(modelIndex);
        const QString selectedGemLastUpdate = m_gemModel->GetLastUpdated(modelIndex);
        const QString selectedDisplayGemName = m_gemModel->GetDisplayName(modelIndex);
        const QString selectedGemRepoUri = m_gemModel->GetRepoUri(modelIndex);

        // Refresh gem repo
        if (!selectedGemRepoUri.isEmpty())
        {
            AZ::Outcome<void, AZStd::string> refreshResult = PythonBindingsInterface::Get()->RefreshGemRepo(selectedGemRepoUri);
            if (refreshResult.IsSuccess())
            {
                Refresh();
            }
            else
            {
                QMessageBox::critical(
                    this, tr("Operation failed"),
                    tr("Failed to refresh gem repository %1<br>Error:<br>%2").arg(selectedGemRepoUri, refreshResult.GetError().c_str()));
            }
        }
        // If repo uri isn't specified warn user that repo might not be refreshed
        else
        {
            int result = QMessageBox::warning(
                this, tr("Gem Repository Unspecified"),
                tr("The repo for %1 is unspecfied. Repository cannot be automatically refreshed. "
                   "Please ensure this gem's repo is refreshed before attempting to update.")
                    .arg(selectedDisplayGemName),
                QMessageBox::Cancel, QMessageBox::Ok);

            // Allow user to cancel update to manually refresh repo
            if (result != QMessageBox::Ok)
            {
                return;
            }
        }

        // Check if there is an update avaliable now that repo is refreshed
        bool updateAvaliable = PythonBindingsInterface::Get()->IsGemUpdateAvaliable(selectedGemName, selectedGemLastUpdate);

        GemUpdateDialog* confirmUpdateDialog = new GemUpdateDialog(selectedGemName, updateAvaliable, this);
        if (confirmUpdateDialog->exec() == QDialog::Accepted)
        {
            m_downloadController->AddObjectDownload(selectedGemName, "" , DownloadController::DownloadObjectType::Gem);
        }
    }

    void GemCatalogScreen::UninstallGem(const QModelIndex& modelIndex)
    {
        const QString selectedDisplayGemName = m_gemModel->GetDisplayName(modelIndex);

        GemUninstallDialog* confirmUninstallDialog = new GemUninstallDialog(selectedDisplayGemName, this);
        if (confirmUninstallDialog->exec() == QDialog::Accepted)
        {
            const QString selectedGemPath = m_gemModel->GetPath(modelIndex);

            const bool wasAdded = GemModel::WasPreviouslyAdded(modelIndex);
            const bool wasAddedDependency = GemModel::WasPreviouslyAddedDependency(modelIndex);

            // Remove gem from gems to be added to update any dependencies
            GemModel::SetIsAdded(*m_gemModel, modelIndex, false);
            GemModel::DeactivateDependentGems(*m_gemModel, modelIndex);

            // Unregister the gem
            auto unregisterResult = PythonBindingsInterface::Get()->UnregisterGem(selectedGemPath);
            if (!unregisterResult)
            {
                QMessageBox::critical(this, tr("Failed to unregister gem"), unregisterResult.GetError().c_str());
            }
            else
            {
                const QString selectedGemName = m_gemModel->GetName(modelIndex);

                // Remove gem from model
                m_gemModel->RemoveGem(modelIndex);

                // Delete uninstalled gem directory
                if (!ProjectUtils::DeleteProjectFiles(selectedGemPath, /*force*/true))
                {
                    QMessageBox::critical(
                        this, tr("Failed to remove gem directory"), tr("Could not delete gem directory at:<br>%1").arg(selectedGemPath));
                }

                // Show undownloaded remote gem again
                Refresh();

                // Select remote gem
                QModelIndex remoteGemIndex = m_gemModel->FindIndexByNameString(selectedGemName);
                GemModel::SetWasPreviouslyAdded(*m_gemModel, remoteGemIndex, wasAdded);
                GemModel::SetWasPreviouslyAddedDependency(*m_gemModel, remoteGemIndex, wasAddedDependency);
                QModelIndex proxyIndex = m_proxyModel->mapFromSource(remoteGemIndex);
                m_proxyModel->GetSelectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);
            }
        }
    }

    void GemCatalogScreen::hideEvent(QHideEvent* event)
    {
        ScreenWidget::hideEvent(event);
        m_notificationsView->OnHide();
    }

    void GemCatalogScreen::showEvent(QShowEvent* event)
    {
        ScreenWidget::showEvent(event);
        m_notificationsView->OnShow();
    }

    void GemCatalogScreen::resizeEvent(QResizeEvent* event)
    {
        ScreenWidget::resizeEvent(event);
        m_notificationsView->UpdateToastPosition();
    }

    void GemCatalogScreen::moveEvent(QMoveEvent* event)
    {
        ScreenWidget::moveEvent(event);
        m_notificationsView->UpdateToastPosition();
    }

    void GemCatalogScreen::FillModel(const QString& projectPath)
    {
        m_projectPath = projectPath;

        const AZ::Outcome<QVector<GemInfo>, AZStd::string>& allGemInfosResult = PythonBindingsInterface::Get()->GetAllGemInfos(projectPath);
        if (allGemInfosResult.IsSuccess())
        {
            // Add all available gems to the model.
            const QVector<GemInfo>& allGemInfos = allGemInfosResult.GetValue();
            for (const GemInfo& gemInfo : allGemInfos)
            {
                if (gemInfo.m_name != "${Name}")
                {
                    m_gemModel->AddGem(gemInfo);
                }
            }

            const AZ::Outcome<QVector<GemInfo>, AZStd::string>& allRepoGemInfosResult = PythonBindingsInterface::Get()->GetGemInfosForAllRepos();
            if (allRepoGemInfosResult.IsSuccess())
            {
                const QVector<GemInfo>& allRepoGemInfos = allRepoGemInfosResult.GetValue();
                for (const GemInfo& gemInfo : allRepoGemInfos)
                {
                    // do not add gems that have already been downloaded
                    if (!m_gemModel->FindIndexByNameString(gemInfo.m_name).isValid())
                    {
                        m_gemModel->AddGem(gemInfo);
                    }
                }
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Operation failed"), QString("Cannot retrieve gems from repos.<br><br>Error:<br>%1").arg(allRepoGemInfosResult.GetError().c_str()));
            }

            m_gemModel->UpdateGemDependencies();

            // if we don't have a project path early out
            if (m_projectPath.isEmpty())
            {
                return;
            }

            m_notificationsEnabled = false;

            // Gather enabled gems for the given project.
            const auto& enabledGemNamesResult = PythonBindingsInterface::Get()->GetEnabledGemNames(projectPath);
            if (enabledGemNamesResult.IsSuccess())
            {
                const QVector<AZStd::string>& enabledGemNames = enabledGemNamesResult.GetValue();
                for (const AZStd::string& enabledGemName : enabledGemNames)
                {
                    const QModelIndex modelIndex = m_gemModel->FindIndexByNameString(enabledGemName.c_str());
                    if (modelIndex.isValid())
                    {
                        GemModel::SetWasPreviouslyAdded(*m_gemModel, modelIndex, true);
                        GemModel::SetIsAdded(*m_gemModel, modelIndex, true);
                    }
                    // ${Name} is a special name used in templates and is not really an error
                    else if (enabledGemName != "${Name}")
                    {
                        AZ_Warning("ProjectManager::GemCatalog", false,
                            "Cannot find entry for gem with name '%s'. The CMake target name probably does not match the specified name in the gem.json.",
                            enabledGemName.c_str());
                    }
                }
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Operation failed"), QString("Cannot retrieve enabled gems for project %1.<br><br>Error:<br>%2").arg(projectPath, enabledGemNamesResult.GetError().c_str()));
            }

            m_notificationsEnabled = true;
        }
        else
        {
            QMessageBox::critical(nullptr, tr("Operation failed"), QString("Cannot retrieve gems for %1.<br><br>Error:<br>%2").arg(projectPath, allGemInfosResult.GetError().c_str()));
        }
    }

    void GemCatalogScreen::ShowInspector()
    {
        m_rightPanelStack->setCurrentIndex(RightPanelWidgetOrder::Inspector);
        m_headerWidget->GemCartShown();
    }

    void GemCatalogScreen::HandleOpenGemRepo()
    {
        if (m_readOnly)
        {
            emit ChangeScreenRequest(ProjectManagerScreen::GemsGemRepos);
        }
        else
        {
            emit ChangeScreenRequest(ProjectManagerScreen::GemRepos);
        }
    }

    void GemCatalogScreen::HandleCreateGem()
    {
        emit ChangeScreenRequest(ProjectManagerScreen::CreateGem);
    }

    void GemCatalogScreen::HandleEditGem(const QModelIndex& currentModelIndex)
    {
        if (m_screensControl)
        {
            auto editGemScreen = qobject_cast<EditGem*>(m_screensControl->FindScreen(ProjectManagerScreen::EditGem));
            if (editGemScreen)
            {
                m_curEditedIndex = currentModelIndex;
                editGemScreen->ResetWorkflow(m_gemModel->GetGemInfo(currentModelIndex));
                emit ChangeScreenRequest(ProjectManagerScreen::EditGem);
            }
        }
    }

    void GemCatalogScreen::UpdateAndShowGemCart(QWidget* cartWidget)
    {
        QWidget* previousCart = m_rightPanelStack->widget(RightPanelWidgetOrder::Cart);
        if (previousCart)
        {
            m_rightPanelStack->removeWidget(previousCart);
        }

        m_rightPanelStack->insertWidget(RightPanelWidgetOrder::Cart, cartWidget);
        m_rightPanelStack->setCurrentIndex(RightPanelWidgetOrder::Cart);
    }

    void GemCatalogScreen::OnGemDownloadResult(const QString& gemName, bool succeeded)
    {
        if (succeeded)
        {
            // refresh the information for downloaded gems
            const AZ::Outcome<QVector<GemInfo>, AZStd::string>& allGemInfosResult =
                PythonBindingsInterface::Get()->GetAllGemInfos(m_projectPath);
            if (allGemInfosResult.IsSuccess())
            {
                // we should find the gem name now in all gem infos
                for (const GemInfo& gemInfo : allGemInfosResult.GetValue())
                {
                    if (gemInfo.m_name == gemName)
                    {
                        QModelIndex oldIndex = m_gemModel->FindIndexByNameString(gemName);
                        if (oldIndex.isValid())
                        {
                            // Check if old gem is selected
                            bool oldGemSelected = false;
                            if (m_gemModel->GetSelectionModel()->currentIndex() == oldIndex)
                            {
                                oldGemSelected = true;
                            }

                            // Remove old remote gem
                            m_gemModel->RemoveGem(oldIndex);

                            // Add new downloaded version of gem
                            QModelIndex newIndex = m_gemModel->AddGem(gemInfo);
                            GemModel::SetDownloadStatus(*m_gemModel, newIndex, GemInfo::DownloadSuccessful);
                            GemModel::SetIsAdded(*m_gemModel, newIndex, true);

                            // Select new version of gem if it was previously selected
                            if (oldGemSelected)
                            {
                                QModelIndex proxyIndex = m_proxyModel->mapFromSource(newIndex);
                                m_proxyModel->GetSelectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);
                            }
                        }

                        break;
                    }
                }
            }
        }
        else
        {
            QModelIndex index = m_gemModel->FindIndexByNameString(gemName);
            if (index.isValid())
            {
                GemModel::SetIsAdded(*m_gemModel, index, false);
                GemModel::DeactivateDependentGems(*m_gemModel, index);
                GemModel::SetDownloadStatus(*m_gemModel, index, GemInfo::DownloadFailed);
            }
        }
    }

    void GemCatalogScreen::HandleGemCreated(const GemInfo& gemInfo)
    {
        // This signal occurs only upon successful completion of creating a gem. As such, the gemInfo data is assumed to be valid.

        // make sure the project gem catalog model is updated
        AddToGemModel(gemInfo);

        // create Toast Notification for project gem catalog
        QString notification = tr("%1 has been created.").arg(gemInfo.m_displayName);
        ShowStandardToastNotification(notification);
    }

    void GemCatalogScreen::HandleGemEdited(const GemInfo& newGemInfo)
    {
        // This signal only occurs upon successful completion of editing a gem. As such, the gemInfo is assumed to be valid

        // Make sure to update the current model index in the gem catalog model.
        // The current edited index is only set by HandleEdit before editing a gem, and nowhere else.
        // As such, the index should be valid.
        m_gemModel->RemoveGem(m_curEditedIndex);
        m_gemModel->AddGem(newGemInfo);

        //gem inspector needs to have its selection updated to the newly added gem
        SelectGem(newGemInfo.m_name);

        QString notification = tr("%1 was edited.").arg(newGemInfo.m_displayName);
        ShowStandardToastNotification(notification);
    }

    ProjectManagerScreen GemCatalogScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemCatalog;
    }

    QString GemCatalogScreen::GetTabText()
    {
        return "Gems";
    }

    bool GemCatalogScreen::IsTab()
    {
        return true;
    }
} // namespace O3DE::ProjectManager
