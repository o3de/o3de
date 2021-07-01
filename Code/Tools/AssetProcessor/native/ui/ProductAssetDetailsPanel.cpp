/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#include <native/ui/ui_GoToButton.h>
#include <native/ui/ui_ProductAssetDetailsPanel.h>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QStringLiteral>
#include <QUrl>

namespace AssetProcessor
{

    ProductAssetDetailsPanel::ProductAssetDetailsPanel(QWidget* parent) : AssetDetailsPanel(parent), m_ui(new Ui::ProductAssetDetailsPanel)
    {
        m_ui->setupUi(this);
        m_ui->scrollAreaWidgetContents->setLayout(m_ui->scrollableVerticalLayout);
        m_ui->MissingProductDependenciesTable->setColumnWidth(1, 160);
        ResetText();
        connect(m_ui->MissingProductDependenciesSupport, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnSupportClicked);
        connect(m_ui->ScanMissingDependenciesButton, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnScanFileClicked);
        connect(m_ui->ScanFolderButton, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnScanFolderClicked);

        connect(m_ui->ClearMissingDependenciesButton, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnClearScanFileClicked);
        connect(m_ui->ClearScanFolderButton, &QPushButton::clicked, this, &ProductAssetDetailsPanel::OnClearScanFolderClicked);
    }

    ProductAssetDetailsPanel::~ProductAssetDetailsPanel()
    {

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

    void ProductAssetDetailsPanel::AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
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
                GoToSource(sourceEntry.m_sourceName);
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
        // Clear & ClearContents leave the table dimensions the same, so set rowCount to zero to reset it.
        m_ui->outgoingProductDependenciesTable->setRowCount(0);

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
                        m_ui->outgoingProductDependenciesTable->insertRow(productDependencyCount);

                        // Qt handles cleanup automatically, setting this as the parent means
                        // when this panel is torn down, these widgets will be destroyed.
                        GoToButton* rowGoToButton = new GoToButton(this);
                        connect(rowGoToButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                            GoToProduct(product.m_productName);
                        });
                        m_ui->outgoingProductDependenciesTable->setCellWidget(productDependencyCount, 0, rowGoToButton);

                        QTableWidgetItem* rowName = new QTableWidgetItem(product.m_productName.c_str());
                        m_ui->outgoingProductDependenciesTable->setItem(productDependencyCount, 1, rowName);
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

        if (productDependencyCount == 0)
        {
            m_ui->outgoingProductDependenciesTable->insertRow(productDependencyCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("No product dependencies"));
            m_ui->outgoingProductDependenciesTable->setItem(productDependencyCount, 1, rowName);
            ++productDependencyCount;
        }

        if (productPathDependencyCount == 0)
        {
            QListWidgetItem* listWidgetItem = new QListWidgetItem();
            listWidgetItem->setText(tr("No unmet dependencies"));
            m_ui->outgoingUnmetPathProductDependenciesList->addItem(listWidgetItem);
            ++productPathDependencyCount;
        }

        m_ui->outgoingProductDependenciesTable->setMinimumHeight(m_ui->outgoingProductDependenciesTable->rowHeight(0) * productDependencyCount + 2 * m_ui->outgoingProductDependenciesTable->frameWidth());
        m_ui->outgoingProductDependenciesTable->adjustSize();

