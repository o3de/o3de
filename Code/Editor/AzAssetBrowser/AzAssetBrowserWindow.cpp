/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AzAssetBrowserWindow.h"

// AzToolsFramework
#include <AzCore/Console/IConsole.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>

// AzQtComponents
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

// Editor
#include "AzAssetBrowser/AzAssetBrowserRequestHandler.h"
#include "LyViewPaneNames.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AzAssetBrowser/ui_AzAssetBrowserWindow.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

AZ_CVAR_EXTERNED(bool, ed_useNewAssetBrowserTableView);

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

    ~ListenerForShowAssetEditorEvent() override
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
    , m_tableModel(new AzToolsFramework::AssetBrowser::AssetBrowserTableModel(parent))
{
    m_ui->setupUi(this);
    m_ui->m_searchWidget->Setup(true, true);

    OnInitViewToggleButton();

    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    AzAssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(
        m_assetBrowserModel, &AzAssetBrowser::AssetBrowserComponentRequests::GetAssetBrowserModel);
    AZ_Assert(m_assetBrowserModel, "Failed to get filebrowser model");
    m_filterModel->setSourceModel(m_assetBrowserModel);
    m_filterModel->SetFilter(m_ui->m_searchWidget->GetFilter());

    m_ui->m_assetBrowserTableViewWidget->setVisible(false);
    m_ui->m_toggleDisplayViewBtn->setVisible(false);
    m_ui->m_searchWidget->SetFilterInputInterval(AZStd::chrono::milliseconds(250));
    if (ed_useNewAssetBrowserTableView)
    {
        m_ui->m_toggleDisplayViewBtn->setVisible(true);
        m_ui->m_toggleDisplayViewBtn->setIcon(QIcon(":/Menu/menu.svg"));

        m_tableModel->setFilterRole(Qt::DisplayRole);
        m_tableModel->setSourceModel(m_filterModel.data());
        m_tableModel->setDynamicSortFilter(true);
        m_ui->m_assetBrowserTableViewWidget->setModel(m_tableModel.data());

        connect(
            m_filterModel.data(), &AzAssetBrowser::AssetBrowserFilterModel::filterChanged, this,
            &AzAssetBrowserWindow::SetTableViewVisibleAfterFilter);
        connect(
            m_ui->m_assetBrowserTableViewWidget, &AzAssetBrowser::AssetBrowserTableView::selectionChangedSignal, this,
            &AzAssetBrowserWindow::SelectionChangedSlot);
        connect(m_ui->m_assetBrowserTableViewWidget, &QAbstractItemView::doubleClicked, this, &AzAssetBrowserWindow::DoubleClickedItem);
        connect(
            m_ui->m_assetBrowserTableViewWidget, &AzAssetBrowser::AssetBrowserTableView::ClearStringFilter, m_ui->m_searchWidget,
            &AzAssetBrowser::SearchWidget::ClearStringFilter);
        connect(
            m_ui->m_assetBrowserTableViewWidget, &AzAssetBrowser::AssetBrowserTableView::ClearTypeFilter, m_ui->m_searchWidget,
            &AzAssetBrowser::SearchWidget::ClearTypeFilter);

        m_ui->m_assetBrowserTableViewWidget->SetName("AssetBrowserTableView_main");
    }

    m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel.data());

    connect(
        m_ui->m_searchWidget->GetFilter().data(), &AzAssetBrowser::AssetBrowserEntryFilter::updatedSignal, m_filterModel.data(),
        &AzAssetBrowser::AssetBrowserFilterModel::filterUpdatedSlot);
    connect(
        m_filterModel.data(), &AzAssetBrowser::AssetBrowserFilterModel::filterChanged, this,
        [this]()
        {
            const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
            const bool selectFirstFilteredIndex = false;
            m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
        });

    connect(
        m_ui->m_assetBrowserTreeViewWidget, &AzAssetBrowser::AssetBrowserTreeView::selectionChangedSignal, this,
        &AzAssetBrowserWindow::SelectionChangedSlot);

    connect(m_ui->m_assetBrowserTreeViewWidget, &QAbstractItemView::doubleClicked, this, &AzAssetBrowserWindow::DoubleClickedItem);

    connect(
        m_ui->m_assetBrowserTreeViewWidget, &AzAssetBrowser::AssetBrowserTreeView::ClearStringFilter, m_ui->m_searchWidget,
        &AzAssetBrowser::SearchWidget::ClearStringFilter);
    connect(
        m_ui->m_assetBrowserTreeViewWidget, &AzAssetBrowser::AssetBrowserTreeView::ClearTypeFilter, m_ui->m_searchWidget,
        &AzAssetBrowser::SearchWidget::ClearTypeFilter);

    connect(
        this, &AzAssetBrowserWindow::SizeChangedSignal, m_ui->m_assetBrowserTableViewWidget,
        &AzAssetBrowser::AssetBrowserTableView::UpdateSizeSlot);

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

