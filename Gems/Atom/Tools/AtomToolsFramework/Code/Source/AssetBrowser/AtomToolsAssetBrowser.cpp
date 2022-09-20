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

        m_ui->m_viewOptionButton->setIcon(QIcon(":/Icons/menu.svg"));
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
        connect(m_ui->m_viewOptionButton, &QPushButton::clicked, this, &AtomToolsAssetBrowser::OpenOptionsMenu);
        connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel, &AssetBrowserFilterModel::filterUpdatedSlot);
    }

    AtomToolsAssetBrowser::~AtomToolsAssetBrowser()
    {
        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SaveState();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void AtomToolsAssetBrowser::SetFilterState(const AZStd::string& category, const AZStd::string& displayName, bool enabled)
    {
        m_ui->m_searchWidget->SetFilterState(category.c_str(), displayName.c_str(), enabled);
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
        const AZStd::vector<AssetBrowserEntry*> entries = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();

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

    void AtomToolsAssetBrowser::OpenOptionsMenu()
    {
        QMenu menu;
        QAction* action = menu.addAction(tr("Show Asset Preview"), this, &AtomToolsAssetBrowser::TogglePreview);
        action->setCheckable(true);
        action->setChecked(m_ui->m_previewerFrame->isVisible());
        menu.exec(QCursor::pos());
    }

    AzToolsFramework::AssetBrowser::FilterConstType AtomToolsAssetBrowser::CreateFilter() const
    {
        using namespace AzToolsFramework::AssetBrowser;

        QSharedPointer<EntryTypeFilter> sourceFilter(new EntryTypeFilter);
        sourceFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Source);

        QSharedPointer<EntryTypeFilter> folderFilter(new EntryTypeFilter);
        folderFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Folder);

        QSharedPointer<CompositeFilter> sourceOrFolderFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::OR));
        sourceOrFolderFilter->AddFilter(sourceFilter);
        sourceOrFolderFilter->AddFilter(folderFilter);

        QSharedPointer<CompositeFilter> finalFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND));
        finalFilter->AddFilter(sourceOrFolderFilter);
        finalFilter->AddFilter(m_ui->m_searchWidget->GetFilter());

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
