/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProductAssetDetailsPanel.h"

#include "AssetTreeFilterModel.h"
#include "ProductAssetTreeItemData.h"
#include "native/utilities/assetUtils.h"
#include "native/utilities/MissingDependencyScanner.h"

#include <AssetDatabase/AssetDatabase.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzCore/std/algorithm.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <native/ui/ui_GoToButton.h>
#include <native/ui/ui_ProductAssetDetailsPanel.h>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QStringLiteral>
#include <QUrl>
#include <native/ui/GoToButtonDelegate.h>
#include <AzCore/Jobs/JobFunction.h>

namespace AssetProcessor
{

    ProductAssetDetailsPanel::ProductAssetDetailsPanel(QWidget* parent) : AssetDetailsPanel(parent), m_ui(new Ui::ProductAssetDetailsPanel)
    {
        m_ui->setupUi(this);
        m_ui->scrollAreaWidgetContents->setLayout(m_ui->scrollableVerticalLayout);
        m_ui->MissingProductDependenciesTable->setColumnWidth(static_cast<int>(MissingDependencyTableColumns::ScanTime), 160);
        ResetText();
        connect(m_ui->MissingProductDependenciesSupport, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnSupportClicked);
        connect(m_ui->ScanMissingDependenciesButton, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnScanFileClicked);
        connect(m_ui->ScanFolderButton, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnScanFolderClicked);

        connect(m_ui->ClearMissingDependenciesButton, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnClearScanFileClicked);
        connect(m_ui->ClearScanFolderButton, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnClearScanFolderClicked);

        auto* missingDependenciesDelegate = new GoToButtonDelegate(this);
        connect(
            missingDependenciesDelegate,
            &GoToButtonDelegate::Clicked,
            [this](const GoToButtonData& buttonData)
            {
                GoToProduct(buttonData.m_destination);
            });
        m_ui->MissingProductDependenciesTable->setItemDelegate(missingDependenciesDelegate);
    }

    ProductAssetDetailsPanel::~ProductAssetDetailsPanel()
    {

    }

    void ProductAssetDetailsPanel::SetupDependencyGraph(QTreeView* productAssetsTreeView, AZStd::shared_ptr<AssetDatabaseConnection> assetDatabaseConnection)
    {
        m_outgoingDependencyTreeModel =
            new ProductDependencyTreeModel(assetDatabaseConnection, m_productFilterModel, DependencyTreeType::Outgoing, this);
        m_ui->OutgoingProductDependenciesTreeView->setModel(m_outgoingDependencyTreeModel);
        m_ui->OutgoingProductDependenciesTreeView->setRootIsDecorated(true);
        m_ui->OutgoingProductDependenciesTreeView->setItemDelegate(new ProductDependencyTreeDelegate(this, this));
        connect(
            productAssetsTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, m_outgoingDependencyTreeModel,
            &ProductDependencyTreeModel::AssetDataSelectionChanged);

        m_incomingDependencyTreeModel =
            new ProductDependencyTreeModel(assetDatabaseConnection, m_productFilterModel, DependencyTreeType::Incoming, this);
        m_ui->IncomingProductDependenciesTreeView->setModel(m_incomingDependencyTreeModel);
        m_ui->IncomingProductDependenciesTreeView->setRootIsDecorated(true);
        m_ui->IncomingProductDependenciesTreeView->setItemDelegate(new ProductDependencyTreeDelegate(this, this));
        connect(
            productAssetsTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, m_incomingDependencyTreeModel,
            &ProductDependencyTreeModel::AssetDataSelectionChanged);

        AzQtComponents::StyleManager::setStyleSheet(m_ui->OutgoingProductDependenciesTreeView, QStringLiteral("style:AssetProcessor.qss"));
        AzQtComponents::StyleManager::setStyleSheet(m_ui->IncomingProductDependenciesTreeView, QStringLiteral("style:AssetProcessor.qss"));
    }

    QTreeView* ProductAssetDetailsPanel::GetOutgoingProductDependenciesTreeView() const
    {
        return m_ui->OutgoingProductDependenciesTreeView;
    }

    QTreeView* ProductAssetDetailsPanel::GetIncomingProductDependenciesTreeView() const
    {
        return m_ui->IncomingProductDependenciesTreeView;
    }

