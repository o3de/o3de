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

namespace O3DE::ProjectManager
{
    GemCatalogScreen::GemCatalogScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        m_gemModel = new GemModel(this);
        m_proxModel = new GemSortFilterProxyModel(m_gemModel, this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        m_downloadController = new DownloadController();

        m_headerWidget = new GemCatalogHeaderWidget(m_gemModel, m_proxModel, m_downloadController);
        vLayout->addWidget(m_headerWidget);

        connect(m_gemModel, &GemModel::gemStatusChanged, this, &GemCatalogScreen::OnGemStatusChanged);
        connect(m_headerWidget, &GemCatalogHeaderWidget::OpenGemsRepo, this, &GemCatalogScreen::HandleOpenGemRepo);
        connect(m_headerWidget, &GemCatalogHeaderWidget::AddGem, this, &GemCatalogScreen::OnAddGemClicked);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        vLayout->addLayout(hLayout);

        m_gemListView = new GemListView(m_proxModel, m_proxModel->GetSelectionModel(), this);
        m_gemInspector = new GemInspector(m_gemModel, this);
        m_gemInspector->setFixedWidth(240);

        QWidget* filterWidget = new QWidget(this);
        filterWidget->setFixedWidth(240);
        m_filterWidgetLayout = new QVBoxLayout();
        m_filterWidgetLayout->setMargin(0);
        m_filterWidgetLayout->setSpacing(0);
        filterWidget->setLayout(m_filterWidgetLayout);

        GemListHeaderWidget* listHeaderWidget = new GemListHeaderWidget(m_proxModel);

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
        m_gemModel->Clear();
        m_gemsToRegisterWithProject.clear();
        FillModel(projectPath);

        if (m_filterWidget)
        {
            m_filterWidget->hide();
            m_filterWidget->deleteLater();
        }

        m_proxModel->ResetFilters();
        m_filterWidget = new GemFilterWidget(m_proxModel);
        m_filterWidgetLayout->addWidget(m_filterWidget);

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
                }
            }
        }
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
        AZ::Outcome<QVector<GemInfo>, AZStd::string> allGemInfosResult = PythonBindingsInterface::Get()->GetAllGemInfos(projectPath);
        if (allGemInfosResult.IsSuccess())
        {
            // Add all available gems to the model.
            const QVector<GemInfo> allGemInfos = allGemInfosResult.GetValue();
            for (const GemInfo& gemInfo : allGemInfos)
            {
                m_gemModel->AddGem(gemInfo);
            }

            AZ::Outcome<QVector<GemInfo>, AZStd::string> allRepoGemInfosResult = PythonBindingsInterface::Get()->GetAllGemRepoGemsInfos();
            if (allRepoGemInfosResult.IsSuccess())
            {
                const QVector<GemInfo> allRepoGemInfos = allRepoGemInfosResult.GetValue();
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
            auto enabledGemNamesResult = PythonBindingsInterface::Get()->GetEnabledGemNames(projectPath);
            if (enabledGemNamesResult.IsSuccess())
            {
                const QVector<AZStd::string> enabledGemNames = enabledGemNamesResult.GetValue();
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
            const QString gemPath = GemModel::GetPath(modelIndex);
            const AZ::Outcome<void, AZStd::string> result = pythonBindings->AddGemToProject(gemPath, projectPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(nullptr, "Operation failed",
                    QString("Cannot add gem %1 to project.\n\nError:\n%2").arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

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
                QMessageBox::critical(nullptr, "Operation failed",
                    QString("Cannot remove gem %1 from project.\n\nError:\n%2").arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

                return EnableDisableGemsResult::Failed;
            }
        }

        return EnableDisableGemsResult::Success;
    }

    void GemCatalogScreen::HandleOpenGemRepo()
    {
        QVector<QModelIndex> gemsToBeAdded = m_gemModel->GatherGemsToBeAdded(true);
        QVector<QModelIndex> gemsToBeRemoved = m_gemModel->GatherGemsToBeRemoved(true);

        if (!gemsToBeAdded.empty() || !gemsToBeRemoved.empty())
        {
            QMessageBox::StandardButton warningResult = QMessageBox::warning(
                nullptr, "Pending Changes",
                "There are some unsaved changes to the gem selection,<br> they will be lost if you change screens.<br> Are you sure?",
                QMessageBox::No | QMessageBox::Yes);

            if (warningResult != QMessageBox::Yes)
            {
                return;
            }
        }

        emit ChangeScreenRequest(ProjectManagerScreen::GemRepos);
    }

    ProjectManagerScreen GemCatalogScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemCatalog;
    }
} // namespace O3DE::ProjectManager
