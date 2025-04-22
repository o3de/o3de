/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/RulesTabWidget.h>
#include <source/ui/ui_RulesTabWidget.h>

#include <source/models/RulesFileTableModel.h>
#include <source/ui/ComparisonDataWidget.h>
#include <source/ui/NewFileDialog.h>
#include <source/utils/GUIApplicationManager.h>
#include <source/utils/utils.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzToolsFramework/AssetBundle/AssetBundleAPI.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include <QMenu>
#include <QPushButton>


namespace AssetBundler
{
    RulesTabWidget::RulesTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager)
        : AssetBundlerTabWidget(parent, guiApplicationManager)
    {
        m_ui.reset(new Ui::RulesTabWidget);
        m_ui->setupUi(this);

        m_ui->mainVerticalLayout->setContentsMargins(MarginSize, MarginSize, MarginSize, MarginSize);

        m_fileTableModel.reset(new RulesFileTableModel);
        m_ui->fileTableView->setModel(m_fileTableModel.data());

        // Table View of all Rules files
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
            &RulesTabWidget::FileSelectionChanged);

        m_ui->fileTableView->setIndentation(0);

        // New File Button
        connect(m_ui->createNewFileButton, &QPushButton::clicked, this, &RulesTabWidget::OnNewFileButtonPressed);

        // Run Rule Button
        m_ui->runRuleButton->setEnabled(false);
        connect(m_ui->runRuleButton, &QPushButton::clicked, this, &RulesTabWidget::OnRunRuleButtonPressed);

        // Add Comparison Step button
        connect(m_ui->addComparisonStepButton, &QPushButton::clicked, this, &RulesTabWidget::AddNewComparisonStep);

        SetModelDataSource();
    }

    bool RulesTabWidget::HasUnsavedChanges()
    {
        return m_fileTableModel->HasUnsavedChanges();
    }

    void RulesTabWidget::Reload()
    {
        m_fileTableModel->Reload(
            AzToolsFramework::AssetFileInfoListComparison::GetComparisonRulesFileExtension(),
            m_watchedFolders,
            m_watchedFiles);
        FileSelectionChanged();
    }

    bool RulesTabWidget::SaveCurrentSelection()
    {
        return m_fileTableModel->Save(m_selectedFileTableIndex);
    }

    bool RulesTabWidget::SaveAll()
    {
         return m_fileTableModel->SaveAll();
    }

    void RulesTabWidget::SetModelDataSource()
    {
        // Remove the current watched folders and files
        m_guiApplicationManager->RemoveWatchedPaths(m_watchedFolders + m_watchedFiles);

        // Set the new watched folder for the model
        m_watchedFolders.clear();
        m_watchedFiles.clear();
        m_watchedFolders.insert(m_guiApplicationManager->GetRulesFolder().c_str());
        ReadScanPathsFromAssetBundlerSettings(AssetBundlingFileType::RulesFileType);

        m_guiApplicationManager->AddWatchedPaths(m_watchedFolders + m_watchedFiles);
    }

    AzQtComponents::TableView* RulesTabWidget::GetFileTableView()
    {
        return m_ui->fileTableView;
    }

    QModelIndex RulesTabWidget::GetSelectedFileTableIndex()
    {
        return m_selectedFileTableIndex;
    }

    AssetBundlerAbstractFileTableModel* RulesTabWidget::GetFileTableModel()
    {
        return m_fileTableModel.get();
    }

    void RulesTabWidget::SetActiveProjectLabel(const QString& labelText)
    {
        m_ui->activeProjectLabel->setText(labelText);
    }

    void RulesTabWidget::ApplyConfig()
    {
        const GUIApplicationManager::Config& config = m_guiApplicationManager->GetConfig();
        m_ui->fileTableFrame->setFixedWidth(config.fileTableWidth);
        m_ui->fileTableView->header()->resizeSection(RulesFileTableModel::Column::ColumnFileName, config.fileNameColumnWidth);
    }


    void RulesTabWidget::FileSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
    {
        if (m_ui->fileTableView->selectionModel()->selectedRows().size() == 0)
        {
            m_selectedFileTableIndex = QModelIndex();
            m_selectedComparisonRules = nullptr;
            m_ui->runRuleButton->setEnabled(false);
            m_ui->addComparisonStepButton->setEnabled(false);
            m_ui->rulesFileAbsolutePathLabel->clear();
            RemoveAllComparisonDataCards();

            return;
        }

        m_ui->runRuleButton->setEnabled(true);
        m_ui->addComparisonStepButton->setEnabled(true);

        m_selectedFileTableIndex = m_fileTableFilterModel->mapToSource(m_ui->fileTableView->selectionModel()->selectedRows()[0]);
        m_selectedComparisonRules = m_fileTableModel->GetComparisonSteps(m_selectedFileTableIndex);

        m_ui->rulesFileAbsolutePathLabel->setText(m_fileTableModel->GetFileAbsolutePath(m_selectedFileTableIndex).c_str());

        RebuildComparisonDataCardList();
    }

    void RulesTabWidget::OnNewFileButtonPressed()
    {
        AZStd::string absoluteFilePath = NewFileDialog::OSNewFileDialog(
            this,
            AzToolsFramework::AssetFileInfoListComparison::GetComparisonRulesFileExtension(),
            "Comparison Rules",
            m_guiApplicationManager->GetRulesFolder());

        if (absoluteFilePath.empty())
        {
            // User canceled out of the dialog
            return;
        }

        AZStd::vector<AZStd::string> createdFile = m_fileTableModel->CreateNewFiles(absoluteFilePath);
        if (!createdFile.empty())
        {
            AddScanPathToAssetBundlerSettings(AssetBundlingFileType::RulesFileType, createdFile.at(0));
        }
    }

    void RulesTabWidget::OnRunRuleButtonPressed()
    {
        using namespace AzToolsFramework;

        // Determine which platforms all of the input Asset List files have in common
        AzFramework::PlatformFlags commonPlatforms = AzFramework::PlatformFlags::AllNamedPlatforms;
        for (const AssetFileInfoListComparison::ComparisonData& comparisonStep : m_selectedComparisonRules->GetComparisonList())
        {
            if (!comparisonStep.m_cachedFirstInputPath.empty())
            {
                commonPlatforms = commonPlatforms & GetPlatformsOnDiskForPlatformSpecificFile(comparisonStep.m_cachedFirstInputPath);
            }

            if (!comparisonStep.m_cachedSecondInputPath.empty())
            {
                commonPlatforms = commonPlatforms & GetPlatformsOnDiskForPlatformSpecificFile(comparisonStep.m_cachedSecondInputPath);
            }
        }

        if (commonPlatforms == AzFramework::PlatformFlags::Platform_NONE)
        {
            AZ_Error("AssetBundler", false, "Unable to run Rule: Input Asset List files have no platforms in common.");
            return;
        }

        // Prompt the user to select an output path and the platforms to run the rule on
        NewFileDialog runRuleDialog = NewFileDialog(
            this,
            "Run Rule",
            QString(m_guiApplicationManager->GetAssetListsFolder().c_str()),
            AzToolsFramework::AssetSeedManager::GetAssetListFileExtension(),
            QString(tr("Asset List (*.%1)")).arg(AzToolsFramework::AssetSeedManager::GetAssetListFileExtension()),
            commonPlatforms,
            true);

        auto dialogResponse = runRuleDialog.exec();
        if (dialogResponse == QDialog::DialogCode::Rejected)
        {
            // User canceled the operation
            return;
        }

        AZStd::vector<AZStd::string> outputFilePaths;
        bool hasFileGenerationErrors = false;

        AZStd::fixed_vector<AZStd::string, AzFramework::NumPlatforms> selectedPlatformNames{ AZStd::from_range,
            AzFramework::PlatformHelper::GetPlatforms(runRuleDialog.GetPlatformFlags()) };
        for (const AZStd::string& platformName : selectedPlatformNames)
        {
            // We do not want to modify the original Rules file, as we do not save Asset List file paths to disk
            AssetFileInfoListComparison currentFileCopy = *m_selectedComparisonRules.get();

            //Update the first and second input values with any non-token Asset List file paths that have been set, 
            size_t numComparisonSteps = currentFileCopy.GetNumComparisonSteps();
            for (size_t comparisonStepIndex = 0; comparisonStepIndex < numComparisonSteps; ++comparisonStepIndex)
            {
                AssetFileInfoListComparison::ComparisonData comparisonStep = currentFileCopy.GetComparisonList()[comparisonStepIndex];
                if (comparisonStep.m_firstInput.empty())
                {
                    if (comparisonStep.m_cachedFirstInputPath.empty())
                    {
                        AZ_Error("AssetBundler", false,
                            "Unable to run Rule: Comparison Step #%u has no specified first input.", comparisonStepIndex);
                        return;
                    }

                    FilePath firstInput = FilePath(comparisonStep.m_cachedFirstInputPath, platformName);
                    currentFileCopy.SetFirstInput(comparisonStepIndex, firstInput.AbsolutePath());
                }

                if (comparisonStep.m_comparisonType != AssetFileInfoListComparison::ComparisonType::FilePattern
                    && comparisonStep.m_secondInput.empty())
                {
                    if (comparisonStep.m_cachedSecondInputPath.empty())
                    {
                        AZ_Error("AssetBundler", false,
                            "Unable to run Rule: Comparison Step #%u has no specified second input.", comparisonStepIndex);
                        return;
                    }

                    FilePath secondInput = FilePath(comparisonStep.m_cachedSecondInputPath, platformName);
                    currentFileCopy.SetSecondInput(comparisonStepIndex, secondInput.AbsolutePath());
                }
            }

            // Set the output location of the Asset List file that will be generated
            FilePath finalOutputPath = FilePath(runRuleDialog.GetAbsoluteFilePath(), platformName);
            currentFileCopy.SetOutput(numComparisonSteps - 1, finalOutputPath.AbsolutePath());

            //Run the Rule
            auto compareOutcome = currentFileCopy.CompareAndSaveResults();
            if (compareOutcome.IsSuccess())
            {
                outputFilePaths.emplace_back(finalOutputPath.AbsolutePath());
            }
            else
            {
                hasFileGenerationErrors = true;
                AZ_Error("AssetBundler", false, compareOutcome.GetError().c_str());
            }
        }

        // Add created files to the file watcher
        for (const AZStd::string& absolutePath : outputFilePaths)
        {
            AddScanPathToAssetBundlerSettings(AssetBundlingFileType::AssetListFileType, absolutePath);
        }

        // The watched files list was updated after the files were created, so we need to force-reload them
        m_guiApplicationManager->UpdateFiles(AssetBundlingFileType::AssetListFileType, outputFilePaths);

        NewFileDialog::FileGenerationResultMessageBox(this, outputFilePaths, hasFileGenerationErrors);
    }

    void RulesTabWidget::MarkFileChanged()
    {
        m_fileTableModel->MarkFileChanged(m_selectedFileTableIndex);
    }

    void RulesTabWidget::RebuildComparisonDataCardList()
    {
        RemoveAllComparisonDataCards();
        PopulateComparisonDataCardList();
    }

    void RulesTabWidget::PopulateComparisonDataCardList()
    {
        if (!m_selectedComparisonRules)
        {
            return;
        }

        for (int index = 0; index < m_selectedComparisonRules->GetComparisonList().size(); ++index)
        {
            CreateComparisonDataCard(m_selectedComparisonRules, static_cast<size_t>(index));
        }
    }

    void RulesTabWidget::CreateComparisonDataCard(
        AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> comparisonList,
        size_t comparisonDataIndex)
    {
        ComparisonDataCard* comparisonDataCard = new ComparisonDataCard(
            comparisonList,
            comparisonDataIndex,
            m_guiApplicationManager->GetAssetListsFolder());
        comparisonDataCard->setTitle(tr("Step %1").arg(static_cast<int>(comparisonDataIndex) + 1));
        m_ui->comparisonDataListLayout->addWidget(comparisonDataCard);
        m_comparisonDataCardList.push_back(comparisonDataCard);

        ComparisonDataWidget* comparisonDataWidget = comparisonDataCard->GetComparisonDataWidget();
        connect(comparisonDataCard,
            &ComparisonDataCard::comparisonDataCardContextMenuRequested,
            this,
            &RulesTabWidget::OnComparisonDataCardContextMenuRequested);
        connect(comparisonDataWidget, &ComparisonDataWidget::comparisonDataChanged, this, &RulesTabWidget::MarkFileChanged);
        connect(comparisonDataWidget,
            &ComparisonDataWidget::comparisonDataTokenNameChanged,
            this,
            &RulesTabWidget::OnAnyTokenNameChanged);

        comparisonDataCard->show();
    }

    void RulesTabWidget::RemoveAllComparisonDataCards()
    {
        m_comparisonDataCardList.clear();

        while (!m_ui->comparisonDataListLayout->isEmpty())
        {
            QLayoutItem* item = m_ui->comparisonDataListLayout->takeAt(0);
            item->widget()->hide();
            delete item;
        }
    }

    void RulesTabWidget::AddNewComparisonStep()
    {
        if (!m_selectedComparisonRules)
        {
            return;
        }

        if (!m_selectedComparisonRules->AddComparisonStep(AzToolsFramework::AssetFileInfoListComparison::ComparisonData()))
        {
            return;
        }

        CreateComparisonDataCard(m_selectedComparisonRules, m_selectedComparisonRules->GetComparisonList().size() - 1);
        MarkFileChanged();
    }

    void RulesTabWidget::RemoveComparisonStep(size_t comparisonDataIndex)
    {
        if (!m_selectedComparisonRules)
        {
            return;
        }

        if (m_selectedComparisonRules->RemoveComparisonStep(comparisonDataIndex))
        {
            MarkFileChanged();
            RebuildComparisonDataCardList();
        }
    }

    void RulesTabWidget::MoveComparisonStep(size_t startingIndex, size_t destinationIndex)
    {
        if (!m_selectedComparisonRules)
        {
            return;
        }

        if (m_selectedComparisonRules->MoveComparisonStep(startingIndex, destinationIndex))
        {
            MarkFileChanged();
            RebuildComparisonDataCardList();
        }
    }

    void RulesTabWidget::OnAnyTokenNameChanged(size_t comparisonDataIndex)
    {
        if (comparisonDataIndex >= m_comparisonDataCardList.size() - 1)
        {
            return;
        }

        for (size_t i = comparisonDataIndex + 1; i < m_comparisonDataCardList.size(); ++i)
        {
            m_comparisonDataCardList[i]->GetComparisonDataWidget()->UpdateListOfTokenNames();
        }
    }

    void RulesTabWidget::OnComparisonDataCardContextMenuRequested(size_t comparisonDataIndex, const QPoint& position)
    {
        if (!m_selectedComparisonRules)
        {
            return;
        }

        QMenu menu;

        QAction* moveUpAction = new QAction(tr("Move Up"), this);
        moveUpAction->setEnabled(comparisonDataIndex > 0);
        connect(moveUpAction, &QAction::triggered, this, [=]() { MoveComparisonStep(comparisonDataIndex, comparisonDataIndex - 1); });
        menu.addAction(moveUpAction);

        QAction* moveDownAction = new QAction(tr("Move Down"), this);
        moveDownAction->setEnabled(comparisonDataIndex < m_selectedComparisonRules->GetNumComparisonSteps() - 1);
        connect(moveDownAction, &QAction::triggered, this, [=]() { MoveComparisonStep(comparisonDataIndex, comparisonDataIndex + 2); });
        menu.addAction(moveDownAction);

        QAction* separator = new QAction(this);
        separator->setSeparator(true);
        menu.addAction(separator);

        QAction* deleteAction = new QAction(tr("Remove Comparison Step"), this);
        connect(deleteAction, &QAction::triggered, this, [=]() { RemoveComparisonStep(comparisonDataIndex); });
        menu.addAction(deleteAction);

        menu.exec(position);
    }

} // namespace AssetBundler

#include <source/ui/moc_RulesTabWidget.cpp>