    void ProductAssetDetailsPanel::SetScanQueueEnabled(bool enabled)
    {
        // Don't change state if it's already the same.
        if (m_ui->ScanMissingDependenciesButton->isEnabled() == enabled)
        {
            return;
        }
        m_ui->ScanMissingDependenciesButton->setEnabled(enabled);
        m_ui->ScanFolderButton->setEnabled(enabled);

        if (enabled)
        {
            m_ui->ScanMissingDependenciesButton->setToolTip(tr("Scans this file for missing dependencies. This may take some time."));
            m_ui->ScanFolderButton->setToolTip(tr("Scans all files in this folder and subfolders for missing dependencies. This may take some time."));
        }
        else
        {
            QString disabledTooltip(tr("Scanning disabled until asset processing completes."));
            m_ui->ScanMissingDependenciesButton->setToolTip(disabledTooltip);
            m_ui->ScanFolderButton->setToolTip(disabledTooltip);
        }
    }

    void ProductAssetDetailsPanel::AssetDataSelectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
    {
        // Even if multi-select is enabled, only display the first selected item.
        if (selected.indexes().count() == 0 || !selected.indexes()[0].isValid())
        {
            ResetText();
            return;
        }

        QModelIndex productModelIndex = m_productFilterModel->mapToSource(selected.indexes()[0]);

        if (!productModelIndex.isValid())
        {
            return;
        }
        m_currentItem = static_cast<AssetTreeItem*>(productModelIndex.internalPointer());
        RefreshUI();
    }

    void ProductAssetDetailsPanel::RefreshUI()
    {
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(m_currentItem->GetData());
        m_ui->assetNameLabel->setText(m_currentItem->GetData()->m_name);

        if (m_currentItem->GetData()->m_isFolder || !productItemData)
        {
            // Folders don't have details.
            SetDetailsVisible(false);
            return;
        }
        SetDetailsVisible(true);

        AZ::Data::AssetId assetId;

        m_assetDatabaseConnection->QuerySourceByProductID(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
        {
            assetId = AZ::Data::AssetId(sourceEntry.m_sourceGuid, productItemData->m_databaseInfo.m_subID);
            // Use a decimal value to display the sub ID and not hex. Open 3D Engine is not consistent about
            // how sub IDs are displayed, so it's important to double check what format a sub ID is in before using it elsewhere.
            m_ui->productAssetIdValueLabel->setText(assetId.ToString<AZStd::string>(AZ::Data::AssetId::SubIdDisplayType::Decimal).c_str());

            // Make sure this is the only connection to the button.
            m_ui->gotoAssetButton->m_ui->goToPushButton->disconnect();

            connect(m_ui->gotoAssetButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                GoToSource(SourceAssetReference(sourceEntry.m_scanFolderPK, sourceEntry.m_sourceName.c_str()).AbsolutePath().c_str());
            });

            m_ui->sourceAssetValueLabel->setText(sourceEntry.m_sourceName.c_str());
            return true;
        });

        AZStd::string platform;

        m_assetDatabaseConnection->QueryJobByProductID(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
        {
            QDateTime lastTimeProcessed = QDateTime::fromMSecsSinceEpoch(jobEntry.m_lastLogTime);
            m_ui->lastTimeProcessedValueLabel->setText(lastTimeProcessed.toString());

            m_ui->jobKeyValueLabel->setText(jobEntry.m_jobKey.c_str());
            platform = jobEntry.m_platform;
            m_ui->platformValueLabel->setText(jobEntry.m_platform.c_str());
            return true;
        });