void AzAssetBrowserWindow::resizeEvent(QResizeEvent* resizeEvent)
{
    // leftLayout is the parent of the tableView
    // rightLayout is the parent of the preview window.
    // Workaround: When docking windows this event keeps holding the old size of the widgets instead of the new one
    // but the resizeEvent holds the new size of the whole widget
    // So we have to save the proportions somehow
    const QWidget* leftLayout = m_ui->m_leftLayout;
    const QVBoxLayout* rightLayout = m_ui->m_rightLayout;

    const float oldLeftLayoutWidth = aznumeric_cast<float>(leftLayout->geometry().width());
    const float oldWidth = aznumeric_cast<float>(leftLayout->geometry().width() + rightLayout->geometry().width());

    const float newWidth = oldLeftLayoutWidth * aznumeric_cast<float>(resizeEvent->size().width()) / oldWidth;
    
    emit SizeChangedSignal(aznumeric_cast<int>(newWidth));
    QWidget::resizeEvent(resizeEvent);
}

void AzAssetBrowserWindow::OnInitViewToggleButton()
{
    CreateSwitchViewMenu();
    m_ui->m_toggleDisplayViewBtn->setMenu(m_viewSwitchMenu);
    m_ui->m_toggleDisplayViewBtn->setPopupMode(QToolButton::InstantPopup);

    connect(m_viewSwitchMenu, &QMenu::aboutToShow, this, &AzAssetBrowserWindow::UpdateDisplayInfo);
}

void AzAssetBrowserWindow::CreateSwitchViewMenu()
{
    if (m_viewSwitchMenu != nullptr)
    {
        return;
    }

    m_viewSwitchMenu = new QMenu("Asset Browser Mode Selection", this);

    m_expandedAssetBrowserMode = new QAction(tr("Expanded"), this);
    m_expandedAssetBrowserMode->setCheckable(true);
    connect(m_expandedAssetBrowserMode, &QAction::triggered, this, &AzAssetBrowserWindow::SetExpandedAssetBrowserMode);
    m_viewSwitchMenu->addAction(m_expandedAssetBrowserMode);

    m_defaultAssetBrowserMode = new QAction(tr("Default"), this);
    m_defaultAssetBrowserMode->setCheckable(true);
    connect(m_defaultAssetBrowserMode, &QAction::triggered, this, &AzAssetBrowserWindow::SetDefaultAssetBrowserMode);
    m_viewSwitchMenu->addAction(m_defaultAssetBrowserMode);

    UpdateDisplayInfo();
}

void AzAssetBrowserWindow::UpdateDisplayInfo()
{
    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    if (m_viewSwitchMenu == nullptr)
    {
        return;
    }

    m_expandedAssetBrowserMode->setChecked(false);
    m_defaultAssetBrowserMode->setChecked(false);

    switch (m_assetBrowserDisplayState)
    {
    case AzAssetBrowser::AssetBrowserDisplayState::ExpandedMode:
        {
            m_expandedAssetBrowserMode->setChecked(true);
            break;
        }
    case AzAssetBrowser::AssetBrowserDisplayState::DefaultMode:
        {
            m_defaultAssetBrowserMode->setChecked(true);
            break;
        }
    }
}

