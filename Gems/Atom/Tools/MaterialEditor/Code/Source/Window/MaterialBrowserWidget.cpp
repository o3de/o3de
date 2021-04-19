/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <Atom/Document/MaterialDocumentRequestBus.h>

#include <Source/Window/MaterialBrowserWidget.h>

#include <Source/Window/ui_MaterialBrowserWidget.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QCursor>
#include <QPushButton>
#include <QList>
#include <QByteArray>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    MaterialBrowserWidget::MaterialBrowserWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::MaterialBrowserWidget)
    {
        using namespace AzToolsFramework::AssetBrowser;

        m_ui->setupUi(this);

        m_ui->m_searchWidget->Setup(true, true);
        m_ui->m_searchWidget->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_ui->m_searchWidget->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);
        m_ui->m_searchWidget->setMinimumSize(QSize(150, 0));
        m_ui->m_viewOptionButton->setIcon(QIcon(":/Icons/View.svg"));
        m_ui->m_splitter->setSizes(QList<int>() << 400 << 200);
        m_ui->m_splitter->setStretchFactor(0, 1);

        connect(m_ui->m_viewOptionButton, &QPushButton::clicked, this, &MaterialBrowserWidget::OpenOptionsMenu);

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

        connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel, &AssetBrowserFilterModel::filterUpdatedSlot);
        connect(m_filterModel, &AssetBrowserFilterModel::filterChanged, this, [this]()
        {
            const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
            constexpr bool selectFirstFilteredIndex = true;
            m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
        });
        connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::activated, this, &MaterialBrowserWidget::OpenSelectedEntries);
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
        MaterialDocumentNotificationBus::Handler::BusConnect();
    }

    MaterialBrowserWidget::~MaterialBrowserWidget()
    {
        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SaveState();
        MaterialDocumentNotificationBus::Handler::BusDisconnect();
        AssetBrowserModelNotificationBus::Handler::BusDisconnect();
    }

    AzToolsFramework::AssetBrowser::FilterConstType MaterialBrowserWidget::CreateFilter() const
    {
        using namespace AzToolsFramework::AssetBrowser;

        // Material Browser uses the following filters:
        // 1. [All source files (no products) that contain products matching the assetType specified by searchWidget (default is materials and textures)]
        // 2. [All folders (including empty folders)]
        // 3. [All Sources and folders matching the search text typed in search widget]
        // Final filter = ((1 OR 2) AND 3)

        QSharedPointer<EntryTypeFilter> sourceFilter(new EntryTypeFilter);
        sourceFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Source);

        QSharedPointer<CompositeFilter> assetTypeFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND));
        assetTypeFilter->AddFilter(sourceFilter);
        assetTypeFilter->AddFilter(m_ui->m_searchWidget->GetTypesFilter());

        QSharedPointer<EntryTypeFilter> folderFilter(new EntryTypeFilter);
        folderFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Folder);

        QSharedPointer<CompositeFilter> sourceOrFolderFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::OR));
        sourceOrFolderFilter->AddFilter(assetTypeFilter);
        sourceOrFolderFilter->AddFilter(folderFilter);

        QSharedPointer<CompositeFilter> finalFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND));
        finalFilter->AddFilter(sourceOrFolderFilter);
        finalFilter->AddFilter(m_ui->m_searchWidget->GetStringFilter());

        return finalFilter;
    }

    void MaterialBrowserWidget::OpenSelectedEntries()
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
                if (AzFramework::StringFunc::Path::IsExtension(sourceEntry->GetFullPath().c_str(), MaterialExtension))
                {
                    MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::OpenDocument, sourceEntry->GetFullPath());
                }
                else if (AzFramework::StringFunc::Path::IsExtension(sourceEntry->GetFullPath().c_str(), MaterialTypeExtension))
                {
                    //ignore MaterialTypeExtension
                }
                else
                {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(sourceEntry->GetFullPath().c_str()));
                }
            }
        }
    }

    void MaterialBrowserWidget::EntryAdded(const AssetBrowserEntry* entry)
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

    void MaterialBrowserWidget::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        AZStd::string absolutePath;
        MaterialDocumentRequestBus::EventResult(absolutePath, documentId, &MaterialDocumentRequestBus::Events::GetAbsolutePath);
        if (!absolutePath.empty())
        {
            m_pathToSelect = absolutePath;
            AzFramework::StringFunc::Path::Normalize(m_pathToSelect);
            m_ui->m_assetBrowserTreeViewWidget->SelectFileAtPath(m_pathToSelect);
        }
    }

    void MaterialBrowserWidget::OpenOptionsMenu()
    {
        QMenu menu;

        QAction* action = new QAction("Show Asset Preview", this);
        action->setCheckable(true);
        action->setChecked(m_ui->m_previewerFrame->isVisible());
        connect(action, &QAction::triggered, [this]() {
            bool isPreviewFrameVisible = m_ui->m_previewerFrame->isVisible();
            m_ui->m_previewerFrame->setVisible(!isPreviewFrameVisible);
            if (isPreviewFrameVisible)
            {
                m_materialBrowserState = m_ui->m_splitter->saveState();
                m_ui->m_splitter->setSizes(QList({ 1, 0 }));
            }
            else
            {
                m_ui->m_splitter->restoreState(m_materialBrowserState);
            }
        });
        menu.addAction(action);
        menu.exec(QCursor::pos());
    }

} // namespace MaterialEditor

#include <Source/Window/moc_MaterialBrowserWidget.cpp>
