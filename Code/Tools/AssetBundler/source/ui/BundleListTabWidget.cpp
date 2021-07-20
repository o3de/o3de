/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <source/ui/BundleListTabWidget.h>
#include <source/ui/ui_BundleListTabWidget.h>

#include <source/models/BundleFileListModel.h>
#include <source/utils/GUIApplicationManager.h>

#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <QFileDialog>
#include <QItemSelectionModel>

const double BytesToMegabytes = 1024.0 * 1024.0;

namespace AssetBundler
{

    BundleListTabWidget::BundleListTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager)
        : AssetBundlerTabWidget(parent, guiApplicationManager)
    {
        m_ui.reset(new Ui::BundleListTabWidget);
        m_ui->setupUi(this);

        m_ui->mainVerticalLayout->setContentsMargins(MarginSize, MarginSize, MarginSize, MarginSize);

        m_fileTableModel.reset(new BundleFileListModel);
        m_fileTableFilterModel.reset(new AssetBundlerFileTableFilterModel(
            this,
            m_fileTableModel->GetFileNameColumnIndex(),
            m_fileTableModel->GetTimeStampColumnIndex()));

        m_fileTableFilterModel->setSourceModel(m_fileTableModel.data());
        m_ui->fileTableView->setModel(m_fileTableFilterModel.data());
        connect(m_ui->fileFilteredSearchWidget,
            &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
            m_fileTableFilterModel.data(),
            static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetBundlerFileTableFilterModel::FilterChanged));

        connect(m_ui->fileTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &BundleListTabWidget::FileSelectionChanged);

        m_ui->fileTableView->setIndentation(0);

        m_relatedBundlesListModel.reset(new QStringListModel);
        m_ui->relatedBundlesListView->setModel(m_relatedBundlesListModel.data());

        m_ui->bundleFileContentsVerticalLayout->setContentsMargins(MarginSize, MarginSize, MarginSize, MarginSize);

        SetModelDataSource();
    }

    void BundleListTabWidget::Reload()
    {
        // The act of cracking open paks kicks off a DirectoryChanged event. We need to temporarily remove the Bundles
        // directory from our watched paths to prevent an infinite loop of events. 
        m_guiApplicationManager->RemoveWatchedPaths(m_watchedFolders);

        // Reload all the bundle files
        m_fileTableModel->Reload(AzToolsFramework::AssetBundleSettings::GetBundleFileExtension(), m_watchedFolders, m_watchedFiles);

        // Update the selected row
        FileSelectionChanged();

        // Start recieving DirectoryChanged events for these folders again
        m_guiApplicationManager->AddWatchedPaths(m_watchedFolders);
    }

    void BundleListTabWidget::SetModelDataSource()
    {
        // Remove the current watched folders and files
        m_guiApplicationManager->RemoveWatchedPaths(m_watchedFolders + m_watchedFiles);

        // Set the new watched folder for the model
        m_watchedFolders.clear();
        m_watchedFiles.clear();
        m_watchedFolders.insert(m_guiApplicationManager->GetBundlesFolder().c_str());
        ReadScanPathsFromAssetBundlerSettings(AssetBundlingFileType::BundleFileType);

        m_guiApplicationManager->AddWatchedPaths(m_watchedFolders + m_watchedFiles);
    }

    AzQtComponents::TableView* BundleListTabWidget::GetFileTableView()
    {
        return m_ui->fileTableView;
    }

    QModelIndex BundleListTabWidget::GetSelectedFileTableIndex()
    {
        return m_selectedFileTableIndex;
    }

    AssetBundlerAbstractFileTableModel* BundleListTabWidget::GetFileTableModel()
    {
        return m_fileTableModel.get();
    }

    void BundleListTabWidget::SetActiveProjectLabel(const QString& labelText)
    {
        m_ui->activeProjectLabel->setText(labelText);
    }

    void BundleListTabWidget::ApplyConfig()
    {
        const GUIApplicationManager::Config& config = m_guiApplicationManager->GetConfig();
        m_ui->fileTableFrame->setFixedWidth(config.fileTableWidth);
        m_ui->fileTableView->header()->resizeSection(BundleFileListModel::Column::ColumnFileName, config.fileNameColumnWidth);
    }


    void BundleListTabWidget::FileSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
    {
        if (m_ui->fileTableView->selectionModel()->selectedRows().size() == 0)
        {
            m_selectedFileTableIndex = QModelIndex();
            ClearDisplayedBundleValues();
            return;
        }

        m_selectedFileTableIndex = m_fileTableFilterModel->mapToSource(m_ui->fileTableView->selectionModel()->selectedRows()[0]);
        AZ::Outcome< BundleFileInfoPtr, void> fileInfoOutcome = m_fileTableModel->GetBundleInfo(m_selectedFileTableIndex);

        if (!fileInfoOutcome.IsSuccess())
        {
            ClearDisplayedBundleValues();
            return;
        }

        BundleFileInfoPtr fileInfoPtr = fileInfoOutcome.GetValue();

        m_ui->absolutePathLabel->setText(fileInfoPtr->m_absolutePath.c_str());

        QString sizeFormat("%1 MB");
        double compressedSize = (double)fileInfoPtr->m_compressedSize / BytesToMegabytes;
        m_ui->compressedSizeValueLabel->setText(sizeFormat.arg(compressedSize, 6, 'f', 3));

        bool hasRelatedBundles = !fileInfoPtr->m_relatedBundles.isEmpty();
        if (hasRelatedBundles)
        {
            m_relatedBundlesListModel->setStringList(fileInfoPtr->m_relatedBundles);
        }
        m_ui->relatedBundlesLabel->setVisible(hasRelatedBundles);
        m_ui->relatedBundlesListView->setVisible(hasRelatedBundles);
    }

    void BundleListTabWidget::ClearDisplayedBundleValues()
    {
        m_ui->absolutePathLabel->clear();
        m_ui->compressedSizeValueLabel->clear();
        m_ui->relatedBundlesLabel->setVisible(false);
        m_ui->relatedBundlesListView->setVisible(false);
        m_relatedBundlesListModel->setStringList({});
    }
}

#include <source/ui/moc_BundleListTabWidget.cpp>
