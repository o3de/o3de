/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Atom/Document/ShaderManagementConsoleDocumentRequestBus.h>

#include <Source/Window/ShaderManagementConsoleBrowserWidget.h>

#include <Source/Window/ui_ShaderManagementConsoleBrowserWidget.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    ShaderManagementConsoleBrowserWidget::ShaderManagementConsoleBrowserWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::ShaderManagementConsoleBrowserWidget)
    {
        using namespace AzToolsFramework::AssetBrowser;

        m_ui->setupUi(this);

        m_ui->m_searchWidget->Setup(true, true);
        m_ui->m_searchWidget->SetFilterState("", AZ::RPI::ShaderAsset::Group, true);
        m_ui->m_searchWidget->setMinimumSize(QSize(150, 0));

        // Get the asset browser model
        AssetBrowserModel* assetBrowserModel = nullptr;
        AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
        AZ_Assert(assetBrowserModel, "Failed to get file browser model");

        // Hook up the data set to the tree view
        m_filterModel = aznew AssetBrowserFilterModel(this);
        m_filterModel->setSourceModel(assetBrowserModel);
        m_filterModel->SetFilter(CreateFilter());

        m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel);
        m_ui->m_assetBrowserTreeViewWidget->SetShowSourceControlIcons(true);
        m_ui->m_assetBrowserTreeViewWidget->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SetName("AssetBrowserTreeView_main");

        connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel, &AssetBrowserFilterModel::filterUpdatedSlot);
        connect(m_filterModel, &AssetBrowserFilterModel::filterChanged, this, [this]()
        {
            const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
            constexpr bool selectFirstFilteredIndex = true;
            m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
        });
        connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::activated, this, &ShaderManagementConsoleBrowserWidget::OpenSelectedEntries);
        connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal, [this]() {
            const auto& selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
            if (!selectedAssets.empty())
            {
                m_ui->m_previewerFrame->Display(selectedAssets.front());
            }
            else
            {
                m_ui->m_previewerFrame->Clear();
            }
        });

        AssetBrowserModelNotificationBus::Handler::BusConnect();
        ShaderManagementConsoleDocumentNotificationBus::Handler::BusConnect();
    }

    ShaderManagementConsoleBrowserWidget::~ShaderManagementConsoleBrowserWidget()
    {
        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SaveState();
        ShaderManagementConsoleDocumentNotificationBus::Handler::BusDisconnect();
        AssetBrowserModelNotificationBus::Handler::BusDisconnect();
    }

    AzToolsFramework::AssetBrowser::FilterConstType ShaderManagementConsoleBrowserWidget::CreateFilter() const
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

    void ShaderManagementConsoleBrowserWidget::OpenSelectedEntries()
    {
        const AZStd::vector<AssetBrowserEntry*> entries = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();

        const int multiSelectPromptThreshold = 10;
        if (entries.size() >= multiSelectPromptThreshold)
        {
            if (QMessageBox::question(
                QApplication::activeWindow(),
                QString("Attemptng to open %1 files").arg(entries.size()),
                "Would you like to open anyway?",
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            {
                return;
            }
        }

        for (const AssetBrowserEntry* entry : entries)
        {
            const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
            if (!sourceEntry)
            {
                const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
                if (productEntry)
                {
                    sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(productEntry->GetParent());
                }
            }

            if (sourceEntry)
            {
                if (AzFramework::StringFunc::Path::IsExtension(sourceEntry->GetFullPath().c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
                {
                    ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::OpenDocument, sourceEntry->GetFullPath());
                }
                else
                {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(sourceEntry->GetFullPath().c_str()));
                }
            }
        }
    }

    void ShaderManagementConsoleBrowserWidget::EntryAdded(const AssetBrowserEntry* entry)
    {
        if (m_pathToSelect.empty())
        {
            return;
        }

        const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
        if (!sourceEntry)
        {
            const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
            if (productEntry)
            {
                sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(productEntry->GetParent());
            }
        }

        if (sourceEntry)
        {
            AZStd::string sourcePath = sourceEntry->GetFullPath();
            AzFramework::StringFunc::Path::Normalize(sourcePath);
            if (m_pathToSelect == sourcePath)
            {
                m_ui->m_assetBrowserTreeViewWidget->SelectFileAtPath(m_pathToSelect);
                m_pathToSelect.clear();
            }
        }
    }

    void ShaderManagementConsoleBrowserWidget::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        AZStd::string absolutePath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(absolutePath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);
        if (!absolutePath.empty())
        {
            m_pathToSelect = absolutePath;
            AzFramework::StringFunc::Path::Normalize(m_pathToSelect);
            m_ui->m_assetBrowserTreeViewWidget->SelectFileAtPath(m_pathToSelect);
        }
    }

} // namespace ShaderManagementConsole

#include <Source/Window/moc_ShaderManagementConsoleBrowserWidget.cpp>
