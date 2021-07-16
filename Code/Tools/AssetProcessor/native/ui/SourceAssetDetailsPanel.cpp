/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SourceAssetDetailsPanel.h"

#include "AssetTreeFilterModel.h"
#include "GoToButton.h"
#include "SourceAssetTreeItemData.h"
#include "SourceAssetTreeModel.h"


#include <native/ui/ui_GoToButton.h>
#include <native/ui/ui_SourceAssetDetailsPanel.h>

namespace AssetProcessor
{

    SourceAssetDetailsPanel::SourceAssetDetailsPanel(QWidget* parent) : AssetDetailsPanel(parent), m_ui(new Ui::SourceAssetDetailsPanel)
    {
        m_ui->setupUi(this);
        m_ui->scrollAreaWidgetContents->setLayout(m_ui->scrollableVerticalLayout);
        ResetText();
    }

    SourceAssetDetailsPanel::~SourceAssetDetailsPanel()
    {

    }

    void SourceAssetDetailsPanel::AssetDataSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
    {
        QItemSelection sourceSelection = m_sourceFilterModel->mapSelectionToSource(selected);
        // Even if multi-select is enabled, only display the first selected item.
        if (sourceSelection.indexes().count() == 0 || !sourceSelection.indexes()[0].isValid())
        {
            ResetText();
            return;
        }

        QModelIndex sourceModelIndex = sourceSelection.indexes()[0];

        if (!sourceModelIndex.isValid())
        {
            return;
        }
        AssetTreeItem* childItem = static_cast<AssetTreeItem*>(sourceModelIndex.internalPointer());

        const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(childItem->GetData());

        m_ui->assetNameLabel->setText(childItem->GetData()->m_name);

        if (childItem->GetData()->m_isFolder || !sourceItemData)
        {
            // Folders don't have details.
            SetDetailsVisible(false);
            return;
        }
        SetDetailsVisible(true);
        m_ui->scanFolderValueLabel->setText(sourceItemData->m_scanFolderInfo.m_scanFolder.c_str());
        m_ui->sourceGuidValueLabel->setText(sourceItemData->m_sourceInfo.m_sourceGuid.ToString<AZStd::string>().c_str());

        AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        BuildProducts(assetDatabaseConnection, sourceItemData);
        BuildOutgoingSourceDependencies(assetDatabaseConnection, sourceItemData);
        BuildIncomingSourceDependencies(assetDatabaseConnection, sourceItemData);
    }

    void SourceAssetDetailsPanel::BuildProducts(
        AssetDatabaseConnection& assetDatabaseConnection,
        const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData)
    {
        // Clear & ClearContents leave the table dimensions the same, so set rowCount to zero to reset it.
        m_ui->productTable->setRowCount(0);

        int productCount = 0;
        assetDatabaseConnection.QueryProductBySourceID(
            sourceItemData->m_sourceInfo.m_sourceID,
            [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
        {
            m_ui->productTable->insertRow(productCount);

            // Qt handles cleanup automatically, setting this as the parent means
            // when this panel is torn down, these widgets will be destroyed.
            GoToButton* rowGoToButton = new GoToButton(this);
            connect(rowGoToButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                GoToProduct(productEntry.m_productName);
            });
            m_ui->productTable->setCellWidget(productCount, 0, rowGoToButton);

            QTableWidgetItem* rowName = new QTableWidgetItem(productEntry.m_productName.c_str());
            m_ui->productTable->setItem(productCount, 1, rowName);
            ++productCount;
            return true;
        });

        m_ui->productsValueLabel->setText(QString::number(productCount));
        if (productCount == 0)
        {
            m_ui->productTable->insertRow(productCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("No products"));
            m_ui->productTable->setItem(productCount, 1, rowName);
            ++productCount;
        }

        // The default list behavior is to maintain size and let you scroll within.
        // The entire frame is scrollable here, so the list should adjust to fit the contents.
        m_ui->productTable->setMinimumHeight(m_ui->productTable->rowHeight(0) * productCount + 2 * m_ui->productTable->frameWidth());
        m_ui->productTable->adjustSize();

    }

