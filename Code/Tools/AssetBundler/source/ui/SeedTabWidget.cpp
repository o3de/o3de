/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/SeedTabWidget.h>
#include <source/ui/ui_SeedTabWidget.h>

#include <source/utils/GUIApplicationManager.h>
#include <source/ui/AddSeedDialog.h>
#include <source/ui/EditSeedDialog.h>
#include <source/ui/NewFileDialog.h>
#include <source/models/SeedListFileTableModel.h>
#include <source/models/SeedListTableModel.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>

#include <QCheckBox>
#include <QFileDialog>
#include <QItemSelection>
#include <QItemSelectionRange>
#include <QMessageBox>
#include <QPushButton>


const char GenerateAssetListFilesDialogName[] = "Generate Asset List Files";

const int CheckBoxTableIndentationSize = 2; // when the indentation is 0, the checkboxes are too close to the edge

namespace AssetBundler
{
    SeedTabWidget::SeedTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager, const QString& assetBundlingDirectory)
        : AssetBundlerTabWidget(parent, guiApplicationManager)
        , m_fileTableModel(new SeedListFileTableModel(this))
        , m_seedListContentsModel(new SeedListTableModel())
    {
        AZ_UNUSED(assetBundlingDirectory);

        m_ui.reset(new Ui::SeedTabWidget);
        m_ui->setupUi(this);

        m_ui->mainVerticalLayout->setContentsMargins(MarginSize, MarginSize, MarginSize, MarginSize);

        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        // File view of all Seed List Files
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
            &SeedTabWidget::FileSelectionChanged);

        m_ui->fileTableView->setIndentation(CheckBoxTableIndentationSize); 

        // New File Button
        connect(m_ui->createNewSeedListButton, &QPushButton::clicked, this, &SeedTabWidget::OnNewFileButtonPressed);

        // Select Default Seed Lists
        connect(m_ui->selectDefaultSeedListsCheckBox, &QCheckBox::clicked, this, &SeedTabWidget::OnSelectDefaultSeedListsCheckBoxChanged);

        // Generate Asset Lists Button
        SetGenerateAssetListsButtonEnabled(false);
        connect(m_ui->generateAssetListsButton, &QPushButton::clicked, this, &SeedTabWidget::OnGenerateAssetListsButtonPressed);

        // Table that displays the contents of a Seed List File
        m_seedListContentsFilterModel.reset(new AssetBundlerFileTableFilterModel(this, SeedListTableModel::Column::ColumnRelativePath));

        m_seedListContentsFilterModel->setSourceModel(m_seedListContentsModel.data());
        m_ui->seedFileContentsTable->setModel(m_seedListContentsFilterModel.data());
        connect(m_ui->seedListContentsFilteredSearchWidget,
            &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
            m_seedListContentsFilterModel.data(),
            static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetBundlerFileTableFilterModel::FilterChanged));

        m_ui->seedFileContentsTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_ui->seedFileContentsTable,
            &QTreeView::customContextMenuRequested,
            this,
            &SeedTabWidget::OnSeedListContentsTableContextMenuRequested);

        m_ui->seedFileContentsTable->setIndentation(0);

        // Edit All Platforms 
        connect(m_ui->editAllSeedsButton, &QPushButton::clicked, this, &SeedTabWidget::OnEditAllButtonPressed);

        // Add Seed
        connect(m_ui->addSeedButton, &QPushButton::clicked, this, &SeedTabWidget::OnAddSeedButtonPressed);

        SetModelDataSource();
    }

    SeedTabWidget::~SeedTabWidget()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool SeedTabWidget::HasUnsavedChanges()
    {
        return m_fileTableModel->HasUnsavedChanges();
    }

    void SeedTabWidget::Reload()
    {
        // Reload all the seed list files
        m_fileTableModel->Reload(
            AzToolsFramework::AssetSeedManager::GetSeedFileExtension(),
            m_watchedFolders,
            m_watchedFiles,
            m_filePathToGemNameMap);

        // Update the selected row
        FileSelectionChanged();
    }

    bool SeedTabWidget::SaveCurrentSelection()
    {
        return m_fileTableModel->Save(m_selectedFileTableIndex);
    }

    bool SeedTabWidget::SaveAll()
    {
        return m_fileTableModel->SaveAll();
    }

    void SeedTabWidget::SetModelDataSource()
    {
        // Remove the current watched folders and files
        m_guiApplicationManager->RemoveWatchedPaths(m_watchedFolders + m_watchedFiles);

        // Set the new watched folders for the model
        m_watchedFolders.clear();
        m_watchedFolders.insert(m_guiApplicationManager->GetSeedListsFolder().c_str());

        // Get the list of default Seed List files
        m_filePathToGemNameMap = AssetBundler::GetDefaultSeedListFiles(
            AZStd::string_view{ AZ::Utils::GetEnginePath() },
            m_guiApplicationManager->GetCurrentProjectName(),
            m_guiApplicationManager->GetGemInfoList(), m_guiApplicationManager->GetEnabledPlatforms());

        // Get the list of default Seeds that are not stored in a Seed List file on-disk
        AZStd::vector<AZStd::string> defaultSeeds =
            GetDefaultSeeds(AZ::Utils::GetProjectPath(), m_guiApplicationManager->GetCurrentProjectName());
        m_fileTableModel->AddDefaultSeedsToInMemoryList(
            defaultSeeds,
            m_guiApplicationManager->GetCurrentProjectName().c_str(),
            m_guiApplicationManager->GetEnabledPlatforms());

        // Set the new watched filess for the model
        m_watchedFiles.clear();
        for (const auto& it : m_filePathToGemNameMap)
        {
            m_watchedFiles.insert(it.first.c_str());
        }

        ReadScanPathsFromAssetBundlerSettings(AssetBundlingFileType::SeedListFileType);
        m_guiApplicationManager->AddWatchedPaths(m_watchedFolders + m_watchedFiles);
    }

    AzQtComponents::TableView* SeedTabWidget::GetFileTableView()
    {
        return m_ui->fileTableView;
    }

    QModelIndex SeedTabWidget::GetSelectedFileTableIndex()
    {
        return m_selectedFileTableIndex;
    }

    AssetBundlerAbstractFileTableModel* SeedTabWidget::GetFileTableModel()
    {
        return m_fileTableModel.get();
    }

    void SeedTabWidget::SetActiveProjectLabel(const QString& labelText)
    {
        m_ui->activeProjectLabel->setText(labelText);
    }

    void SeedTabWidget::ApplyConfig()
    {
        const GUIApplicationManager::Config& config = m_guiApplicationManager->GetConfig();

        m_ui->fileTableFrame->setFixedWidth(config.fileTableWidth);
        m_ui->fileTableView->header()->resizeSection(SeedListFileTableModel::Column::ColumnFileName, config.seedListFileNameColumnWidth);
        m_ui->fileTableView->header()->resizeSection(SeedListFileTableModel::Column::ColumnCheckBox, config.checkBoxColumnWidth);
        m_ui->fileTableView->header()->resizeSection(SeedListFileTableModel::Column::ColumnProject, config.projectNameColumnWidth);

        m_ui->seedFileContentsTable->header()->resizeSection(
            SeedListTableModel::Column::ColumnRelativePath,
            config.seedListContentsNameColumnWidth);
    }

    void SeedTabWidget::UncheckSelectDefaultSeedListsCheckBox()
    {
        m_ui->selectDefaultSeedListsCheckBox->setChecked(false);
    }

    void SeedTabWidget::SetGenerateAssetListsButtonEnabled(bool isEnabled)
    {
        m_ui->generateAssetListsButton->setEnabled(isEnabled);
    }

    bool SeedTabWidget::OnPreError(
        const char* /*window*/,
        const char* /*fileName*/,
        int /*line*/,
        const char* /*func*/,
        const char* /*message*/)
    {
        m_hasWarningsOrErrors = true;
        return false;
    }

    bool SeedTabWidget::OnPreWarning(
        const char* /*window*/,
        const char* /*fileName*/,
        int /*line*/,
        const char* /*func*/,
        const char* /*message*/)
    {
        m_hasWarningsOrErrors = true;
        return false;
    }

    void SeedTabWidget::FileSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
    {
        if (m_ui->fileTableView->selectionModel()->selectedRows().size() == 0)
        {
            // Set selected index to an invalid value
            m_selectedFileTableIndex = QModelIndex();
            m_ui->seedListFileAbsolutePathLabel->clear();
            return;
        }

        m_selectedFileTableIndex = m_fileTableFilterModel->mapToSource(m_ui->fileTableView->selectionModel()->currentIndex());

        m_seedListContentsModel = m_fileTableModel->GetSeedListFileContents(m_selectedFileTableIndex);
        m_seedListContentsFilterModel->setSourceModel(m_seedListContentsModel.data());

        m_ui->seedListFileAbsolutePathLabel->setText(m_fileTableModel->GetFileAbsolutePath(m_selectedFileTableIndex).c_str());
    }

    void SeedTabWidget::OnNewFileButtonPressed()
    {
        AZStd::string absoluteFilePath = NewFileDialog::OSNewFileDialog(
            this,
            AzToolsFramework::AssetSeedManager::GetSeedFileExtension(),
            "Seed List",
            m_guiApplicationManager->GetSeedListsFolder());

        if (absoluteFilePath.empty())
        {
            // User canceled out of the file dialog
            return;
        }

        AZStd::vector<AZStd::string> createdFiles = m_fileTableModel->CreateNewFiles(
            absoluteFilePath,
            AzFramework::PlatformFlags::Platform_NONE,
            QString(m_guiApplicationManager->GetCurrentProjectName().c_str()));

        if (!createdFiles.empty())
        {
            AddScanPathToAssetBundlerSettings(AssetBundlingFileType::SeedListFileType, createdFiles.at(0));
        }
    }

    void SeedTabWidget::OnSelectDefaultSeedListsCheckBoxChanged()
    {
        m_fileTableModel->SelectDefaultSeedLists(m_ui->selectDefaultSeedListsCheckBox->isChecked());
    }

    void SeedTabWidget::OnGenerateAssetListsButtonPressed()
    {
        m_generateAssetListsDialog.reset(
            new NewFileDialog(
                this,
                GenerateAssetListFilesDialogName,
                QString(m_guiApplicationManager->GetAssetListsFolder().c_str()),
                AzToolsFramework::AssetSeedManager::GetAssetListFileExtension(),
                QString(tr("Asset List (*.%1)")).arg(AzToolsFramework::AssetSeedManager::GetAssetListFileExtension()),
                m_guiApplicationManager->GetEnabledPlatforms()));
        auto dialogResponse = m_generateAssetListsDialog->exec();
        if (dialogResponse == QDialog::DialogCode::Rejected)
        {
            // User canceled the operation
            return;
        }

        m_hasWarningsOrErrors = false;
        auto createdFiles = m_fileTableModel->GenerateAssetLists(
            m_generateAssetListsDialog->GetAbsoluteFilePath(),
            m_generateAssetListsDialog->GetPlatformFlags());

        // Warnings will not prevent the generation of Asset List files, we must track them separately
        NewFileDialog::FileGenerationResultMessageBox(this, createdFiles, m_hasWarningsOrErrors);

        if (createdFiles.empty())
        {
            // Error has already been thrown
            return;
        }

        // Add created files to the file watcher
        for (const AZStd::string& absolutePath : createdFiles)
        {
            AddScanPathToAssetBundlerSettings(AssetBundlingFileType::AssetListFileType, absolutePath);
        }

        // The watched files list was updated after the files were created, so we need to force-reload them
        m_guiApplicationManager->UpdateFiles(AssetBundlingFileType::AssetListFileType, createdFiles);
    }

    void SeedTabWidget::OnEditSeedButtonPressed()
    {
        if (!m_selectedFileTableIndex.isValid())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot perform Edit Seed operation: No Seed List File is selected");
            return;
        }

        // Get the current platforms of the selected Seed so we can display them as already checked
        QModelIndex currentSeedIndex =
            m_seedListContentsFilterModel->mapToSource(m_ui->seedFileContentsTable->selectionModel()->currentIndex());
        auto getPlatformOutcome = m_seedListContentsModel->GetSeedPlatforms(currentSeedIndex);
        if (!getPlatformOutcome.IsSuccess())
        {
            // Error has already been thrown
            return;
        }

        // Create and display the Edit Seed dialog
        m_editSeedDialog.reset(new EditSeedDialog(this, m_guiApplicationManager->GetEnabledPlatforms(), getPlatformOutcome.GetValue()));
        auto dialogResponse = m_editSeedDialog->exec();
        if (dialogResponse == QDialog::DialogCode::Rejected)
        {
            // User canceled the operation
            return;
        }

        // Set the data in the model
        m_fileTableModel->SetSeedPlatforms(m_selectedFileTableIndex, currentSeedIndex, m_editSeedDialog->GetPlatformFlags());
    }

    void SeedTabWidget::OnEditAllButtonPressed()
    {
        if (!m_selectedFileTableIndex.isValid())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot perform Edit All operation: No Seed List File is selected");
            return;
        }

        // Get the platforms of all the seeds so we can display them as already checked or partially checked
        AzFramework::PlatformFlags selectedPlatformFlags = AzFramework::PlatformFlags::AllNamedPlatforms;
        AzFramework::PlatformFlags partiallySelectedPlatformFlags = AzFramework::PlatformFlags::Platform_NONE;
        QMap<QModelIndex, AzFramework::PlatformFlags> indexToPlatformFlagsMap;
        for (int row = 0; row < m_seedListContentsModel->rowCount(); ++row)
        {
            QModelIndex currentSeedIndex = m_seedListContentsModel->index(row, 0);
            auto getPlatformOutcome = m_seedListContentsModel->GetSeedPlatforms(currentSeedIndex);
            if (!getPlatformOutcome.IsSuccess())
            {
                // Error has already been thrown
                return;
            }

            indexToPlatformFlagsMap[currentSeedIndex] = getPlatformOutcome.TakeValue();
            selectedPlatformFlags &= indexToPlatformFlagsMap[currentSeedIndex];
            partiallySelectedPlatformFlags |= selectedPlatformFlags ^ indexToPlatformFlagsMap[currentSeedIndex];
        }

        // Create and display the Edit Seed dialog
        m_editSeedDialog.reset(new EditSeedDialog(
            this,
            m_guiApplicationManager->GetEnabledPlatforms(),
            selectedPlatformFlags,
            partiallySelectedPlatformFlags));

        auto dialogResponse = m_editSeedDialog->exec();
        if (dialogResponse == QDialog::DialogCode::Rejected)
        {
            // User canceled the operation
            return;
        }

        // Set the data in the model
        for (int row = 0; row < m_seedListContentsModel->rowCount(); ++row)
        {
            QModelIndex currentSeedIndex = m_seedListContentsModel->index(row, 0);
            AzFramework::PlatformFlags checkedPlatforms = m_editSeedDialog->GetPlatformFlags();
            AzFramework::PlatformFlags partiallyCheckedPlatforms = m_editSeedDialog->GetPartiallySelectedPlatformFlags();
            // If the platform is partially checked, we want to keep its original status when saving the changes
            AzFramework::PlatformFlags platformFlags =
                indexToPlatformFlagsMap[currentSeedIndex] & partiallyCheckedPlatforms | checkedPlatforms;

            m_fileTableModel->SetSeedPlatforms(m_selectedFileTableIndex, currentSeedIndex, platformFlags);
        }
    }

    void SeedTabWidget::OnAddSeedButtonPressed()
    {
        if (!m_selectedFileTableIndex.isValid())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot perform Add Seed operation: No Seed List File is selected");
            return;
        }

        // Get path to the platform-specific cache folder of one of the enabled platforms
        AzFramework::PlatformFlags enabledPlatforms = m_guiApplicationManager->GetEnabledPlatforms();
        AZStd::fixed_vector<AzFramework::PlatformId, AzFramework::PlatformId::NumPlatformIds> enabledPlatformIndices =
            AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(enabledPlatforms);
        AZStd::string platformSpecificCachePath =
            AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(enabledPlatformIndices[0]);

        // Create and display the Add Seed Dialog
        m_addSeedDialog.reset(new AddSeedDialog(this, enabledPlatforms, platformSpecificCachePath));
        auto dialogResponse = m_addSeedDialog->exec();
        if (dialogResponse == QDialog::DialogCode::Rejected)
        {
            // User canceled the operation
            return;
        }

        // Set the data in the model
        m_fileTableModel->AddSeed(m_selectedFileTableIndex, m_addSeedDialog->GetFileName(), m_addSeedDialog->GetPlatformFlags());
    }

    void SeedTabWidget::OnRemoveSeedButtonPressed()
    {
        if (!m_selectedFileTableIndex.isValid())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot perform Remove Seed operation: No Seed List File is selected");
            return;
        }

        // Set the data in the model
        QModelIndex currentSeedIndex =
            m_seedListContentsFilterModel->mapToSource(m_ui->seedFileContentsTable->selectionModel()->currentIndex());
        m_fileTableModel->RemoveSeed(m_selectedFileTableIndex, currentSeedIndex);
    }

    void SeedTabWidget::OnSeedListContentsTableContextMenuRequested(const QPoint& pos)
    {
        if (!m_selectedFileTableIndex.isValid())
        {
            return;
        }

        QMenu* contextMenu = new QMenu(this);
        contextMenu->setToolTipsVisible(true);

        QAction* action = contextMenu->addAction(tr("Edit Platforms"));
        connect(action, &QAction::triggered, this, &SeedTabWidget::OnEditSeedButtonPressed);
        action->setToolTip(tr("Change what platforms are referenced when generating an Asset List file."));

        contextMenu->addSeparator();

        action = contextMenu->addAction(tr("Add Seed"));
        connect(action, &QAction::triggered, this, &SeedTabWidget::OnAddSeedButtonPressed);
        action->setToolTip(tr("Add a new Seed to the Seed List file."));

        action = contextMenu->addAction(tr("Remove Seed"));
        connect(action, &QAction::triggered, this, &SeedTabWidget::OnRemoveSeedButtonPressed);
        action->setToolTip(tr("Removes the Seed from the Seed List file."));

        contextMenu->exec(m_ui->seedFileContentsTable->mapToGlobal(pos));
        delete contextMenu;
    }

} //namespace AssetBundler

#include <source/ui/moc_SeedTabWidget.cpp>
