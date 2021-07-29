/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/AssetListTabWidget.h>
#include <source/ui/ui_AssetListTabWidget.h>

#include <source/utils/GUIApplicationManager.h>
#include <source/ui/GenerateBundlesModal.h>
#include <source/ui/NewFileDialog.h>
#include <source/models/AssetListFileTableModel.h>
#include <source/models/AssetListTableModel.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>

#include <QItemSelection>
#include <QItemSelectionRange>
#include <QMessageBox>
#include <QPushButton>

namespace AssetBundler
{
    AssetListTabWidget::AssetListTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager)
        : AssetBundlerTabWidget(parent, guiApplicationManager)
        , m_fileTableModel(new AssetListFileTableModel())
        , m_assetListContentsModel(new AssetListTableModel())
    {
        m_ui.reset(new Ui::AssetListTabWidget);
        m_ui->setupUi(this);

        m_ui->mainVerticalLayout->setContentsMargins(10, 10, 10, 10);

        // File view of all Asset List Files
        m_fileTableFilterModel.reset(new AssetBundlerFileTableFilterModel(
            this,
            m_fileTableModel->GetFileNameColumnIndex(),
            m_fileTableModel->GetTimeStampColumnIndex()));

        m_fileTableFilterModel->setSourceModel(m_fileTableModel.data());
        m_ui->assetListsTable->setModel(m_fileTableFilterModel.data());
        connect(m_ui->fileFilteredSearchWidget,
            &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
            m_fileTableFilterModel.data(),
            static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetBundlerFileTableFilterModel::FilterChanged));

        connect(m_ui->assetListsTable->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &AssetListTabWidget::FileSelectionChanged);

        m_ui->fileTableHeaderLayout->setContentsMargins(0, 0, 0, 0);
        m_ui->fileTableVerticalLayout->setContentsMargins(0, 0, 0, 0);
        m_ui->assetListsTable->setIndentation(0);

        // Generate Bundle Button
        m_ui->generateBundleButton->setDefault(true);
        m_ui->generateBundleButton->setEnabled(false);
        connect(m_ui->generateBundleButton, &QPushButton::clicked, this, &AssetListTabWidget::OnGenerateBundleButtonPressed);

        // Absolute path of selected Asset List file
        m_ui->assetListFileAbsolutePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        // Table that displays the contents of an Asset List File
        m_assetListContentsFilterModel.reset(new AssetBundlerFileTableFilterModel(this, AssetListTableModel::Column::ColumnAssetName));

        m_assetListContentsFilterModel->setSourceModel(m_assetListContentsModel.data());
        m_ui->assetListContentsTable->setModel(m_assetListContentsFilterModel.data());
        connect(m_ui->assetListContentsFilteredSearchWidget,
            &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
            m_assetListContentsFilterModel.data(),
            static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetBundlerFileTableFilterModel::FilterChanged));


        m_ui->fileContentsHeaderLayout->setContentsMargins(0, 0, 0, 0);
        m_ui->fileContentsVerticalLayout->setContentsMargins(0, 0, 0, 0);
        m_ui->assetListContentsTable->setIndentation(0);

        SetModelDataSource();
    }

    void AssetListTabWidget::SetModelDataSource()
    {
        // Remove the current watched folders and files
        m_guiApplicationManager->RemoveWatchedPaths(m_watchedFolders + m_watchedFiles);

        // Set the new watched folder for the model
        m_watchedFolders.clear();
        m_watchedFiles.clear();
        m_watchedFolders.insert(m_guiApplicationManager->GetAssetListsFolder().c_str());
        ReadScanPathsFromAssetBundlerSettings(AssetBundlingFileType::AssetListFileType);

        m_guiApplicationManager->AddWatchedPaths(m_watchedFolders + m_watchedFiles);
    }

    AzQtComponents::TableView* AssetListTabWidget::GetFileTableView()
    {
        return m_ui->assetListsTable;
    }

    QModelIndex AssetListTabWidget::GetSelectedFileTableIndex()
    {
        return m_selectedFileTableIndex;
    }

    AssetBundlerAbstractFileTableModel* AssetListTabWidget::GetFileTableModel()
    {
        return m_fileTableModel.get();
    }

    void AssetListTabWidget::SetActiveProjectLabel(const QString& labelText)
    {
        m_ui->activeProjectLabel->setText(labelText);
    }

    void AssetListTabWidget::ApplyConfig()
    {
        const GUIApplicationManager::Config& config = m_guiApplicationManager->GetConfig();

        m_ui->fileTableFrame->setFixedWidth(config.fileTableWidth);

        m_ui->assetListsTable->header()->resizeSection(
            AssetListFileTableModel::Column::ColumnFileName,
            config.assetListFileNameColumnWidth);
        m_ui->assetListsTable->header()->resizeSection(
            AssetListFileTableModel::Column::ColumnPlatform,
            config.assetListPlatformColumnWidth);

        m_ui->assetListContentsFilteredSearchWidget->setFixedWidth(config.fileTableWidth);

        m_ui->assetListContentsTable->header()->resizeSection(
            AssetListTableModel::Column::ColumnAssetName,
            config.productAssetNameColumnWidth);
        m_ui->assetListContentsTable->header()->resizeSection(
            AssetListTableModel::Column::ColumnRelativePath,
            config.productAssetRelativePathColumnWidth);
    }


    void AssetListTabWidget::Reload()
    {
        // Reload all the asset list files
        m_fileTableModel->Reload(AzToolsFramework::AssetSeedManager::GetAssetListFileExtension(), m_watchedFolders, m_watchedFiles);

        // Update the selected row
        FileSelectionChanged();
    }

    void AssetListTabWidget::FileSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        AZ_UNUSED(selected);
        AZ_UNUSED(deselected);

        if (m_ui->assetListsTable->selectionModel()->selectedRows().size() == 0)
        {
            // Set selected index to an invalid value
            m_selectedFileTableIndex = QModelIndex();
            m_ui->assetListFileAbsolutePathLabel->setText("");
            m_assetListContentsModel.reset(new AssetListTableModel());
            m_assetListContentsFilterModel->setSourceModel(m_assetListContentsModel.data());
            m_ui->generateBundleButton->setEnabled(false);

            return;
        }

        m_selectedFileTableIndex = m_fileTableFilterModel->mapToSource(m_ui->assetListsTable->selectionModel()->currentIndex());

        m_ui->assetListFileAbsolutePathLabel->setText(QString(m_fileTableModel->GetFileAbsolutePath(m_selectedFileTableIndex).c_str()));

        m_assetListContentsModel = m_fileTableModel->GetAssetListFileContents(m_selectedFileTableIndex);
        m_assetListContentsFilterModel->setSourceModel(m_assetListContentsModel.data());

        m_ui->generateBundleButton->setEnabled(true);
    }

    void AssetListTabWidget::OnGenerateBundleButtonPressed()
    {
        GenerateBundlesModal generateBundlesModal(
            this,
            m_fileTableModel->GetFileAbsolutePath(m_selectedFileTableIndex),
            m_guiApplicationManager->GetBundlesFolder(),
            m_guiApplicationManager->GetBundleSettingsFolder(),
            this);
        generateBundlesModal.exec();
    }
} //namespace AssetBundler

#include <source/ui/moc_AssetListTabWidget.cpp>
