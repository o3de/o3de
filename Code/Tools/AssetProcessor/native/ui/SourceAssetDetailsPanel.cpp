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
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <native/ui/ui_GoToButton.h>
#include <native/ui/ui_SourceAssetDetailsPanel.h>
#include <native/utilities/assetUtils.h>

namespace AssetProcessor
{
    SourceAssetDetailsPanel::SourceAssetDetailsPanel(QWidget* parent)
        : AssetDetailsPanel(parent)
        , m_ui(new Ui::SourceAssetDetailsPanel)
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
        AssetTreeFilterModel* filterModel = m_isIntermediateAsset ? m_intermediateFilterModel : m_sourceFilterModel;
        QItemSelection sourceSelection = filterModel->mapSelectionToSource(selected);
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

        const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData =
            AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(childItem->GetData());

        m_ui->assetNameLabel->setText(childItem->GetData()->m_name);

        if (childItem->GetData()->m_isFolder || !sourceItemData)
        {
            // Folders don't have details.
            SetDetailsVisible(false);
            return;
        }

        SetDetailsVisible(true);

        AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        if (m_isIntermediateAsset)
        {
            // First, find the product version of this intermediate asset.
            AZStd::string intermediateProductPath = AssetUtilities::GetIntermediateAssetDatabaseName(sourceItemData->m_assetDbName.c_str());
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry upstreamSource;

            assetDatabaseConnection.QueryProductByProductName(
                intermediateProductPath.c_str(),
                [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
                {
                    // Second, get the source that created this product.
                    assetDatabaseConnection.QuerySourceByProductID(
                        productEntry.m_productID,
                        [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
                        {
                            upstreamSource = sourceEntry;
                            return true;
                        });
                    return true;
                });

            // Finally, populate the UI with the information on the source that output this.
            m_ui->gotoAssetButton->m_ui->goToPushButton->disconnect();

            connect(
                m_ui->gotoAssetButton->m_ui->goToPushButton,
                &QPushButton::clicked,
                [=]
                {
                    GoToSource(upstreamSource.m_sourceName);
                });

            m_ui->sourceAssetValueLabel->setText(upstreamSource.m_sourceName.c_str());
        }

        m_ui->scanFolderValueLabel->setText(sourceItemData->m_scanFolderInfo.m_scanFolder.c_str());
        m_ui->sourceGuidValueLabel->setText(sourceItemData->m_sourceInfo.m_sourceGuid.ToString<AZStd::string>().c_str());


        BuildProducts(assetDatabaseConnection, sourceItemData);
        BuildOutgoingSourceDependencies(assetDatabaseConnection, sourceItemData);
        BuildIncomingSourceDependencies(assetDatabaseConnection, sourceItemData);
    }

    void SourceAssetDetailsPanel::BuildProducts(
        AssetDatabaseConnection& assetDatabaseConnection, const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData)
    {
        // Clear & ClearContents leave the table dimensions the same, so set rowCount to zero to reset it.
        m_ui->productTable->setRowCount(0);
        m_ui->IntermediateAssetsTable->setRowCount(0);

        int intermediateAssetCount = 0;
        int productCount = 0;
        assetDatabaseConnection.QueryProductBySourceID(
            sourceItemData->m_sourceInfo.m_sourceID,
            [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
            {
                AZ::s64 sourcePK;
                assetDatabaseConnection.QueryJobByProductID(
                    productEntry.m_productID,
                    [&sourcePK](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
                    {
                        sourcePK = jobEntry.m_sourcePK;
                        return true;
                    });

                if (IsProductOutputFlagSet(productEntry, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset))
                {
                    m_ui->IntermediateAssetsTable->insertRow(intermediateAssetCount);

                    AZStd::string intermediateAssetSourcePath;
                    assetDatabaseConnection.QuerySourceBySourceID(
                        sourcePK,
                        [&intermediateAssetSourcePath](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
                        {
                            intermediateAssetSourcePath = sourceEntry.m_sourceName;
                            return true;
                        });

                    AZStd::string sourceIntermediateAssetPath = AssetUtilities::StripAssetPlatformNoCopy(productEntry.m_productName);

                    // Qt handles cleanup automatically, setting this as the parent means
                    // when this panel is torn down, these widgets will be destroyed.
                    GoToButton* rowGoToButton = new GoToButton(this);
                    connect(
                        rowGoToButton->m_ui->goToPushButton,
                        &QPushButton::clicked,
                        [=]
                        {
                            GoToSource(sourceIntermediateAssetPath);
                        });
                    m_ui->IntermediateAssetsTable->setCellWidget(intermediateAssetCount, 0, rowGoToButton);

                    QTableWidgetItem* rowName = new QTableWidgetItem(productEntry.m_productName.c_str());
                    if (IsProductOutputFlagSet(productEntry, AssetBuilderSDK::ProductOutputFlags::CachedAsset))
                    {
                        rowName->setIcon(QIcon(":/cached_asset_item.png"));
                    }
                    m_ui->IntermediateAssetsTable->setItem(intermediateAssetCount, 1, rowName);
                    ++intermediateAssetCount;
                }
                else
                {
                    m_ui->productTable->insertRow(productCount);

                    // Qt handles cleanup automatically, setting this as the parent means
                    // when this panel is torn down, these widgets will be destroyed.
                    GoToButton* rowGoToButton = new GoToButton(this);
                    connect(
                        rowGoToButton->m_ui->goToPushButton,
                        &QPushButton::clicked,
                        [=]
                        {
                            GoToProduct(productEntry.m_productName);
                        });
                    m_ui->productTable->setCellWidget(productCount, 0, rowGoToButton);

                    QTableWidgetItem* rowName = new QTableWidgetItem(productEntry.m_productName.c_str());
                    if (IsProductOutputFlagSet(productEntry, AssetBuilderSDK::ProductOutputFlags::CachedAsset))
                    {
                        rowName->setIcon(QIcon(":/cached_asset_item.png"));
                    }
                    m_ui->productTable->setItem(productCount, 1, rowName);
                    ++productCount;
                }
                return true;
            });

        m_ui->SourceAssetDetailTabs->setTabText(0, tr("Products (%1)").arg(productCount));
        m_ui->SourceAssetDetailTabs->setTabVisible(0, productCount > 0);

        m_ui->SourceAssetDetailTabs->setTabText(3, tr("Intermediate Assets (%1)").arg(intermediateAssetCount));
        m_ui->SourceAssetDetailTabs->setTabVisible(3, intermediateAssetCount > 0);
    }

    void SourceAssetDetailsPanel::BuildOutgoingSourceDependencies(
        AssetDatabaseConnection& assetDatabaseConnection, const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData)
    {
        m_ui->outgoingSourceDependenciesTable->setRowCount(0);
        int sourceDependencyCount = 0;
        assetDatabaseConnection.QueryDependsOnSourceBySourceDependency(
            sourceItemData->m_sourceInfo.m_sourceGuid,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry)
            {
                m_ui->outgoingSourceDependenciesTable->insertRow(sourceDependencyCount);

                // Some outgoing source dependencies are wildcard, or unresolved paths.
                // Only add a button to link to rows that actually exist.

                AzToolsFramework::AssetDatabase::SourceDatabaseEntry dependencyDetails;
                AZStd::string displayString;

                if (sourceFileDependencyEntry.m_dependsOnSource.IsUuid())
                {
                    assetDatabaseConnection.QuerySourceBySourceGuid(
                        sourceFileDependencyEntry.m_dependsOnSource.GetUuid(),
                        [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            dependencyDetails = entry;
                            displayString = dependencyDetails.m_sourceName;
                            return false;
                        });

                    if (displayString.empty())
                    {
                        displayString = sourceFileDependencyEntry.m_dependsOnSource.GetUuid().ToFixedString();
                    }
                }
                else
                {
                    assetDatabaseConnection.QuerySourceBySourceName(
                        sourceFileDependencyEntry.m_dependsOnSource.GetPath().c_str(),
                        [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
                        {
                            dependencyDetails = sourceEntry;
                            displayString = dependencyDetails.m_sourceName;
                            return false;
                        });

                    if (displayString.empty())
                    {
                        displayString = sourceFileDependencyEntry.m_dependsOnSource.GetPath();
                    }
                }

                SourceAssetTreeModel* treeModel = dependencyDetails.m_scanFolderPK == 1 ? m_intermediateTreeModel : m_sourceTreeModel;
                QModelIndex goToIndex = treeModel->GetIndexForSource(dependencyDetails.m_sourceName);
                if (goToIndex.isValid())
                {
                    // Qt handles cleanup automatically, setting this as the parent means
                    // when this panel is torn down, these widgets will be destroyed.
                    GoToButton* rowGoToButton = new GoToButton(this);
                    connect(
                        rowGoToButton->m_ui->goToPushButton,
                        &QPushButton::clicked,
                        [=]
                        {
                            GoToSource(dependencyDetails.m_sourceName);
                        });
                    m_ui->outgoingSourceDependenciesTable->setCellWidget(sourceDependencyCount, 0, rowGoToButton);
                }

                QTableWidgetItem* rowName = new QTableWidgetItem(displayString.c_str());
                m_ui->outgoingSourceDependenciesTable->setItem(sourceDependencyCount, 1, rowName);
                ++sourceDependencyCount;
                return true;
            });

        m_ui->SourceAssetDetailTabs->setTabText(1, tr("Dependencies - Out (%1)").arg(sourceDependencyCount));
        m_ui->SourceAssetDetailTabs->setTabVisible(1, sourceDependencyCount > 0);
    }

    void SourceAssetDetailsPanel::BuildIncomingSourceDependencies(
        AssetDatabaseConnection& assetDatabaseConnection, const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData)
    {
        m_ui->incomingSourceDependenciesTable->setRowCount(0);
        int sourceDependencyCount = 0;

        auto absolutePath = AZ::IO::Path(sourceItemData->m_scanFolderInfo.m_scanFolder) / sourceItemData->m_sourceInfo.m_sourceName;

        assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
            sourceItemData->m_sourceInfo.m_sourceGuid,
            sourceItemData->m_sourceInfo.m_sourceName.c_str(),
            absolutePath.FixedMaxPathStringAsPosix().c_str(),
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry)
            {
                m_ui->incomingSourceDependenciesTable->insertRow(sourceDependencyCount);

                AZStd::string sourceName;
                assetDatabaseConnection.QuerySourceBySourceGuid(
                    sourceFileDependencyEntry.m_sourceGuid,
                    [&sourceName](const auto& entry)
                    {
                        sourceName = entry.m_sourceName;
                        return false;
                    });

                // Qt handles cleanup automatically, setting this as the parent means
                // when this panel is torn down, these widgets will be destroyed.
                GoToButton* rowGoToButton = new GoToButton(this);
                connect(
                    rowGoToButton->m_ui->goToPushButton,
                    &QPushButton::clicked,
                    [this, sourceName]
                    {
                        GoToSource(sourceName);
                    });
                m_ui->incomingSourceDependenciesTable->setCellWidget(sourceDependencyCount, 0, rowGoToButton);

                QTableWidgetItem* rowName = new QTableWidgetItem(QString(sourceName.c_str()));
                m_ui->incomingSourceDependenciesTable->setItem(sourceDependencyCount, 1, rowName);

                ++sourceDependencyCount;
                return true;
            });

        m_ui->SourceAssetDetailTabs->setTabText(2, tr("Dependencies - In (%1)").arg(sourceDependencyCount));
        m_ui->SourceAssetDetailTabs->setTabVisible(2, sourceDependencyCount > 0);
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

        m_ui->sourceAssetTitleLabel->setVisible(visible && m_isIntermediateAsset);
        m_ui->gotoAssetButton->setVisible(visible && m_isIntermediateAsset);
        m_ui->sourceAssetValueLabel->setVisible(visible && m_isIntermediateAsset);

        m_ui->AssetInfoSeparatorLine->setVisible(visible);

        m_ui->SourceAssetDetailTabs->setVisible(visible);
    }
} // namespace AssetProcessor
