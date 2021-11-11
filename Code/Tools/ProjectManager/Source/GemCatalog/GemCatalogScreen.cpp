/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemCatalogScreen.h>
#include <PythonBindingsInterface.h>
#include <GemCatalog/GemListHeaderWidget.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <GemCatalog/GemRequirementDialog.h>
#include <GemCatalog/GemDependenciesDialog.h>
#include <DownloadController.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <PythonBindingsInterface.h>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QHash>

namespace O3DE::ProjectManager
{
    GemCatalogScreen::GemCatalogScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        m_gemModel = new GemModel(this);
        m_proxyModel = new GemSortFilterProxyModel(m_gemModel, this);

        // default to sort by gem name
        m_proxyModel->setSortRole(GemModel::RoleName);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        m_downloadController = new DownloadController();

        m_headerWidget = new GemCatalogHeaderWidget(m_gemModel, m_proxyModel, m_downloadController);
        vLayout->addWidget(m_headerWidget);

        connect(m_gemModel, &GemModel::gemStatusChanged, this, &GemCatalogScreen::OnGemStatusChanged);
        connect(m_headerWidget, &GemCatalogHeaderWidget::OpenGemsRepo, this, &GemCatalogScreen::HandleOpenGemRepo);
        connect(m_headerWidget, &GemCatalogHeaderWidget::AddGem, this, &GemCatalogScreen::OnAddGemClicked);
        connect(m_downloadController, &DownloadController::Done, this, &GemCatalogScreen::OnGemDownloadResult);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        vLayout->addLayout(hLayout);

        m_gemListView = new GemListView(m_proxyModel, m_proxyModel->GetSelectionModel(), this);
        m_gemInspector = new GemInspector(m_gemModel, this);
        m_gemInspector->setFixedWidth(240);

        connect(m_gemInspector, &GemInspector::TagClicked, [=](const Tag& tag) { SelectGem(tag.id); });

        QWidget* filterWidget = new QWidget(this);
        filterWidget->setFixedWidth(240);
        m_filterWidgetLayout = new QVBoxLayout();
        m_filterWidgetLayout->setMargin(0);
        m_filterWidgetLayout->setSpacing(0);
        filterWidget->setLayout(m_filterWidgetLayout);

        GemListHeaderWidget* listHeaderWidget = new GemListHeaderWidget(m_proxyModel);

        QVBoxLayout* middleVLayout = new QVBoxLayout();
        middleVLayout->setMargin(0);
        middleVLayout->setSpacing(0);
        middleVLayout->addWidget(listHeaderWidget);
        middleVLayout->addWidget(m_gemListView);

        hLayout->addWidget(filterWidget);
        hLayout->addLayout(middleVLayout);
        hLayout->addWidget(m_gemInspector);

