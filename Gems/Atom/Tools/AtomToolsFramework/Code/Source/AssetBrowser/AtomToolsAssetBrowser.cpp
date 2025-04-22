/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBrowser/ui_AtomToolsAssetBrowser.h>
#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowser.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/sort.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QAction>
#include <QCursor>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    AtomToolsAssetBrowser::AtomToolsAssetBrowser(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::AtomToolsAssetBrowser)
    {
        using namespace AzToolsFramework::AssetBrowser;

        m_ui->setupUi(this);

        m_ui->m_searchWidget->Setup(true, true);
        m_ui->m_searchWidget->setMinimumSize(QSize(150, 0));

        m_ui->m_splitter->setSizes(QList<int>() << 400 << 200);
        m_ui->m_splitter->setStretchFactor(0, 1);

        // Get the asset browser model
        AssetBrowserModel* assetBrowserModel = nullptr;
        AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
        AZ_Assert(assetBrowserModel, "Failed to get file browser model");

        // Hook up the data set to the tree view
        m_filterModel = aznew AssetBrowserFilterModel(this);
        m_filterModel->setSourceModel(assetBrowserModel);
        m_filterModel->SetFilter(CreateFilter());

        m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel);
        m_ui->m_assetBrowserTreeViewWidget->SetShowSourceControlIcons(false);
        m_ui->m_assetBrowserTreeViewWidget->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SetName("AssetBrowserTreeView_main");

        connect(m_filterModel, &AssetBrowserFilterModel::filterChanged, this, &AtomToolsAssetBrowser::UpdateFilter);
        connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::activated, this, &AtomToolsAssetBrowser::OpenSelectedEntries);
        connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal, this, &AtomToolsAssetBrowser::UpdatePreview);
        connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel, &AssetBrowserFilterModel::filterUpdatedSlot);

        InitOptionsMenu();
        InitSettingsHandler();
        InitSettings();
    }

    AtomToolsAssetBrowser::~AtomToolsAssetBrowser()
    {
        // Disconnect the event handler before saving settings so that it does not get triggered from the destructor.
        m_settingsNotifyEventHandler.Disconnect();

        // Rewrite any potentially unsaved settings to the registry.
        SaveSettings();

        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SaveState();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    AzToolsFramework::AssetBrowser::SearchWidget* AtomToolsAssetBrowser::GetSearchWidget()
    {
        return m_ui->m_searchWidget;
    }

    void AtomToolsAssetBrowser::SetFileTypeFilters(const FileTypeFilterVec& fileTypeFilters)
    {
        m_fileTypeFilters = fileTypeFilters;

        // Pre sort the file type filters just so that they are organized alphabetically in the menu.
        AZStd::sort(
            m_fileTypeFilters.begin(),
            m_fileTypeFilters.end(),
            [](const auto& fileTypeFilter1, const auto& fileTypeFilter2)
            {
                return fileTypeFilter1.m_name < fileTypeFilter2.m_name;
            });

        UpdateFileTypeFilters();
    }

    void AtomToolsAssetBrowser::UpdateFileTypeFilters()
    {
        m_fileTypeFiltersEnabled = AZStd::any_of(
            m_fileTypeFilters.begin(),
            m_fileTypeFilters.end(),
            [](const auto& fileTypeFilter)
            {
                return fileTypeFilter.m_enabled;
            });
    }

    void AtomToolsAssetBrowser::SetOpenHandler(AZStd::function<void(const AZStd::string&)> openHandler)
    {
        m_openHandler = openHandler;
    }

    void AtomToolsAssetBrowser::SelectEntries(const AZStd::string& absolutePath)
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        m_pathToSelect = absolutePath;
        if (ValidateDocumentPath(m_pathToSelect))
        {
            // Selecting a new asset in the browser is not guaranteed to happen immediately.
            // The asset browser model notifications are sent before the model is updated.
            // Instead of relying on the notifications, queue the selection and process it on tick until this change occurs.
            AZ::SystemTickBus::Handler::BusConnect();
        }
    }

    void AtomToolsAssetBrowser::OpenSelectedEntries()
    {
        const AZStd::vector<const AssetBrowserEntry*> entries = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();

        const bool promptToOpenMultipleFiles =
            GetSettingsValue<bool>("/O3DE/AtomToolsFramework/AssetBrowser/PromptToOpenMultipleFiles", true);
        const AZ::u64 promptToOpenMultipleFilesThreshold =
            GetSettingsValue<AZ::u64>("/O3DE/AtomToolsFramework/AssetBrowser/PromptToOpenMultipleFilesThreshold", 10);

        if (promptToOpenMultipleFiles && promptToOpenMultipleFilesThreshold <= entries.size())
        {
            QMessageBox::StandardButton result = QMessageBox::question(
                GetToolMainWindow(),
                tr("Attemptng to open %1 files").arg(entries.size()),
                tr("Would you like to open anyway?"),
                QMessageBox::Yes | QMessageBox::No);
            if (result == QMessageBox::No)
            {
                return;
            }
        }

        for (const AssetBrowserEntry* entry : entries)
        {
            if (entry && entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Folder && m_openHandler)
            {
                m_openHandler(entry->GetFullPath().c_str());
            }
        }
    }

    AzToolsFramework::AssetBrowser::FilterConstType AtomToolsAssetBrowser::CreateFilter() const
    {
        using namespace AzToolsFramework::AssetBrowser;

        auto filterFn = [this](const AssetBrowserEntry* entry)
        {
            switch (entry->GetEntryType())
            {
            case AssetBrowserEntry::AssetEntryType::Folder:
                {
                    if (const auto& path = entry->GetFullPath(); !IsPathIgnored(path))
                    {
                        return m_showEmptyFolders;
                    }

                    // The path is invalid or ignored
                    return false;
                }

            case AssetBrowserEntry::AssetEntryType::Source:
                {
                    if (const auto& path = entry->GetFullPath(); !IsPathIgnored(path))
                    {
                        // Filter assets against supported extensions instead of using asset type comparisons
                        if (m_fileTypeFiltersEnabled)
                        {
                            for (const auto& fileTypeFilter : m_fileTypeFilters)
                            {
                                if (fileTypeFilter.m_enabled)
                                {
                                    for (const auto& extension : fileTypeFilter.m_extensions)
                                    {
                                        if (AZ::StringFunc::EndsWith(path, extension))
                                        {
                                            return true;
                                        }
                                    }
                                }
                            }

                            // Filters were enabled but no matching filter was found so exclude this entry.
                            return false;
                        }

                        // Filters were not enabled so automatically include all entries.
                        return true;
                    }

                    // The path is invalid or ignored
                    return false;
                }
            }

            return false;
        };

        // The custom filter uses a lambda or function pointer instead of combining complicated filter logic operations. The filter must
        // propagate down in order to support showing and hiding empty folders.
        QSharedPointer<CustomFilter> customFilter(new CustomFilter(filterFn));
        customFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);

        QSharedPointer<CompositeFilter> finalFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND));
        finalFilter->AddFilter(m_ui->m_searchWidget->GetFilter());
        finalFilter->AddFilter(customFilter);
        return finalFilter;
    }

    void AtomToolsAssetBrowser::UpdateFilter()
    {
        const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
        constexpr bool selectFirstFilteredIndex = true;
        m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
    }

    void AtomToolsAssetBrowser::UpdatePreview()
    {
        const auto& selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
        if (!selectedAssets.empty())
        {
            m_ui->m_previewerFrame->Display(selectedAssets.front());
        }
        else
        {
            m_ui->m_previewerFrame->Clear();
        }
    }

    void AtomToolsAssetBrowser::TogglePreview()
    {
        const bool isPreviewFrameVisible = m_ui->m_previewerFrame->isVisible();
        m_ui->m_previewerFrame->setVisible(!isPreviewFrameVisible);
        if (isPreviewFrameVisible)
        {
            m_browserState = m_ui->m_splitter->saveState();
            m_ui->m_splitter->setSizes(QList({ 1, 0 }));
        }
        else
        {
            m_ui->m_splitter->restoreState(m_browserState);
        }
    }

    void AtomToolsAssetBrowser::InitOptionsMenu()
    {
        // Create pop-up menu to toggle the visibility of the asset browser preview window
        m_optionsMenu = new QMenu(this);
        QMenu::connect(
            m_optionsMenu,
            &QMenu::aboutToShow,
            [this]()
            {
                // Register action to toggle showing and hiding the asset preview image
                m_optionsMenu->clear();
                QAction* action = m_optionsMenu->addAction(tr("Show Asset Preview"), this, &AtomToolsAssetBrowser::TogglePreview);
                action->setCheckable(true);
                action->setChecked(m_ui->m_previewerFrame->isVisible());

                // Register action to toggle showing and hiding folders with no visible children
                m_optionsMenu->addSeparator();
                QAction* emptyFolderAction = m_optionsMenu->addAction(
                    tr("Show Empty Folders"),
                    this,
                    [this]()
                    {
                        m_showEmptyFolders = !m_showEmptyFolders;
                        m_filterModel->filterUpdatedSlot();
                    });
                emptyFolderAction->setCheckable(true);
                emptyFolderAction->setChecked(m_showEmptyFolders);

                // Register actions to toggle showing and hiding asset browser entries matching supported extensions
                if (!m_fileTypeFilters.empty())
                {
                    m_optionsMenu->addSeparator();
                    m_optionsMenu->addAction(
                        tr("Enable All File Filters"),
                        this,
                        [this]()
                        {
                            for (auto& fileTypeFilter : m_fileTypeFilters)
                            {
                                fileTypeFilter.m_enabled = true;
                            }
                            UpdateFileTypeFilters();
                            m_filterModel->filterUpdatedSlot();
                        });

                    m_optionsMenu->addAction(
                        tr("Disable All File Filters"),
                        this,
                        [this]()
                        {
                            for (auto& fileTypeFilter : m_fileTypeFilters)
                            {
                                fileTypeFilter.m_enabled = false;
                            }
                            UpdateFileTypeFilters();
                            m_filterModel->filterUpdatedSlot();
                        });
                    m_optionsMenu->addSeparator();

                    for (const auto& fileTypeFilter : m_fileTypeFilters)
                    {
                        QAction* extensionAction = m_optionsMenu->addAction(
                            tr("Show %1 Files").arg(fileTypeFilter.m_name.c_str()),
                            this,
                            [this, fileTypeFilterName = fileTypeFilter.m_name]()
                            {
                                auto fileTypeFilterItr = AZStd::find_if(
                                    m_fileTypeFilters.begin(),
                                    m_fileTypeFilters.end(),
                                    [fileTypeFilterName](const auto& fileTypeFilter)
                                    {
                                        return fileTypeFilter.m_name == fileTypeFilterName;
                                    });
                                if (fileTypeFilterItr != m_fileTypeFilters.end())
                                {
                                    fileTypeFilterItr->m_enabled = !fileTypeFilterItr->m_enabled;
                                }
                                UpdateFileTypeFilters();
                                m_filterModel->filterUpdatedSlot();
                            });
                        extensionAction->setCheckable(true);
                        extensionAction->setChecked(fileTypeFilter.m_enabled);
                    }
                }
            });

        m_ui->m_viewOptionButton->setMenu(m_optionsMenu);
        m_ui->m_viewOptionButton->setIcon(QIcon(":/Icons/menu.svg"));
        m_ui->m_viewOptionButton->setPopupMode(QToolButton::InstantPopup);
    }

    void AtomToolsAssetBrowser::InitSettingsHandler()
    {
        // Monitor for asset browser related settings changes
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            m_settingsNotifyEventHandler = registry->RegisterNotifier(
                [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
                {
                    // Refresh the asset browser model if any of the filter related settings change.
                    if (AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(
                            "/O3DE/AtomToolsFramework/Application/IgnoreCacheFolder", notifyEventArgs.m_jsonKeyPath) ||
                        AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(
                            "/O3DE/AtomToolsFramework/Application/IgnoredPathRegexPatterns", notifyEventArgs.m_jsonKeyPath))
                    {
                        m_filterModel->filterUpdatedSlot();
                    }
                });
        }
    }

    void AtomToolsAssetBrowser::InitSettings()
    {
        // Restoring enabled state for registered file type filters.
        const auto& fileTypeFilterStateMap =
            GetSettingsObject("/O3DE/AtomToolsFramework/AssetBrowser/FileTypeFilterStateMap", AZStd::unordered_map<AZStd::string, bool>{});
        for (const auto& fileTypeFilterStatePair : fileTypeFilterStateMap)
        {
            auto fileTypeFilterItr = AZStd::find_if(
                m_fileTypeFilters.begin(),
                m_fileTypeFilters.end(),
                [fileTypeFilterStatePair](const auto& fileTypeFilter)
                {
                    return fileTypeFilter.m_name == fileTypeFilterStatePair.first;
                });
            if (fileTypeFilterItr != m_fileTypeFilters.end())
            {
                fileTypeFilterItr->m_enabled = fileTypeFilterStatePair.second;
            }
        }
        m_showEmptyFolders = GetSettingsValue("/O3DE/AtomToolsFramework/AssetBrowser/ShowEmptyFolders", false);
        UpdateFileTypeFilters();
    }

    void AtomToolsAssetBrowser::SaveSettings()
    {
        // Record the enabled state for each of the file type filters
        AZStd::unordered_map<AZStd::string, bool> fileTypeFilterStateMap;
        for (const auto& fileTypeFilter : m_fileTypeFilters)
        {
            fileTypeFilterStateMap[fileTypeFilter.m_name] = fileTypeFilter.m_enabled;
        }
        SetSettingsObject("/O3DE/AtomToolsFramework/AssetBrowser/FileTypeFilterStateMap", fileTypeFilterStateMap);
        SetSettingsValue("/O3DE/AtomToolsFramework/AssetBrowser/ShowEmptyFolders", m_showEmptyFolders);
    }

    void AtomToolsAssetBrowser::OnSystemTick()
    {
        if (!ValidateDocumentPath(m_pathToSelect))
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            m_pathToSelect.clear();
            return;
        }

        // Attempt to select the new path
        m_ui->m_assetBrowserTreeViewWidget->SelectFileAtPath(m_pathToSelect);

        // Iterate over the selected entries to verify if the selection was made
        for (const AssetBrowserEntry* entry : m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets())
        {
            if (entry)
            {
                AZStd::string sourcePath = entry->GetFullPath();
                if (ValidateDocumentPath(sourcePath) && AZ::StringFunc::Equal(m_pathToSelect, sourcePath))
                {
                    // Once the selection is confirmed, cancel the operation and disconnect
                    AZ::SystemTickBus::Handler::BusDisconnect();
                    m_pathToSelect.clear();
                }
            }
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/AssetBrowser/moc_AtomToolsAssetBrowser.cpp>