        m_ui->outgoingUnmetPathProductDependenciesList->setMinimumHeight(m_ui->outgoingUnmetPathProductDependenciesList->sizeHintForRow(0) * productPathDependencyCount + 2 * m_ui->outgoingUnmetPathProductDependenciesList->frameWidth());
        m_ui->outgoingUnmetPathProductDependenciesList->adjustSize();
    }

    void ProductAssetDetailsPanel::BuildIncomingProductDependencies(
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData,
        const AZ::Data::AssetId& assetId,
        const AZStd::string& platform)
    {
        // Clear & ClearContents leave the table dimensions the same, so set rowCount to zero to reset it.
        m_ui->incomingProductDependenciesTable->setRowCount(0);

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
                m_ui->incomingProductDependenciesTable->insertRow(incomingProductDependencyCount);

                GoToButton* rowGoToButton = new GoToButton(this);
                connect(rowGoToButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                    GoToProduct(incomingDependency.m_productName);
                });
                m_ui->incomingProductDependenciesTable->setCellWidget(incomingProductDependencyCount, 0, rowGoToButton);

                QTableWidgetItem* rowName = new QTableWidgetItem(incomingDependency.m_productName.c_str());
                m_ui->incomingProductDependenciesTable->setItem(incomingProductDependencyCount, 1, rowName);

                ++incomingProductDependencyCount;
            }
            return true;
        });

        m_ui->incomingProductDependenciesValueLabel->setText(QString::number(incomingProductDependencyCount));

        if (incomingProductDependencyCount == 0)
        {
            m_ui->incomingProductDependenciesTable->insertRow(incomingProductDependencyCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("No incoming product dependencies"));
            m_ui->incomingProductDependenciesTable->setItem(incomingProductDependencyCount, 1, rowName);
            ++incomingProductDependencyCount;
        }
        m_ui->incomingProductDependenciesTable->setMinimumHeight(m_ui->incomingProductDependenciesTable->rowHeight(0) * incomingProductDependencyCount + 2 * m_ui->incomingProductDependenciesTable->frameWidth());
        m_ui->incomingProductDependenciesTable->adjustSize();
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
                GoToButton* rowGoToButton = new GoToButton(this);
                connect(rowGoToButton->m_ui->goToPushButton, &QPushButton::clicked, [&, missingDependency] {
                    GoToProduct(missingDependency.m_missingProductName);
                });
                m_ui->MissingProductDependenciesTable->setCellWidget(missingDependencyRowCount, 0, rowGoToButton);
            }

            QTableWidgetItem* scanTime = new QTableWidgetItem(missingDependency.m_databaseEntry.m_lastScanTime.c_str());
            m_ui->MissingProductDependenciesTable->setItem(missingDependencyRowCount, 1, scanTime);

            QTableWidgetItem* rowName = new QTableWidgetItem(missingDependency.m_databaseEntry.m_missingDependencyString.c_str());
            m_ui->MissingProductDependenciesTable->setItem(missingDependencyRowCount, 2, rowName);

            ++missingDependencyRowCount;
        }

        m_ui->MissingProductDependenciesValueLabel->setText(QString::number(missingDependencyCount));

        if (missingDependencyRowCount == 0)
        {
            m_ui->MissingProductDependenciesTable->insertRow(missingDependencyRowCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("File has not been scanned."));
            m_ui->MissingProductDependenciesTable->setItem(missingDependencyRowCount, 1, rowName);
            ++missingDependencyRowCount;
        }
        else
        {
            m_ui->missingDependencyErrorIcon->setVisible(hasMissingDependency);
        }

        m_ui->MissingProductDependenciesTable->setMinimumHeight(m_ui->MissingProductDependenciesTable->rowHeight(0) * missingDependencyRowCount + 2 * m_ui->MissingProductDependenciesTable->frameWidth());
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

        m_ui->outgoingProductDependenciesTitleLabel->setVisible(visible);
        m_ui->outgoingProductDependenciesValueLabel->setVisible(visible);
        m_ui->outgoingProductDependenciesTable->setVisible(visible);

        m_ui->outgoingUnmetPathProductDependenciesTitleLabel->setVisible(visible);
        m_ui->outgoingUnmetPathProductDependenciesValueLabel->setVisible(visible);
        m_ui->outgoingUnmetPathProductDependenciesList->setVisible(visible);

        m_ui->incomingProductDependenciesTitleLabel->setVisible(visible);
        m_ui->incomingProductDependenciesValueLabel->setVisible(visible);
        m_ui->incomingProductDependenciesTable->setVisible(visible);

        m_ui->MissingProductDependenciesTitleLabel->setVisible(visible);
        m_ui->MissingProductDependenciesValueLabel->setVisible(visible);
        m_ui->MissingProductDependenciesTable->setVisible(visible);
        m_ui->MissingProductDependenciesSupport->setVisible(visible);
        m_ui->ScanMissingDependenciesButton->setVisible(visible);
        m_ui->ClearMissingDependenciesButton->setVisible(visible);

        m_ui->DependencySeparatorLine->setVisible(visible);

        m_ui->missingDependencyErrorIcon->setVisible(false);
    }

    void ProductAssetDetailsPanel::OnSupportClicked(bool /*checked*/)
    {
        QDesktopServices::openUrl(
            QStringLiteral("https://o3de.org/docs/user-guide/packaging/asset-bundler/assets-resolving/"));
    }


    void ProductAssetDetailsPanel::OnScanFileClicked(bool /*checked*/)
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
            existingDependencies.push_back();
            existingDependencies.back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });

        QDir cacheRootDir;
        AssetUtilities::ComputeProjectCacheRoot(cacheRootDir);
        QString pathOnDisk = cacheRootDir.filePath(productItemData->m_databaseInfo.m_productName.c_str());

        AddProductIdToScanCount(productItemData->m_databaseInfo.m_productID, scanName);

        // Run the scan on another thread so the UI remains responsive.
        AZStd::thread scanningThread = AZStd::thread([=]() {
            MissingDependencyScannerRequestBus::Broadcast(&MissingDependencyScannerRequestBus::Events::ScanFile,
                pathOnDisk.toUtf8().constData(),
                MissingDependencyScanner::DefaultMaxScanIteration,
                productItemData->m_databaseInfo.m_productID,
                existingDependencies,
                m_assetDatabaseConnection,
                /*queueDbCommandsOnMainThread*/ true,
                [=](AZStd::string /*relativeDependencyFilePath*/) {
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
        });
        scanningThread.detach();
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

    void ProductAssetDetailsPanel::OnScanFolderClicked(bool /*checked*/)
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

    void ProductAssetDetailsPanel::OnClearScanFileClicked(bool /*checked*/)
    {
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(m_currentItem->GetData());
        ClearMissingDependenciesForFile(productItemData);
    }

    void ProductAssetDetailsPanel::OnClearScanFolderClicked(bool /*checked*/)
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