        BuildOutgoingProductDependencies(productItemData, platform);
        BuildIncomingProductDependencies(productItemData, assetId, platform);
        BuildMissingProductDependencies(productItemData);
    }

    void ProductAssetDetailsPanel::BuildOutgoingProductDependencies(
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
        const AZStd::string& platform)
    {

        m_ui->outgoingUnmetPathProductDependenciesList->clear();
        int productDependencyCount = 0;
        int productPathDependencyCount = 0;
        m_assetDatabaseConnection->QueryProductDependencyByProductId(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& dependency)
        {
            if (!dependency.m_dependencySourceGuid.IsNull())
            {
                m_assetDatabaseConnection->QueryProductBySourceGuidSubID(
                    dependency.m_dependencySourceGuid,
                    dependency.m_dependencySubID,
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product)
                {
                    bool platformMatches = false;

                    m_assetDatabaseConnection->QueryJobByJobID(
                        product.m_jobPK,
                        [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
                    {
                        if (platform.compare(jobEntry.m_platform) == 0)
                        {
                            platformMatches = true;
                        }
                        return true;
                    });

                    if (platformMatches)
                    {
                        ++productDependencyCount;
                    }
                    return true;
                });
            }

            // If there is both a path and an asset ID on this dependency, then something has gone wrong.
            // Other tooling should have reported this error. In the UI, show both the asset ID and path.
            if (!dependency.m_unresolvedPath.empty())
            {
                QListWidgetItem* listWidgetItem = new QListWidgetItem();
                listWidgetItem->setText(dependency.m_unresolvedPath.c_str());
                m_ui->outgoingUnmetPathProductDependenciesList->addItem(listWidgetItem);
                ++productPathDependencyCount;
            }
            return true;
        });
        m_ui->outgoingProductDependenciesValueLabel->setText(QString::number(productDependencyCount));
        m_ui->outgoingUnmetPathProductDependenciesValueLabel->setText(QString::number(productPathDependencyCount));

        if (productPathDependencyCount == 0)
        {
            QListWidgetItem* listWidgetItem = new QListWidgetItem();
            listWidgetItem->setText(tr("No unmet dependencies"));
            m_ui->outgoingUnmetPathProductDependenciesList->addItem(listWidgetItem);
            ++productPathDependencyCount;
        }

        int height = m_ui->outgoingUnmetPathProductDependenciesList->sizeHintForRow(0) * AZStd::min<int>(productPathDependencyCount, 4) +
            2 * m_ui->outgoingUnmetPathProductDependenciesList->frameWidth();
        m_ui->outgoingUnmetPathProductDependenciesList->setMinimumHeight(height);
        m_ui->outgoingUnmetPathProductDependenciesList->setMaximumHeight(height);
        m_ui->outgoingUnmetPathProductDependenciesList->adjustSize();
    }

    void ProductAssetDetailsPanel::BuildIncomingProductDependencies(
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
        const AZ::Data::AssetId& assetId,
        const AZStd::string& platform)
    {

        int incomingProductDependencyCount = 0;
        m_assetDatabaseConnection->QueryDirectReverseProductDependenciesBySourceGuidSubId(
            assetId.m_guid,
            assetId.m_subId,
            [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& incomingDependency)
        {
            bool platformMatches = false;

            m_assetDatabaseConnection->QueryJobByJobID(
                incomingDependency.m_jobPK,
                [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
            {
                if (platform.compare(jobEntry.m_platform) == 0)
                {
                    platformMatches = true;
                }
                return true;
            });
            if (platformMatches)
            {

                ++incomingProductDependencyCount;
            }
            return true;
        });

        m_ui->incomingProductDependenciesValueLabel->setText(QString::number(incomingProductDependencyCount));

    }

    struct MissingDependencyTableInfo
    {
        AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry m_databaseEntry;
        AZStd::string m_missingProductName;
    };

    void ProductAssetDetailsPanel::BuildMissingProductDependencies(
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData)
    {
        // Clear & ClearContents leave the table dimensions the same, so set rowCount to zero to reset it.
        m_ui->MissingProductDependenciesTable->setRowCount(0);

        int missingDependencyRowCount = 0;
        int missingDependencyCount = 0;

        // Sort missing dependencies by scan time.
        AZStd::vector<MissingDependencyTableInfo> missingDependenciesByScanTime;

        m_assetDatabaseConnection->QueryMissingProductDependencyByProductId(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& missingDependency)
        {

            AZStd::string missingProductName;
            m_assetDatabaseConnection->QueryProductBySourceGuidSubID(
                missingDependency.m_dependencySourceGuid,
                missingDependency.m_dependencySubId,
                [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& missingProduct)
            {
                missingProductName = missingProduct.m_productName;
                return false; // There should only be one matching product, stop looking.
            });

            AZStd::vector<MissingDependencyTableInfo>::iterator insertPosition = AZStd::upper_bound(
                missingDependenciesByScanTime.begin(),
                missingDependenciesByScanTime.end(),
                missingDependency.m_scanTimeSecondsSinceEpoch,
                [](AZ::u64 left, const MissingDependencyTableInfo& right) {
                    return left > right.m_databaseEntry.m_scanTimeSecondsSinceEpoch;
                });
            MissingDependencyTableInfo missingDependencyInfo;
            missingDependencyInfo.m_databaseEntry = missingDependency;
            missingDependencyInfo.m_missingProductName = missingProductName;
            missingDependenciesByScanTime.insert(insertPosition, missingDependencyInfo);
            return true;
        });

        bool hasMissingDependency = false;
        for (const auto& missingDependency : missingDependenciesByScanTime)
        {
            m_ui->MissingProductDependenciesTable->insertRow(missingDependencyRowCount);
            // To track if files have been scanned at all, rows with invalid source guids are added on a
            // scan that had no missing dependencies. Don't show a button for those rows.
            if (!missingDependency.m_databaseEntry.m_dependencySourceGuid.IsNull())
            {
                hasMissingDependency = true;
                ++missingDependencyCount;

                auto* goToWidget = new QTableWidgetItem();
                goToWidget->setData(0, QVariant::fromValue(GoToButtonData(missingDependency.m_missingProductName)));
                m_ui->MissingProductDependenciesTable->setItem(missingDependencyRowCount,
                    static_cast<int>(MissingDependencyTableColumns::GoToButton), goToWidget);
            }

            QTableWidgetItem* scanTime = new QTableWidgetItem(missingDependency.m_databaseEntry.m_lastScanTime.c_str());
            m_ui->MissingProductDependenciesTable->setItem(
                missingDependencyRowCount, static_cast<int>(MissingDependencyTableColumns::ScanTime), scanTime);

            QTableWidgetItem* rowName = new QTableWidgetItem(missingDependency.m_databaseEntry.m_missingDependencyString.c_str());
            m_ui->MissingProductDependenciesTable->setItem(
                missingDependencyRowCount, static_cast<int>(MissingDependencyTableColumns::Dependency), rowName);

            ++missingDependencyRowCount;
        }

        m_ui->MissingProductDependenciesValueLabel->setText(QString::number(missingDependencyCount));

        if (missingDependencyRowCount == 0)
        {
            m_ui->MissingProductDependenciesTable->insertRow(missingDependencyRowCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("File has not been scanned."));
            // Put this text in the scan time column, not the missing dependency column, for layout purposes.
            m_ui->MissingProductDependenciesTable->setItem(
                missingDependencyRowCount, static_cast<int>(MissingDependencyTableColumns::ScanTime), rowName);

            ++missingDependencyRowCount;
        }
        else
        {
            m_ui->missingDependencyErrorIcon->setVisible(hasMissingDependency);
        }

        // Because this is a table nested in a scroll view, Qt struggles to automatically resize the width.
        // Set the width manually, to the size of the columns.
        m_ui->MissingProductDependenciesTable->resizeColumnToContents(static_cast<int>(MissingDependencyTableColumns::ScanTime));
        int width = 0;
        for (int columnIndex = 0; columnIndex < static_cast<int>(MissingDependencyTableColumns::Max); ++columnIndex)
        {
            width += m_ui->MissingProductDependenciesTable->columnWidth(columnIndex);
        }
        m_ui->MissingProductDependenciesTable->setMinimumWidth(width);

        m_ui->MissingProductDependenciesTable->adjustSize();
    }

    void ProductAssetDetailsPanel::ResetText()
    {
        m_ui->assetNameLabel->setText(tr("Select an asset to see details"));
        SetDetailsVisible(false);
    }

    void ProductAssetDetailsPanel::SetDetailsVisible(bool visible)
    {
        // The folder selected description has opposite visibility from everything else.
        m_ui->folderSelectedDescription->setVisible(!visible);
        m_ui->ScanFolderButton->setVisible(!visible);
        m_ui->ClearScanFolderButton->setVisible(!visible);
        m_ui->MissingProductDependenciesFolderTitleLabel->setVisible(!visible);

        m_ui->productAssetIdTitleLabel->setVisible(visible);
        m_ui->productAssetIdValueLabel->setVisible(visible);

        m_ui->lastTimeProcessedTitleLabel->setVisible(visible);
        m_ui->lastTimeProcessedValueLabel->setVisible(visible);

        m_ui->jobKeyTitleLabel->setVisible(visible);
        m_ui->jobKeyValueLabel->setVisible(visible);

        m_ui->platformTitleLabel->setVisible(visible);
        m_ui->platformValueLabel->setVisible(visible);

        m_ui->sourceAssetTitleLabel->setVisible(visible);
        m_ui->sourceAssetValueLabel->setVisible(visible);
        m_ui->gotoAssetButton->setVisible(visible);

        m_ui->ProductAssetDetailTabs->setVisible(visible);

        m_ui->MissingProductDependenciesTitleLabel->setVisible(visible);
        m_ui->MissingProductDependenciesValueLabel->setVisible(visible);
        m_ui->MissingProductDependenciesTable->setVisible(visible);
        m_ui->MissingProductDependenciesSupport->setVisible(visible);
        m_ui->ScanMissingDependenciesButton->setVisible(visible);
        m_ui->ClearMissingDependenciesButton->setVisible(visible);

        m_ui->missingDependencyErrorIcon->setVisible(false);
    }

    void ProductAssetDetailsPanel::OnSupportClicked([[maybe_unused]] bool checked)
    {
        QDesktopServices::openUrl(
            QStringLiteral("https://o3de.org/docs/user-guide/packaging/asset-bundler/assets-resolving/"));
    }


    void ProductAssetDetailsPanel::OnScanFileClicked([[maybe_unused]] bool checked)
    {
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(m_currentItem->GetData());
        ScanFileForMissingDependencies(productItemData->m_name, productItemData);
    }

    void ProductAssetDetailsPanel::ScanFileForMissingDependencies(QString scanName, const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData)
    {
        // If the file is already in the queue to scan, don't add it.
        if (m_productIdToScanName.contains(productItemData->m_databaseInfo.m_productID))
        {
            return;
        }
        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer existingDependencies;
        m_assetDatabaseConnection->QueryProductDependencyByProductId(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
        {
            existingDependencies.emplace_back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });

        QDir cacheRootDir;
        AssetUtilities::ComputeProjectCacheRoot(cacheRootDir);
        QString pathOnDisk = cacheRootDir.filePath(productItemData->m_databaseInfo.m_productName.c_str());

        AddProductIdToScanCount(productItemData->m_databaseInfo.m_productID, scanName);
        // Run the scan on another thread so the UI remains responsive.
        auto* job = AZ::CreateJobFunction([=]() {
            MissingDependencyScannerRequestBus::Broadcast(&MissingDependencyScannerRequestBus::Events::ScanFile,
                pathOnDisk.toUtf8().constData(),
                MissingDependencyScanner::DefaultMaxScanIteration,
                productItemData->m_databaseInfo.m_productID,
                existingDependencies,
                m_assetDatabaseConnection,
                /*queueDbCommandsOnMainThread*/ true,
                [=]([[maybe_unused]] AZStd::string relativeDependencyFilePath) {
                RemoveProductIdFromScanCount(productItemData->m_databaseInfo.m_productID, scanName);
                // The MissingDependencyScannerRequestBus callback always runs on the main thread, so no need to queue again.
                AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Broadcast(
                    &AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Events::OnProductFileChanged, productItemData->m_databaseInfo);
                if (m_currentItem)
                {
                    // Refresh the UI if the scan that just finished is selected.
                    const AZStd::shared_ptr<const ProductAssetTreeItemData> currentItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(m_currentItem->GetData());
                    if (currentItemData == productItemData)
                    {
                        RefreshUI();
                    }
                }
            });
        }, true);

        job->Start();
    }

    void ProductAssetDetailsPanel::AddProductIdToScanCount(AZ::s64 scannedProductId, QString scanName)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_scanCountMutex);
        m_productIdToScanName.insert(scannedProductId, scanName);
        QHash<QString, MissingDependencyScanGUIInfo>::iterator scanNameIter = m_scanNameToScanGUIInfo.find(scanName);
        if (scanNameIter == m_scanNameToScanGUIInfo.end())
        {
            MissingDependencyScanGUIInfo scanGUIInfo;
            scanGUIInfo.m_scanWidgetRow = new QListWidgetItem();
            scanGUIInfo.m_scanTimeStart = QDateTime::currentDateTime();
            if (m_missingDependencyScanResults)
            {
                m_missingDependencyScanResults->addItem(scanGUIInfo.m_scanWidgetRow);
                // New items are added to the bottom, scroll to them when they are added.
                m_missingDependencyScanResults->scrollToBottom();
            }
            scanNameIter = m_scanNameToScanGUIInfo.insert(scanName, scanGUIInfo);
        }

        // Update the remaining file count for this scan.
        scanNameIter.value().m_remainingFiles++;

        UpdateScannerUI(scanNameIter.value(), scanName);
    }

    void ProductAssetDetailsPanel::RemoveProductIdFromScanCount(AZ::s64 scannedProductId, QString scanName)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_scanCountMutex);
        m_productIdToScanName.remove(scannedProductId);
        QHash<QString, MissingDependencyScanGUIInfo>::iterator scanNameIter = m_scanNameToScanGUIInfo.find(scanName);
        if (scanNameIter != m_scanNameToScanGUIInfo.end())
        {
            // Update the remaining file count for this scan.
            scanNameIter.value().m_remainingFiles--;

            UpdateScannerUI(scanNameIter.value(), scanName);
            if (scanNameIter.value().m_remainingFiles <= 0)
            {
                m_scanNameToScanGUIInfo.remove(scanName);
            }
        }
    }

    void ProductAssetDetailsPanel::UpdateScannerUI(MissingDependencyScanGUIInfo& scannerUIInfo, QString scanName)
    {
        if (scannerUIInfo.m_scanWidgetRow == nullptr)
        {
            return;
        }
        if (scannerUIInfo.m_remainingFiles == 0)
        {
            qint64 scanTimeInSeconds = scannerUIInfo.m_scanTimeStart.secsTo(QDateTime::currentDateTime());
            scannerUIInfo.m_scanWidgetRow->setText(tr("Completed scanning %1 in %2 seconds").
                arg(scanName).
                arg(scanTimeInSeconds));
        }
        else
        {
            scannerUIInfo.m_scanWidgetRow->setText(tr("%1: Scanning %2 files for %3").
                arg(QLocale::system().toString(scannerUIInfo.m_scanTimeStart, QLocale::ShortFormat)).
                arg(scannerUIInfo.m_remainingFiles).
                arg(scanName));
        }
    }

    void ProductAssetDetailsPanel::OnScanFolderClicked([[maybe_unused]] bool checked)
    {
        if (!m_currentItem)
        {
            return;
        }
        ScanFolderForMissingDependencies(m_currentItem->GetData()->m_name, *m_currentItem);
    }

    void ProductAssetDetailsPanel::ScanFolderForMissingDependencies(QString scanName, AssetTreeItem& folder)
    {
        for (int childIndex = 0; childIndex < folder.getChildCount(); ++childIndex)
        {
            AssetTreeItem* child = folder.GetChild(childIndex);
            if (child->GetData()->m_isFolder)
            {
                ScanFolderForMissingDependencies(scanName, *child);
            }
            else
            {
                const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(child->GetData());
                ScanFileForMissingDependencies(scanName, productItemData);
            }
        }
    }

    void ProductAssetDetailsPanel::OnClearScanFileClicked([[maybe_unused]] bool checked)
    {
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(m_currentItem->GetData());
        ClearMissingDependenciesForFile(productItemData);
    }

    void ProductAssetDetailsPanel::OnClearScanFolderClicked([[maybe_unused]] bool checked)
    {
        if (!m_currentItem)
        {
            return;
        }
        ClearMissingDependenciesForFolder(*m_currentItem);
    }

    void ProductAssetDetailsPanel::ClearMissingDependenciesForFile(const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData)
    {
        m_assetDatabaseConnection->DeleteMissingProductDependencyByProductId(productItemData->m_databaseInfo.m_productID);
        AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Broadcast(
            &AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Events::OnProductFileChanged, productItemData->m_databaseInfo);

        const AZStd::shared_ptr<const ProductAssetTreeItemData> currentItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(m_currentItem->GetData());
        if (currentItemData == productItemData)
        {
            RefreshUI();
        }
    }

    void ProductAssetDetailsPanel::ClearMissingDependenciesForFolder(AssetTreeItem& folder)
    {
        for (int childIndex = 0; childIndex < folder.getChildCount(); ++childIndex)
        {
            AssetTreeItem* child = folder.GetChild(childIndex);
            if (child->GetData()->m_isFolder)
            {
                ClearMissingDependenciesForFolder(*child);
            }
            else
            {
                const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(child->GetData());
                ClearMissingDependenciesForFile(productItemData);
            }
        }
    }
}

