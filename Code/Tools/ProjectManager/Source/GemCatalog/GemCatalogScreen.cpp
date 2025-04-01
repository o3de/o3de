/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemCatalogScreen.h>
#include <AzCore/Dependency/Dependency.h>
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
#include <GemRepo/GemRepoScreen.h>
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
#include <QFileInfo>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QHash>
#include <QSet>
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

        // default to sort by gem display name 
        m_proxyModel->setSortRole(GemModel::RoleDisplayName);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        m_headerWidget = new GemCatalogHeaderWidget(m_gemModel, m_proxyModel, m_downloadController);
        vLayout->addWidget(m_headerWidget);

        connect(m_gemModel, &GemModel::gemStatusChanged, this, &GemCatalogScreen::OnGemStatusChanged);
        connect(m_gemModel, &GemModel::dependencyGemStatusChanged, this, &GemCatalogScreen::OnDependencyGemStatusChanged);
        connect(m_gemModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &GemCatalogScreen::ShowInspector);
        connect(m_headerWidget, &GemCatalogHeaderWidget::RefreshGems, this, &GemCatalogScreen::Refresh);
        connect(m_headerWidget, &GemCatalogHeaderWidget::OpenGemsRepo, this, &GemCatalogScreen::HandleOpenGemRepo);
        connect(m_headerWidget, &GemCatalogHeaderWidget::CreateGem, this, &GemCatalogScreen::HandleCreateGem);
        connect(m_headerWidget, &GemCatalogHeaderWidget::AddGem, this, &GemCatalogScreen::OnAddGemClicked);
        connect(m_headerWidget, &GemCatalogHeaderWidget::UpdateGemCart, this, &GemCatalogScreen::UpdateAndShowGemCart);
        connect(m_downloadController, &DownloadController::Done, this, &GemCatalogScreen::OnGemDownloadResult);

        ScreensCtrl* screensCtrl = GetScreensCtrl(this);
        if (screensCtrl)
        {
            connect(screensCtrl, &ScreensCtrl::NotifyRemoteContentRefreshed, [this]() { m_needRefresh = true; });
        }

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        vLayout->addLayout(hLayout);

        m_rightPanelStack = new QStackedWidget(this);
        m_rightPanelStack->setFixedWidth(sidePanelWidth);

        m_gemInspector = new GemInspector(m_gemModel, m_rightPanelStack, m_readOnly);

        connect(
            m_gemInspector,
            &GemInspector::TagClicked,
            [=](const Tag& tag)
            {
                SelectGem(tag.id);
            });
        connect(m_gemInspector, &GemInspector::UpdateGem, this, &GemCatalogScreen::UpdateGem);
        connect(m_gemInspector, &GemInspector::UninstallGem, this, &GemCatalogScreen::UninstallGem);
        connect(m_gemInspector, &GemInspector::EditGem, this, &GemCatalogScreen::HandleEditGem);
        connect(m_gemInspector, &GemInspector::DownloadGem, this, &GemCatalogScreen::DownloadGem);
        connect(m_gemInspector, &GemInspector::ShowToastNotification, this, &GemCatalogScreen::ShowStandardToastNotification);

        QWidget* filterWidget = new QWidget(this);
        filterWidget->setFixedWidth(sidePanelWidth);
        m_filterWidgetLayout = new QVBoxLayout();
        m_filterWidgetLayout->setMargin(0);
        m_filterWidgetLayout->setSpacing(0);
        filterWidget->setLayout(m_filterWidgetLayout);

        m_filterWidget = new GemFilterWidget(m_proxyModel);
        m_filterWidgetLayout->addWidget(m_filterWidget);

        GemListHeaderWidget* catalogHeaderWidget = new GemListHeaderWidget(m_proxyModel);
        connect(catalogHeaderWidget, &GemListHeaderWidget::OnRefresh, this, &GemCatalogScreen::Refresh);

        constexpr int previewWidth = GemItemDelegate::s_itemMargins.left() + GemPreviewImageWidth +
                                            AdjustableHeaderWidget::s_headerTextIndent;
        constexpr int versionWidth = GemItemDelegate::s_versionSize + GemItemDelegate::s_versionSizeSpacing;
        constexpr int statusWidth = GemItemDelegate::s_statusIconSize + GemItemDelegate::s_statusButtonSpacing +
                                             GemItemDelegate::s_buttonWidth + GemItemDelegate::s_contentMargins.right();
        constexpr int minHeaderSectionWidth = AZStd::min(previewWidth, AZStd::min(versionWidth, statusWidth));

        AdjustableHeaderWidget* listHeaderWidget = new AdjustableHeaderWidget(
            QStringList{ tr("Gem Image"), tr("Gem Name"), tr("Gem Summary"), tr("Latest Version"), tr("Status") },
            QVector<int>{ previewWidth,
                          GemItemDelegate::s_defaultSummaryStartX - previewWidth,
                          0, // Section is set to stretch to fit
                          versionWidth,
                          statusWidth},
            minHeaderSectionWidth,
            QVector<QHeaderView::ResizeMode>{ QHeaderView::ResizeMode::Fixed,
                                              QHeaderView::ResizeMode::Interactive,
                                              QHeaderView::ResizeMode::Stretch,
                                              QHeaderView::ResizeMode::Fixed,
                                              QHeaderView::ResizeMode::Fixed },
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

        m_notificationsView = AZStd::make_unique<AzToolsFramework::ToastNotificationsView>(this, AZ_CRC_CE("GemCatalogNotificationsView"));
        m_notificationsView->SetOffset(QPoint(10, 70));
        m_notificationsView->SetMaxQueuedNotifications(1);
        m_notificationsView->SetRejectDuplicates(false); // we want to show notifications if a user repeats actions
    }

    void GemCatalogScreen::SetUpScreensControl(QWidget* parent)
    {
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
    }

    void GemCatalogScreen::NotifyCurrentScreen()
    {
        if (m_readOnly && m_gemModel->rowCount() == 0)
        {
            // init the read only catalog the first time it is shown
            ReinitForProject(m_projectPath);
        }
        else if (m_needRefresh)
        {
            // generally we need to refresh because remote repos were updated
            m_needRefresh = false;
            Refresh();
        }
    }

    void GemCatalogScreen::NotifyProjectRemoved(const QString& projectPath)
    {
        // Use QFileInfo because the project path might be the project folder
        // or a remote project.json file in the cache
        if (QFileInfo(projectPath) == QFileInfo(m_projectPath))
        {
            m_projectPath = "";
            m_gemModel->Clear();
            m_gemsToRegisterWithProject.clear();
        }
    }

    void GemCatalogScreen::ReinitForProject(const QString& projectPath)
    {
        // Avoid slow rebuilding, user can manually refresh if needed
        // Use QFileInfo because the project path might be the project folder
        // or a remote project.json file in the cache
        if (m_gemModel->rowCount() > 0 && QFileInfo(projectPath) == QFileInfo(m_projectPath))
        {
            return;
        }

        m_projectPath = projectPath;

        m_gemModel->Clear();
        m_gemsToRegisterWithProject.clear();

        FillModel(projectPath);

        m_gemModel->UpdateGemDependencies();
        m_proxyModel->sort(/*column=*/0);
        m_proxyModel->ResetFilters();
        m_filterWidget->UpdateAllFilters(/*clearAllCheckboxes=*/true);

        m_headerWidget->ReinitForProject();

        QModelIndex firstProxyIndex = m_proxyModel->index(0, 0);
        m_proxyModel->GetSelectionModel()->setCurrentIndex(firstProxyIndex, QItemSelectionModel::ClearAndSelect);
        m_gemInspector->Update(m_proxyModel->mapToSource(firstProxyIndex));
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
                    ShowStandardToastNotification(tr("%1 added").arg(addedGemInfo.m_displayName));
                }
            }
        }
    }

    void GemCatalogScreen::AddToGemModel(const GemInfo& gemInfo)
    {
        const auto modelIndex = m_gemModel->AddGem(gemInfo);
        m_gemModel->UpdateGemDependencies();
        m_proxyModel->sort(/*column=*/0);
        m_proxyModel->InvalidateFilter();
        m_filterWidget->UpdateAllFilters(/*clearAllCheckboxes=*/false);

        // attempt to select the newly added gem
        if(auto proxyIndex = m_proxyModel->mapFromSource(modelIndex); proxyIndex.isValid())
        {
            m_proxyModel->GetSelectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);
        }
    }

    void GemCatalogScreen::Refresh(bool refreshRemoteRepos)
    {
        QSet<QPersistentModelIndex> validIndexes;

        if (const auto& outcome = PythonBindingsInterface::Get()->GetAllGemInfos(m_projectPath); outcome.IsSuccess())
        {
            const auto& indexes = m_gemModel->AddGems(outcome.GetValue(), /*updateExisting=*/true);
            validIndexes = QSet(indexes.cbegin(), indexes.cend());
        }

        if(refreshRemoteRepos)
        {
            PythonBindingsInterface::Get()->RefreshAllGemRepos();
        }

        if (const auto& outcome = PythonBindingsInterface::Get()->GetGemInfosForAllRepos(); outcome.IsSuccess())
        {
            const auto& indexes = m_gemModel->AddGems(outcome.GetValue(), /*updateExisting=*/true);
            validIndexes.unite(QSet(indexes.cbegin(), indexes.cend()));
        }

        // remove gems from the model that no longer exist in the hash and are not project dependencies
        int i = 0;
        while (i < m_gemModel->rowCount())
        {
            QModelIndex index = m_gemModel->index(i,0);
            const bool gemFound = validIndexes.contains(index);
            if (!gemFound && !m_gemModel->IsAdded(index) && !m_gemModel->IsAddedDependency(index))
            {
                m_gemModel->RemoveGem(index);
            }
            else
            {
                if (!gemFound && (m_gemModel->IsAdded(index) || m_gemModel->IsAddedDependency(index)))
                {
                    QString gemName = m_gemModel->GetName(index);
                    const QString error = tr("Gem %1 was removed or unregistered, but is still used by the project.").arg(gemName);
                    AZ_Warning("Project Manager", false, error.toUtf8().constData());
                    QMessageBox::warning(this, tr("Gem not found"), error.toUtf8().constData());
                }

                i++;
            }
        }

        m_gemModel->UpdateGemDependencies();
        m_proxyModel->sort(/*column=*/0);
        m_proxyModel->InvalidateFilter();
        m_filterWidget->UpdateAllFilters(/*clearAllCheckboxes=*/false);

        auto selectedIndex = m_proxyModel->GetSelectionModel()->currentIndex();
        // be careful to not pass in the proxy model index, we want the source model index
        if (selectedIndex.isValid())
        {
            m_gemInspector->Update(m_proxyModel->mapToSource(selectedIndex));
        }
        else
        {
            QModelIndex firstProxyIndex = m_proxyModel->index(0, 0);
            m_proxyModel->GetSelectionModel()->setCurrentIndex(firstProxyIndex, QItemSelectionModel::ClearAndSelect);
            m_gemInspector->Update(m_proxyModel->mapToSource(firstProxyIndex));
        }
    }

    void GemCatalogScreen::OnGemStatusChanged(const QString& gemName, uint32_t numChangedDependencies) 
    {
        if (m_notificationsEnabled && !m_readOnly)
        {
            auto modelIndex = m_gemModel->FindIndexByNameString(gemName);
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
                const GemInfo& gemInfo = GemModel::GetGemInfo(modelIndex);
                const QString& newVersion = GemModel::GetNewVersion(modelIndex);
                const QString& version = newVersion.isEmpty() ? gemInfo.m_version : newVersion;


                // avoid showing the version twice if it's already in the display name
                if (gemInfo.m_isEngineGem || (version.isEmpty() || gemInfo.m_displayName.contains(version) || version.contains("Unknown", Qt::CaseInsensitive)))
                {
                    notification = gemInfo.m_displayName;
                } 
                else
                {
                    notification = QString("%1 %2").arg(gemInfo.m_displayName, version);
                }

                if (numChangedDependencies > 0)
                {
                    notification += tr(" and ");
                }

                if (added)
                {
                    if (newVersion.isEmpty() && (GemModel::GetDownloadStatus(modelIndex) == GemInfo::DownloadStatus::NotDownloaded) ||
                        (GemModel::GetDownloadStatus(modelIndex) == GemInfo::DownloadStatus::DownloadFailed))
                    {
                        // download the current version
                        DownloadGem(modelIndex, gemInfo.m_version, gemInfo.m_path);
                    }
                    else if (!newVersion.isEmpty())
                    {
                        const GemInfo& newVersionGemInfo = GemModel::GetGemInfo(modelIndex, newVersion);
                        if (newVersionGemInfo.m_downloadStatus == GemInfo::DownloadStatus::NotDownloaded ||
                            GemModel::GetDownloadStatus(modelIndex) == GemInfo::DownloadStatus::DownloadFailed)
                        {
                            // download the new version
                            DownloadGem(modelIndex, newVersionGemInfo.m_version, newVersionGemInfo.m_path);
                        }
                    }
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
        auto modelIndex = m_gemModel->FindIndexByNameString(gemName);
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
        auto modelIndex = m_gemModel->FindIndexByNameString(gemName);
        if (!m_proxyModel->filterAcceptsRow(modelIndex.row(), QModelIndex()))
        {
            m_proxyModel->ResetFilters();
            m_filterWidget->UpdateAllFilters(/*clearAllCheckboxes=*/true);
        }

        if (QModelIndex proxyIndex = m_proxyModel->mapFromSource(modelIndex); proxyIndex.isValid())
        {
            m_proxyModel->GetSelectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);
            m_gemListView->scrollTo(proxyIndex);
        }

        ShowInspector();
    }

    void GemCatalogScreen::UpdateGem(const QModelIndex& modelIndex, const QString& version, const QString& path)
    {
        const GemInfo& gemInfo = m_gemModel->GetGemInfo(modelIndex, version, path);

        if (!gemInfo.m_repoUri.isEmpty())
        {
            AZ::Outcome<void, AZStd::string> refreshResult = PythonBindingsInterface::Get()->RefreshGemRepo(gemInfo.m_repoUri);
            if (refreshResult.IsSuccess())
            {
                Refresh();
            }
            else
            {
                QMessageBox::critical(
                    this, tr("Operation failed"),
                    tr("Failed to refresh gem repository %1<br>Error:<br>%2").arg(gemInfo.m_repoUri, refreshResult.GetError().c_str()));
            }
        }
        // If repo uri isn't specified warn user that repo might not be refreshed
        else
        {
            int result = QMessageBox::warning(
                this, tr("Gem Repository Unspecified"),
                tr("The repo for %1 is unspecfied. Repository cannot be automatically refreshed. "
                   "Please ensure this gem's repo is refreshed before attempting to update.")
                    .arg(gemInfo.m_displayName),
                QMessageBox::Cancel, QMessageBox::Ok);

            // Allow user to cancel update to manually refresh repo
            if (result != QMessageBox::Ok)
            {
                return;
            }
        }

        // include the version if valid
        auto outcome = AZ::SemanticVersion::ParseFromString(version.toUtf8().constData());
        const QString gemName = outcome ? QString("%1==%2").arg(gemInfo.m_name, version) : gemInfo.m_name;

        // Check if there is an update avaliable now that repo is refreshed
        bool updateAvaliable = PythonBindingsInterface::Get()->IsGemUpdateAvaliable(gemName, gemInfo.m_lastUpdatedDate);

        GemUpdateDialog* confirmUpdateDialog = new GemUpdateDialog(gemInfo.m_name, updateAvaliable, this);
        if (confirmUpdateDialog->exec() == QDialog::Accepted)
        {
            DownloadGem(modelIndex, version, path);
        }
    }

    void GemCatalogScreen::DownloadGem(const QModelIndex& modelIndex, const QString& version, const QString& path)
    {
        const GemInfo& gemInfo = m_gemModel->GetGemInfo(modelIndex, version, path);

        // include the version if valid
        auto outcome = AZ::SemanticVersion::ParseFromString(version.toUtf8().constData());
        const QString gemName = outcome ? QString("%1==%2").arg(gemInfo.m_name, version) : gemInfo.m_name;
        m_downloadController->AddObjectDownload(gemName, "" , DownloadController::DownloadObjectType::Gem);

        GemModel::SetDownloadStatus(*m_gemModel, modelIndex, GemInfo::DownloadStatus::Downloading);
    }

    void GemCatalogScreen::UninstallGem(const QModelIndex& modelIndex, const QString& path)
    {
        const QString gemDisplayName = m_gemModel->GetDisplayName(modelIndex);
        const GemInfo& gemInfo = m_gemModel->GetGemInfo(modelIndex, "", path);

        bool confirmed = false;
        if (gemInfo.m_gemOrigin == GemInfo::Remote)
        {
            GemUninstallDialog* confirmUninstallDialog = new GemUninstallDialog(gemDisplayName, this);
            confirmed = confirmUninstallDialog->exec() == QDialog::Accepted;
        }
        else
        {
            confirmed = QMessageBox::warning(this, tr("Remove Gem"),
                tr("Do you want to remove %1?<br>The gem will only be unregistered, and can be re-added.  The files will not be removed from disk.").arg(gemDisplayName),
                QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok;
        }

        if (confirmed)
        {
            const bool wasAdded = GemModel::WasPreviouslyAdded(modelIndex);
            const bool wasAddedDependency = GemModel::WasPreviouslyAddedDependency(modelIndex);

            // Remove gem from gems to be added to update any dependencies
            GemModel::SetIsAdded(*m_gemModel, modelIndex, false);
            GemModel::DeactivateDependentGems(*m_gemModel, modelIndex);

            // Unregister the gem
            auto unregisterResult = PythonBindingsInterface::Get()->UnregisterGem(path);
            if (!unregisterResult)
            {
                QMessageBox::critical(this, tr("Failed to unregister %1").arg(gemDisplayName), unregisterResult.GetError().c_str());
            }
            else
            {
                const QString gemName = m_gemModel->GetName(modelIndex);
                m_gemModel->RemoveGem(gemName, /*version=*/"", path);

                bool filesDeleted = false;
                if (gemInfo.m_gemOrigin == GemInfo::Remote)
                {
                    // Remote gems will have their files deleted 
                    filesDeleted = ProjectUtils::DeleteProjectFiles(path, /*force*/ true);
                    if (!filesDeleted)
                    {
                        QMessageBox::critical(
                            this, tr("Failed to remove gem directory"), tr("Could not delete gem directory at:<br>%1").arg(path));
                    }
                    else
                    {
                        ShowStandardToastNotification(tr("%1 uninstalled").arg(gemDisplayName));
                    }
                }
                else
                {
                        ShowStandardToastNotification(tr("%1 removed").arg(gemDisplayName));
                }

                Refresh();

                const auto gemIndex = m_gemModel->FindIndexByNameString(gemName);
                QModelIndex proxyIndex;
                if (gemIndex.isValid())
                {
                    GemModel::SetWasPreviouslyAdded(*m_gemModel, gemIndex, wasAdded);
                    GemModel::SetWasPreviouslyAddedDependency(*m_gemModel, gemIndex, wasAddedDependency);

                    if (filesDeleted)
                    {
                        GemModel::SetDownloadStatus(*m_gemModel, gemIndex, GemInfo::DownloadStatus::NotDownloaded);
                    }

                    proxyIndex = m_proxyModel->mapFromSource(gemIndex);
                }

                if (!proxyIndex.isValid())
                {
                    // if the gem was removed then we need to pick a new entry
                    proxyIndex = m_proxyModel->mapFromSource(m_gemModel->index(0, 0));
                }

                if (proxyIndex.isValid())
                {
                    m_proxyModel->GetSelectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);
                }
                else
                {
                    m_proxyModel->GetSelectionModel()->setCurrentIndex(m_proxyModel->index(0,0), QItemSelectionModel::ClearAndSelect);
                }
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
            m_gemModel->AddGems(allGemInfosResult.GetValue());

            const auto& allRepoGemInfosResult = PythonBindingsInterface::Get()->GetGemInfosForAllRepos(projectPath);
            if (allRepoGemInfosResult.IsSuccess())
            {
                m_gemModel->AddGems(allRepoGemInfosResult.GetValue());
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Operation failed"), QString("Cannot retrieve gems from repos.<br><br>Error:<br>%1").arg(allRepoGemInfosResult.GetError().c_str()));
            }

            // we need to update all gem dependencies before activating all the gems for this project
            m_gemModel->UpdateGemDependencies();

            if (!m_projectPath.isEmpty())
            {
                constexpr bool includeDependencies = false;
                const auto& enabledGemNamesResult = PythonBindingsInterface::Get()->GetEnabledGems(projectPath, includeDependencies);
                if (enabledGemNamesResult.IsSuccess())
                {
                    m_gemModel->ActivateGems(enabledGemNamesResult.GetValue());
                }
                else
                {
                    QMessageBox::critical(nullptr, tr("Operation failed"), QString("Cannot retrieve enabled gems for project %1.<br><br>Error:<br>%2").arg(projectPath, enabledGemNamesResult.GetError().c_str()));
                }
            }

            // sort after activating gems in case the display name for a gem is different for the active version 
            m_proxyModel->sort(/*column=*/0);
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

    void GemCatalogScreen::HandleEditGem(const QModelIndex& currentModelIndex, const QString& path)
    {
        if (m_screensControl)
        {
            auto editGemScreen = qobject_cast<EditGem*>(m_screensControl->FindScreen(ProjectManagerScreen::EditGem));
            if (editGemScreen)
            {
                m_curEditedIndex = currentModelIndex;
                editGemScreen->ResetWorkflow(m_gemModel->GetGemInfo(currentModelIndex, /*version=*/"", path));
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
        QString gemNameWithoutVersionSpecifier =  ProjectUtils::GetDependencyName(gemName);
        const auto index = m_gemModel->FindIndexByNameString(gemNameWithoutVersionSpecifier);
        if (succeeded)
        {
            Refresh();

            if(index.isValid())
            {
                GemModel::SetDownloadStatus(*m_gemModel, index, GemInfo::DownloadSuccessful);
            }
        }
        else if (index.isValid())
        {
            GemModel::SetIsAdded(*m_gemModel, index, false);
            GemModel::DeactivateDependentGems(*m_gemModel, index);
            GemModel::SetDownloadStatus(*m_gemModel, index, GemInfo::DownloadFailed);
        }
    }

    void GemCatalogScreen::HandleGemCreated(const GemInfo& gemInfo)
    {
        // This signal occurs only upon successful completion of creating a gem. As such, the gemInfo data is assumed to be valid.

        // make sure the project gem catalog model is updated
        AddToGemModel(gemInfo);

        // create Toast Notification for project gem catalog
        QString notification = tr("%1 has been created").arg(gemInfo.m_displayName);
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

        QString notification = tr("%1 was edited").arg(newGemInfo.m_displayName);
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