    void SourceAssetDetailsPanel::BuildOutgoingSourceDependencies(
        AssetDatabaseConnection& assetDatabaseConnection,
        const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData)
    {
        m_ui->outgoingSourceDependenciesTable->setRowCount(0);
        int sourceDependencyCount = 0;
        assetDatabaseConnection.QueryDependsOnSourceBySourceDependency(
            sourceItemData->m_sourceInfo.m_sourceName.c_str(),
            nullptr,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry)
        {
            m_ui->outgoingSourceDependenciesTable->insertRow(sourceDependencyCount);

            // Some outgoing source dependencies are wildcard, or unresolved paths.
            // Only add a button to link to rows that actually exist.
            QModelIndex goToIndex = m_sourceTreeModel->GetIndexForSource(sourceFileDependencyEntry.m_dependsOnSource);
            if (goToIndex.isValid())
            {
                // Qt handles cleanup automatically, setting this as the parent means
                // when this panel is torn down, these widgets will be destroyed.
                GoToButton* rowGoToButton = new GoToButton(this);
                connect(rowGoToButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                    GoToSource(sourceFileDependencyEntry.m_dependsOnSource);
                });
                m_ui->outgoingSourceDependenciesTable->setCellWidget(sourceDependencyCount, 0, rowGoToButton);
            }

            QTableWidgetItem* rowName = new QTableWidgetItem(sourceFileDependencyEntry.m_dependsOnSource.c_str());
            m_ui->outgoingSourceDependenciesTable->setItem(sourceDependencyCount, 1, rowName);
            ++sourceDependencyCount;
            return true;
        });

        m_ui->outgoingSourceDependenciesValueLabel->setText(QString::number(sourceDependencyCount));
        if (sourceDependencyCount == 0)
        {
            m_ui->outgoingSourceDependenciesTable->insertRow(sourceDependencyCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("No source dependencies"));
            m_ui->outgoingSourceDependenciesTable->setItem(sourceDependencyCount, 1, rowName);
            ++sourceDependencyCount;
        }

        // The default list behavior is to maintain size and let you scroll within.
        // The entire frame is scrollable here, so the list should adjust to fit the contents.
        m_ui->outgoingSourceDependenciesTable->setMinimumHeight(m_ui->outgoingSourceDependenciesTable->rowHeight(0) * sourceDependencyCount + 2 * m_ui->outgoingSourceDependenciesTable->frameWidth());
        m_ui->outgoingSourceDependenciesTable->adjustSize();
    }

    void SourceAssetDetailsPanel::BuildIncomingSourceDependencies(
        AssetDatabaseConnection& assetDatabaseConnection,
        const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData)
    {
        m_ui->incomingSourceDependenciesTable->setRowCount(0);
        int sourceDependencyCount = 0;
        assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
            sourceItemData->m_sourceInfo.m_sourceName.c_str(),
            nullptr,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry)
        {
            m_ui->incomingSourceDependenciesTable->insertRow(sourceDependencyCount);

            // Qt handles cleanup automatically, setting this as the parent means
            // when this panel is torn down, these widgets will be destroyed.
            GoToButton* rowGoToButton = new GoToButton(this);
            connect(rowGoToButton->m_ui->goToPushButton, &QPushButton::clicked, [=] {
                GoToSource(sourceFileDependencyEntry.m_source);
            });
            m_ui->incomingSourceDependenciesTable->setCellWidget(sourceDependencyCount, 0, rowGoToButton);

            QTableWidgetItem* rowName = new QTableWidgetItem(sourceFileDependencyEntry.m_source.c_str());
            m_ui->incomingSourceDependenciesTable->setItem(sourceDependencyCount, 1, rowName);

            ++sourceDependencyCount;
            return true;
        });

        m_ui->incomingSourceDependenciesValueLabel->setText(QString::number(sourceDependencyCount));
        if (sourceDependencyCount == 0)
        {
            m_ui->incomingSourceDependenciesTable->insertRow(sourceDependencyCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr("No source dependencies"));
            m_ui->incomingSourceDependenciesTable->setItem(sourceDependencyCount, 1, rowName);
            ++sourceDependencyCount;
        }

        // The default list behavior is to maintain size and let you scroll within.
        // The entire frame is scrollable here, so the list should adjust to fit the contents.
        m_ui->incomingSourceDependenciesTable->setMinimumHeight(m_ui->incomingSourceDependenciesTable->rowHeight(0) * sourceDependencyCount + 2 * m_ui->incomingSourceDependenciesTable->frameWidth());
        m_ui->incomingSourceDependenciesTable->adjustSize();

    }

    void SourceAssetDetailsPanel::ResetText()
    {
        m_ui->assetNameLabel->setText(tr("Select an asset to see details"));
        SetDetailsVisible(false);
    }

    void SourceAssetDetailsPanel::SetDetailsVisible(bool visible)
    {
        // The folder selected description has opposite visibility from everything else.
        m_ui->folderSelectedDescription->setVisible(!visible);

        m_ui->scanFolderTitleLabel->setVisible(visible);
        m_ui->scanFolderValueLabel->setVisible(visible);

        m_ui->sourceGuidTitleLabel->setVisible(visible);
        m_ui->sourceGuidValueLabel->setVisible(visible);

        m_ui->productsTitleLabel->setVisible(visible);
        m_ui->productsValueLabel->setVisible(visible);
        m_ui->productTable->setVisible(visible);

        m_ui->outgoingSourceDependenciesTitleLabel->setVisible(visible);
        m_ui->outgoingSourceDependenciesValueLabel->setVisible(visible);
        m_ui->outgoingSourceDependenciesTable->setVisible(visible);

        m_ui->incomingSourceDependenciesTitleLabel->setVisible(visible);
        m_ui->incomingSourceDependenciesValueLabel->setVisible(visible);
        m_ui->incomingSourceDependenciesTable->setVisible(visible);

        m_ui->DependencySeparatorLine->setVisible(visible);
    }
}

