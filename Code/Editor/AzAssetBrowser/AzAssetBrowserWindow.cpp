/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AzAssetBrowserWindow.h"

// AzToolsFramework
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

// AzQtComponents
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

// Editor
#include "AzAssetBrowser/AzAssetBrowserRequestHandler.h"
#include "LyViewPaneNames.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AzAssetBrowser/ui_AzAssetBrowserWindow.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


class ListenerForShowAssetEditorEvent
    : public QObject
    , private AzToolsFramework::EditorEvents::Bus::Handler
{
public:
    ListenerForShowAssetEditorEvent(QObject* parent = nullptr)
        : QObject(parent)
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    ~ListenerForShowAssetEditorEvent()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void SelectAsset(const QString& assetPath) override
    {
        AzToolsFramework::OpenViewPane(LyViewPane::AssetBrowser);

        AzAssetBrowserWindow* assetBrowser = AzToolsFramework::GetViewPaneWidget<AzAssetBrowserWindow>(LyViewPane::AssetBrowser);
        if (assetBrowser)
        {
            AzQtComponents::bringWindowToTop(assetBrowser);

            assetBrowser->SelectAsset(assetPath);
        }
    }
};

AzAssetBrowserWindow::AzAssetBrowserWindow(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::AzAssetBrowserWindowClass())
    , m_filterModel(new AzToolsFramework::AssetBrowser::AssetBrowserFilterModel(parent))
{
    m_ui->setupUi(this);
    m_ui->m_searchWidget->Setup(true, true);

    using namespace AzToolsFramework::AssetBrowser;
    AssetBrowserComponentRequestBus::BroadcastResult(m_assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
    AZ_Assert(m_assetBrowserModel, "Failed to get filebrowser model");
    m_filterModel->setSourceModel(m_assetBrowserModel);
    m_filterModel->SetFilter(m_ui->m_searchWidget->GetFilter());

    m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel.data());

    connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal,
        m_filterModel.data(), &AssetBrowserFilterModel::filterUpdatedSlot);
    connect(m_filterModel.data(), &AssetBrowserFilterModel::filterChanged, this, [this]()
    {
        const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
        const bool selectFirstFilteredIndex = false;
        m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
    });
    connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal,
        this, &AzAssetBrowserWindow::SelectionChangedSlot);
    connect(m_ui->m_assetBrowserTreeViewWidget, &QAbstractItemView::doubleClicked, this, &AzAssetBrowserWindow::DoubleClickedItem);

    connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::ClearStringFilter, m_ui->m_searchWidget, &SearchWidget::ClearStringFilter);
    connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::ClearTypeFilter, m_ui->m_searchWidget, &SearchWidget::ClearTypeFilter);
    m_ui->m_assetBrowserTreeViewWidget->SetName("AssetBrowserTreeView_main");
}

AzAssetBrowserWindow::~AzAssetBrowserWindow()
{
    m_ui->m_assetBrowserTreeViewWidget->SaveState();
}

void AzAssetBrowserWindow::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.preferedDockingArea = Qt::LeftDockWidgetArea;
    AzToolsFramework::RegisterViewPane<AzAssetBrowserWindow>(LyViewPane::AssetBrowser, LyViewPane::CategoryTools, options);
}

QObject* AzAssetBrowserWindow::createListenerForShowAssetEditorEvent(QObject* parent)
{
    auto* listener = new ListenerForShowAssetEditorEvent(parent);

    // the listener is attached to the parent and will get cleaned up then
    return listener;
}

void AzAssetBrowserWindow::UpdatePreview() const
{
    auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
    if (selectedAssets.size() != 1)
    {
        m_ui->m_previewerFrame->Clear();
        return;
    }

    m_ui->m_previewerFrame->Display(selectedAssets.front());
}

static void ExpandTreeToIndex(QTreeView* treeView, const QModelIndex& index)
{
    treeView->collapseAll();

    // Note that we deliberately don't expand the index passed in

    // collapseAll above will close all but the top level nodes.
    // treeView->expand(index) marks a node as expanded, but if it's parent isn't expanded,
    // there won't be any paint updates because it doesn't expand parent nodes.
    // So, to minimize paint updates, we expand everything in reverse order (leaf up to root), so that
    // painting will only actually occur once the top level parent is expanded.
    QModelIndex parentIndex = index.parent();
    while (parentIndex.isValid())
    {
        treeView->expand(parentIndex);
        parentIndex = parentIndex.parent();
    }
}

void AzAssetBrowserWindow::SelectAsset(const QString& assetPath)
{
    using namespace AzToolsFramework::AssetBrowser;

    QModelIndex index = m_assetBrowserModel->findIndex(assetPath);
    if (index.isValid())
    {
        m_ui->m_searchWidget->ClearTextFilter();
        m_ui->m_searchWidget->ClearTypeFilter();

        // Queue the expand and select stuff, so that it doesn't get processed the same
        // update as the search widget clearing - something with the search widget clearing
        // interferes with the update from the select and expand, and if you don't
        // queue it, the tree doesn't expand reliably.

        QTimer::singleShot(0, this, [this, filteredIndex = index] {
            // the treeview has a filter model so we have to backwards go from that
            QModelIndex index = m_filterModel->mapFromSource(filteredIndex);

            QTreeView* treeView = m_ui->m_assetBrowserTreeViewWidget;
            ExpandTreeToIndex(treeView, index);

            treeView->scrollTo(index);
            treeView->setCurrentIndex(index);

            treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        });
    }
}

void AzAssetBrowserWindow::SelectionChangedSlot(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/) const
{
    UpdatePreview();
}

// while its tempting to use Activated here, we dont actually want it to count as activation
// just becuase on some OS clicking once is activation.
void AzAssetBrowserWindow::DoubleClickedItem([[maybe_unused]] const QModelIndex& element)
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;
    // assumption: Double clicking an item selects it before telling us we double clicked it.
    auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
    for (const AssetBrowserEntry* entry : selectedAssets)
    {
        AZ::Data::AssetId assetIdToOpen;
        AZStd::string fullFilePath;

        if (const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(entry))
        {
            assetIdToOpen = productEntry->GetAssetId();
            fullFilePath = entry->GetFullPath();
        }
        else if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry))
        {
            // manufacture an empty AssetID with the source's UUID
            assetIdToOpen = AZ::Data::AssetId(sourceEntry->GetSourceUuid(), 0);
            fullFilePath = entry->GetFullPath();
        }
        
        bool handledBySomeone = false;
        if (assetIdToOpen.IsValid())
        {
            AssetBrowserInteractionNotificationBus::Broadcast(&AssetBrowserInteractionNotifications::OpenAssetInAssociatedEditor, assetIdToOpen, handledBySomeone);
        }

        if (!handledBySomeone && !fullFilePath.empty())
        {
            AzAssetBrowserRequestHandler::OpenWithOS(fullFilePath);
        }
    }

}

#include <AzAssetBrowser/moc_AzAssetBrowserWindow.cpp>