        m_notificationsView = AZStd::make_unique<AzToolsFramework::ToastNotificationsView>(this, AZ_CRC("GemCatalogNotificationsView"));
        m_notificationsView->SetOffset(QPoint(10, 70));
        m_notificationsView->SetMaxQueuedNotifications(1);
    }

    void GemCatalogScreen::ReinitForProject(const QString& projectPath)
    {
        m_projectPath = projectPath;
        m_gemModel->Clear();
        m_gemsToRegisterWithProject.clear();
        FillModel(projectPath);

        m_proxyModel->ResetFilters();
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
            QModelIndex firstModelIndex = m_gemListView->model()->index(0,0);
            m_gemListView->selectionModel()->select(firstModelIndex, QItemSelectionModel::ClearAndSelect);
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
                AZ::Outcome<GemInfo, void> gemInfoResult = PythonBindingsInterface::Get()->GetGemInfo(directory);
                if (gemInfoResult)
                {
                    m_gemModel->AddGem(gemInfoResult.GetValue<GemInfo>());
                    m_gemModel->UpdateGemDependencies();
                    m_proxyModel->sort(/*column=*/0);
                }
            }
        }
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
                gemInfoHash.insert(gemInfo.m_name, gemInfo);
            }
        }

        // add all the gem repos into the hash
        const AZ::Outcome<QVector<GemInfo>, AZStd::string>& allRepoGemInfosResult = PythonBindingsInterface::Get()->GetAllGemRepoGemsInfos();
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
                m_gemModel->removeRow(i);
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
        m_proxyModel->ResetFilters();
        m_filterWidget->ResetAllFilters();
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
                    notification += " " + tr("and") + " ";
                }
                if (added && GemModel::GetDownloadStatus(modelIndex) == GemInfo::DownloadStatus::NotDownloaded)
                {
                    m_downloadController->AddGemDownload(GemModel::GetName(modelIndex));
                }
            }

            if (numChangedDependencies == 1 )
            {
                notification += "1 Gem " + tr("dependency");
            }
            else if (numChangedDependencies > 1)
            {
                notification += QString("%1 Gem ").arg(numChangedDependencies) + tr("dependencies");
            }
            notification += " " + (added ? tr("activated") : tr("deactivated"));

            AzQtComponents::ToastConfiguration toastConfiguration(AzQtComponents::ToastType::Custom, notification, "");
            toastConfiguration.m_customIconImage = ":/gem.svg";
            toastConfiguration.m_borderRadius = 4;
            toastConfiguration.m_duration = AZStd::chrono::milliseconds(3000);
            m_notificationsView->ShowToastNotification(toastConfiguration);
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
        m_proxyModel->GetSelectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
        m_gemListView->scrollTo(proxyIndex);
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
                m_gemModel->AddGem(gemInfo);
            }

            const AZ::Outcome<QVector<GemInfo>, AZStd::string>& allRepoGemInfosResult = PythonBindingsInterface::Get()->GetAllGemRepoGemsInfos();
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

    GemCatalogScreen::EnableDisableGemsResult GemCatalogScreen::EnableDisableGemsForProject(const QString& projectPath)
    {
        IPythonBindings* pythonBindings = PythonBindingsInterface::Get();
        QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded();
        QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved();

        if (m_gemModel->DoGemsToBeAddedHaveRequirements())
        {
            GemRequirementDialog* confirmRequirementsDialog = new GemRequirementDialog(m_gemModel, this);
            if(confirmRequirementsDialog->exec() == QDialog::Rejected)
            {
                return EnableDisableGemsResult::Cancel;
            }
        }

        if (m_gemModel->HasDependentGemsToRemove())
        {
            GemDependenciesDialog* dependenciesDialog = new GemDependenciesDialog(m_gemModel, this);
            if(dependenciesDialog->exec() == QDialog::Rejected)
            {
                return EnableDisableGemsResult::Cancel;
            }

            toBeAdded = m_gemModel->GatherGemsToBeAdded();
            toBeRemoved = m_gemModel->GatherGemsToBeRemoved();
        }

        for (const QModelIndex& modelIndex : toBeAdded)
        {
            const QString& gemPath = GemModel::GetPath(modelIndex);

            // make sure any remote gems we added were downloaded successfully 
            if (GemModel::GetGemOrigin(modelIndex) == GemInfo::Remote && GemModel::GetDownloadStatus(modelIndex) != GemInfo::Downloaded)
            {
                QMessageBox::critical(
                    nullptr, "Cannot add gem that isn't downloaded",
                    tr("Cannot add gem %1 to project because it isn't downloaded yet or failed to download.")
                        .arg(GemModel::GetDisplayName(modelIndex)));

                return EnableDisableGemsResult::Failed;
            }

            const AZ::Outcome<void, AZStd::string> result = pythonBindings->AddGemToProject(gemPath, projectPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(nullptr, "Failed to add gem to project",
                    tr("Cannot add gem %1 to project.<br><br>Error:<br>%2").arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

                return EnableDisableGemsResult::Failed;
            }

            // register external gems that were added with relative paths
            if (m_gemsToRegisterWithProject.contains(gemPath))
            {
                pythonBindings->RegisterGem(QDir(projectPath).relativeFilePath(gemPath), projectPath);
            }
        }

        for (const QModelIndex& modelIndex : toBeRemoved)
        {
            const QString gemPath = GemModel::GetPath(modelIndex);
            const AZ::Outcome<void, AZStd::string> result = pythonBindings->RemoveGemFromProject(gemPath, projectPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(nullptr, "Failed to remove gem from project",
                    tr("Cannot remove gem %1 from project.<br><br>Error:<br>%2").arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

                return EnableDisableGemsResult::Failed;
            }
        }

        return EnableDisableGemsResult::Success;
    }

    void GemCatalogScreen::HandleOpenGemRepo()
    {
        emit ChangeScreenRequest(ProjectManagerScreen::GemRepos);
    }

    void GemCatalogScreen::OnGemDownloadResult(const QString& gemName, bool succeeded)
    {
        if (succeeded)
        {
            // refresh the information for downloaded gems
            const AZ::Outcome<QVector<GemInfo>, AZStd::string>& allGemInfosResult = PythonBindingsInterface::Get()->GetAllGemInfos(m_projectPath);
            if (allGemInfosResult.IsSuccess())
            {
                // we should find the gem name now in all gem infos
                for (const GemInfo& gemInfo : allGemInfosResult.GetValue())
                {
                    if (gemInfo.m_name == gemName)
                    {
                        QModelIndex index = m_gemModel->FindIndexByNameString(gemName);
                        if (index.isValid())
                        {
                            m_gemModel->setData(index, GemInfo::Downloaded, GemModel::RoleDownloadStatus);
                            m_gemModel->setData(index, gemInfo.m_path, GemModel::RolePath);
                            m_gemModel->setData(index, gemInfo.m_path, GemModel::RoleDirectoryLink);
                        }

                        return;
                    }
                }
            }
        }
    }

    ProjectManagerScreen GemCatalogScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemCatalog;
    }
} // namespace O3DE::ProjectManager