void AzAssetBrowserWindow::SetExpandedAssetBrowserMode()
{
    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    m_assetBrowserDisplayState = AzAssetBrowser::AssetBrowserDisplayState::ExpandedMode;

    if (m_ui->m_assetBrowserTableViewWidget->isVisible())
    {
        m_ui->m_assetBrowserTableViewWidget->setVisible(false);
        m_ui->m_assetBrowserTreeViewWidget->setVisible(true);
    }
}

void AzAssetBrowserWindow::SetDefaultAssetBrowserMode()
{
    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    m_assetBrowserDisplayState = AzAssetBrowser::AssetBrowserDisplayState::DefaultMode;
    SetTableViewVisibleAfterFilter();
}


void AzAssetBrowserWindow::SetTableViewVisibleAfterFilter()
{
    const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
    m_ui->m_assetBrowserTableViewWidget->setVisible(hasFilter);
    m_ui->m_assetBrowserTreeViewWidget->setVisible(!hasFilter);
}

void AzAssetBrowserWindow::UpdatePreview() const
{
    const auto& selectedAssets = m_ui->m_assetBrowserTreeViewWidget->isVisible() ? m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets()
                                                                                 : m_ui->m_assetBrowserTableViewWidget->GetSelectedAssets();

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
    QModelIndex index = m_assetBrowserModel->findIndex(assetPath);
    if (index.isValid())
    {
        m_ui->m_searchWidget->ClearTextFilter();
        m_ui->m_searchWidget->ClearTypeFilter();

        // Queue the expand and select stuff, so that it doesn't get processed the same
        // update as the search widget clearing - something with the search widget clearing
        // interferes with the update from the select and expand, and if you don't
        // queue it, the tree doesn't expand reliably.

        QTimer::singleShot(
            0, this,
            [this, filteredIndex = index]
            {
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

// while its tempting to use Activated here, we don't actually want it to count as activation
// just because on some OS clicking once is activation.
void AzAssetBrowserWindow::DoubleClickedItem([[maybe_unused]] const QModelIndex& element)
{
    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    const auto& selectedAssets = m_ui->m_assetBrowserTreeViewWidget->isVisible() ? m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets()
                                                                                 : m_ui->m_assetBrowserTableViewWidget->GetSelectedAssets();

    for (const AzAssetBrowser::AssetBrowserEntry* entry : selectedAssets)
    {
        AZ::Data::AssetId assetIdToOpen;
        AZStd::string fullFilePath;

        if (const AzAssetBrowser::ProductAssetBrowserEntry* productEntry =
                azrtti_cast<const AzAssetBrowser::ProductAssetBrowserEntry*>(entry))
        {
            assetIdToOpen = productEntry->GetAssetId();
            fullFilePath = entry->GetFullPath();
        }
        else if (
            const AzAssetBrowser::SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const AzAssetBrowser::SourceAssetBrowserEntry*>(entry))
        {
            // manufacture an empty AssetID with the source's UUID
            assetIdToOpen = AZ::Data::AssetId(sourceEntry->GetSourceUuid(), 0);
            fullFilePath = entry->GetFullPath();
        }

        bool handledBySomeone = false;
        if (assetIdToOpen.IsValid())
        {
            AzAssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
                &AzAssetBrowser::AssetBrowserInteractionNotifications::OpenAssetInAssociatedEditor, assetIdToOpen, handledBySomeone);
        }

        if (!handledBySomeone && !fullFilePath.empty())
        {
            AzAssetBrowserRequestHandler::OpenWithOS(fullFilePath);
        }
    }
}

#include <AzAssetBrowser/moc_AzAssetBrowserWindow.cpp>
