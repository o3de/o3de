/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AzAssetBrowserWindow.h"

#include <AssetBrowser/Views/AssetBrowserTreeView.h>

// AzToolsFramework
#include <AzCore/Console/IConsole.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>

// AzQtComponents
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>

// Editor
#include "AzAssetBrowser/AzAssetBrowserRequestHandler.h"
#include "LyViewPaneNames.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AzAssetBrowser/ui_AzAssetBrowserWindow.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

AZ_CVAR_EXTERNED(bool, ed_useNewAssetBrowserTableView);

AZ_CVAR(bool, ed_useWIPAssetBrowserDesign, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Use the in-progress new Asset Browser design");

//! When the Asset Browser window is resized to be less than this many pixels in width
//! the layout changes to accomodate its narrow state better. See AzAssetBrowserWindow::SetNarrowMode
static constexpr int s_narrowModeThreshold = 700;

using namespace AzToolsFramework::AssetBrowser;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        static constexpr const char* MenuIcon = ":/Menu/menu.svg";
    } // namespace AssetBrowser
} // namespace AzToolsFramework

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

    CreateToolsMenu();

    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    AzAssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(
        m_assetBrowserModel, &AzAssetBrowser::AssetBrowserComponentRequests::GetAssetBrowserModel);
    AZ_Assert(m_assetBrowserModel, "Failed to get filebrowser model");
    m_filterModel->setSourceModel(m_assetBrowserModel);
    m_filterModel->SetFilter(m_ui->m_searchWidget->GetFilter());

    m_ui->m_assetBrowserTableViewWidget->setVisible(false);
    m_ui->m_toolsMenuButton->setVisible(false);
    m_ui->m_searchWidget->SetFilterInputInterval(AZStd::chrono::milliseconds(250));

    m_assetBrowserModel->SetFilterModel(m_filterModel.data());
    m_assetBrowserModel->EnableTickBus();

    if (ed_useNewAssetBrowserTableView)
    {
        m_ui->m_toolsMenuButton->setVisible(true);
        m_ui->m_toolsMenuButton->setEnabled(true);
        m_ui->m_toolsMenuButton->setAutoRaise(true);
        m_ui->m_toolsMenuButton->setIcon(QIcon(AzAssetBrowser::MenuIcon));

        m_tableModel->setFilterRole(Qt::DisplayRole);
        m_tableModel->setSourceModel(m_filterModel.data());
        m_tableModel->setDynamicSortFilter(true);
        m_ui->m_assetBrowserTableViewWidget->setModel(m_tableModel.data());

        m_createMenu = new QMenu("Create New Asset Menu", this);
        m_ui->m_createButton->setMenu(m_createMenu);
        m_ui->m_createButton->setEnabled(true);
        m_ui->m_createButton->setAutoRaise(true);
        m_ui->m_createButton->setPopupMode(QToolButton::InstantPopup);

        connect(m_createMenu, &QMenu::aboutToShow, this, &AzAssetBrowserWindow::AddCreateMenu);

        connect(m_filterModel.data(), &AzAssetBrowser::AssetBrowserFilterModel::filterChanged,
            this, &AzAssetBrowserWindow::UpdateWidgetAfterFilter);

        connect(m_ui->m_assetBrowserTableViewWidget->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AzAssetBrowserWindow::CurrentIndexChangedSlot);

        connect(
            m_ui->m_assetBrowserTableViewWidget->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [this](const QItemSelection& selected, const QItemSelection& deselected)
            {
                Q_UNUSED(deselected);
                if (selected.indexes().size() > 0)
                {
                    CurrentIndexChangedSlot(selected.indexes()[0]);
                }
            });

        connect(m_ui->m_assetBrowserTableViewWidget, &QAbstractItemView::doubleClicked,
            this, &AzAssetBrowserWindow::DoubleClickedItem);

        connect(m_ui->m_assetBrowserTableViewWidget, &AzAssetBrowser::AssetBrowserTableView::ClearStringFilter,
            m_ui->m_searchWidget, &AzAssetBrowser::SearchWidget::ClearStringFilter);

        connect(m_ui->m_assetBrowserTableViewWidget, &AzAssetBrowser::AssetBrowserTableView::ClearTypeFilter,
            m_ui->m_searchWidget, &AzAssetBrowser::SearchWidget::ClearTypeFilter);

        m_ui->m_assetBrowserTableViewWidget->SetName("AssetBrowserTableView_main");

        connect(
            m_ui->m_thumbnailView,
            &AssetBrowserThumbnailView::entryClicked,
            this,
            [this](const AssetBrowserEntry* entry)
            {
                if (entry && entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                {
                    m_ui->m_previewerFrame->Display(entry);
                }
            });
        connect(
            m_ui->m_thumbnailView,
            &AssetBrowserThumbnailView::entryDoubleClicked,
            this,
            [this](const AssetBrowserEntry* entry)
            {
                if (!m_ui->m_assetBrowserTreeViewWidget)
                {
                    return;
                }

                if (!entry)
                {
                    return;
                }

                if (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Folder)
                {
                    return;
                }

                if (!m_assetBrowserModel)
                {
                    return;
                }

                if (!m_filterModel.data())
                {
                    return;
                }

                QModelIndex indexForEntry;
                m_assetBrowserModel->GetEntryIndex(const_cast<AssetBrowserEntry*>(entry), indexForEntry);

                if (!indexForEntry.isValid())
                {
                    return;
                }

                auto selectionModel = m_ui->m_assetBrowserTreeViewWidget->selectionModel();
                auto targetIndex = m_filterModel.data()->mapFromSource(indexForEntry);

                selectionModel->select(targetIndex, QItemSelectionModel::ClearAndSelect);

                auto targetIndexAncestor = targetIndex.parent();
                while (targetIndexAncestor.isValid())
                {
                    m_ui->m_assetBrowserTreeViewWidget->expand(targetIndexAncestor);
                    targetIndexAncestor = targetIndexAncestor.parent();
                }

                m_ui->m_assetBrowserTreeViewWidget->scrollTo(targetIndex);
            });

        connect(
            m_ui->m_thumbnailView,
            &AssetBrowserThumbnailView::showInFolderTriggered,
            this,
            [this](const AssetBrowserEntry* entry)
            {
                if (!entry)
                {
                    return;
                }

                if (!entry->GetParent())
                {
                    return;
                }

                m_ui->m_searchWidget->ClearStringFilter();

                QModelIndex indexForEntry;
                m_assetBrowserModel->GetEntryIndex(const_cast<AssetBrowserEntry*>(entry->GetParent()), indexForEntry);

                if (!indexForEntry.isValid())
                {
                    return;
                }

                auto selectionModel = m_ui->m_assetBrowserTreeViewWidget->selectionModel();
                auto targetIndex = m_filterModel.data()->mapFromSource(indexForEntry);

                selectionModel->select(targetIndex, QItemSelectionModel::ClearAndSelect);
            });
    }

    if (!ed_useWIPAssetBrowserDesign)
    {
        m_ui->m_breadcrumbsWrapper->hide(); 
        m_ui->m_middleStackWidget->hide();
        m_ui->m_treeViewButton->hide();
        m_ui->m_thumbnailViewButton->hide();
        m_ui->m_tableViewButton->hide();
        m_ui->m_createButton->hide();
        m_ui->m_searchWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    m_ui->horizontalLayout->setAlignment(m_ui->m_toolsMenuButton, Qt::AlignTop);
    m_ui->horizontalLayout->setAlignment(m_ui->m_treeViewButton, Qt::AlignTop);
    m_ui->horizontalLayout->setAlignment(m_ui->m_tableViewButton, Qt::AlignTop);
    m_ui->horizontalLayout->setAlignment(m_ui->m_thumbnailViewButton, Qt::AlignTop);
    m_ui->horizontalLayout->setAlignment(m_ui->m_breadcrumbsWrapper, Qt::AlignTop);
    m_ui->horizontalLayout->setAlignment(m_ui->m_createButton, Qt::AlignTop);

    m_ui->m_breadcrumbsLayout->insertWidget(0, m_ui->m_pathBreadCrumbs->createSeparator());
    m_ui->m_breadcrumbsLayout->insertWidget(0, m_ui->m_pathBreadCrumbs->createBackForwardToolBar());

    connect(m_ui->m_pathBreadCrumbs, &AzQtComponents::BreadCrumbs::linkClicked, this, [this](const QString& path) {
        m_ui->m_assetBrowserTreeViewWidget->SelectFolder(path.toUtf8().constData());
    });
    connect(m_ui->m_pathBreadCrumbs, &AzQtComponents::BreadCrumbs::pathChanged, this, &AzAssetBrowserWindow::BreadcrumbsPathChangedSlot);
    connect(m_ui->m_pathBreadCrumbs, &AzQtComponents::BreadCrumbs::pathEdited, this, [this](const QString& path) {
        const auto* entry = m_ui->m_assetBrowserTreeViewWidget->GetEntryByPath(path);
        const auto* folderEntry = AzToolsFramework::AssetBrowser::Utils::FolderForEntry(entry);
        if (folderEntry)
        {
            // No need to select the folder ourselves, callback from Breadcrumbs will take care of that
            m_ui->m_pathBreadCrumbs->pushPath(QString::fromUtf8(folderEntry->GetVisiblePath().c_str()));
        }
    });

    connect(m_ui->m_thumbnailViewButton, &QAbstractButton::clicked, this, [this] { SetTwoColumnMode(m_ui->m_thumbnailView); });
    connect(m_ui->m_tableViewButton, &QAbstractButton::clicked, this, [this] { SetTwoColumnMode(m_ui->m_tableView); });
    connect(m_ui->m_treeViewButton, &QAbstractButton::clicked, this, &AzAssetBrowserWindow::SetOneColumnMode);

    m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel.data());
    // !!! Need to set the model on the tree widget first
    m_ui->m_thumbnailView->SetAssetTreeView(m_ui->m_assetBrowserTreeViewWidget);

    connect(m_ui->m_searchWidget->GetFilter().data(), &AzAssetBrowser::AssetBrowserEntryFilter::updatedSignal,
        m_filterModel.data(), &AzAssetBrowser::AssetBrowserFilterModel::filterUpdatedSlot);

    connect(m_filterModel.data(), &AzAssetBrowser::AssetBrowserFilterModel::filterChanged, this,
        [this]()
        {
            const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
            const bool selectFirstFilteredIndex = false;
            m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
        });

    connect(m_ui->m_assetBrowserTreeViewWidget->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &AzAssetBrowserWindow::CurrentIndexChangedSlot);

    connect(
        m_ui->m_assetBrowserTreeViewWidget->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this](const QItemSelection& selected, const QItemSelection& deselected)
        {
            Q_UNUSED(deselected);
            if (selected.indexes().size() > 0)
            {
                CurrentIndexChangedSlot(selected.indexes()[0]);
            }
        });

    connect(
        m_ui->m_assetBrowserTreeViewWidget,
        &QAbstractItemView::clicked,
        this,
        [this](const QModelIndex&)
        {
            m_ui->m_searchWidget->ClearStringFilter();
        });

    connect(m_ui->m_assetBrowserTreeViewWidget, &QAbstractItemView::doubleClicked, this, &AzAssetBrowserWindow::DoubleClickedItem);

    connect(m_ui->m_assetBrowserTreeViewWidget, &AzAssetBrowser::AssetBrowserTreeView::ClearStringFilter,
        m_ui->m_searchWidget, &AzAssetBrowser::SearchWidget::ClearStringFilter);

    connect(m_ui->m_assetBrowserTreeViewWidget, &AzAssetBrowser::AssetBrowserTreeView::ClearTypeFilter,
        m_ui->m_searchWidget, &AzAssetBrowser::SearchWidget::ClearTypeFilter);

    connect(m_assetBrowserModel, &AzAssetBrowser::AssetBrowserModel::RequestOpenItemForEditing,
        m_ui->m_assetBrowserTreeViewWidget, &AzAssetBrowser::AssetBrowserTreeView::OpenItemForEditing);

    connect(this, &AzAssetBrowserWindow::SizeChangedSignal,
        m_ui->m_assetBrowserTableViewWidget, &AzAssetBrowser::AssetBrowserTableView::UpdateSizeSlot);

    m_ui->m_assetBrowserTreeViewWidget->SetName("AssetBrowserTreeView_main");
}

AzAssetBrowserWindow::~AzAssetBrowserWindow()
{
    m_assetBrowserModel->DisableTickBus();
    m_ui->m_assetBrowserTreeViewWidget->SaveState();
}

void AzAssetBrowserWindow::AddCreateMenu()
{
    using namespace AzToolsFramework::AssetBrowser;
    m_createMenu->clear();

    const auto& selectedAssets = m_ui->m_assetBrowserTreeViewWidget->isVisible() ? m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets()
                                                                                 : m_ui->m_assetBrowserTableViewWidget->GetSelectedAssets();
    AssetBrowserEntry* entry = selectedAssets.empty() ? nullptr : selectedAssets.front();
    if (!entry || selectedAssets.size() != 1)
    {
        return;
    }

    if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
    {
        entry = entry->GetParent();
        if (!entry)
        {
            return;
        }
    }
    AZStd::string fullFilePath = entry->GetFullPath();

    AZStd::string folderPath;

    if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
    {
        folderPath = fullFilePath;
    }
    else
    {
        AzFramework::StringFunc::Path::GetFolderPath(fullFilePath.c_str(), folderPath);
    }

    AZ::Uuid sourceID = AZ::Uuid::CreateNull();
    SourceFileCreatorList creators;
    AssetBrowserInteractionNotificationBus::Broadcast(
        &AssetBrowserInteractionNotificationBus::Events::AddSourceFileCreators, folderPath.c_str(), sourceID, creators);
    if (!creators.empty())
    {
        for (const SourceFileCreatorDetails& creatorDetails : creators)
        {
            if (creatorDetails.m_creator)
            {
                m_createMenu->addAction(
                    creatorDetails.m_iconToUse,
                    tr("New ") + tr(creatorDetails.m_displayText.c_str()),
                    [sourceID, fullFilePath, creatorDetails]()
                    {
                        creatorDetails.m_creator(fullFilePath.c_str(), sourceID);
                    });
            }
        }
    }
}

void AzAssetBrowserWindow::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.preferedDockingArea = Qt::LeftDockWidgetArea;
    AzToolsFramework::RegisterViewPane<AzAssetBrowserWindow>(LyViewPane::AssetBrowser, LyViewPane::CategoryTools, options);

    options.showInMenu = false;
    const QString name = QString("%1 (2)").arg(LyViewPane::AssetBrowser);
    AzToolsFramework::RegisterViewPane<AzAssetBrowserWindow>(qPrintable(name), LyViewPane::CategoryTools, options);
}

QObject* AzAssetBrowserWindow::createListenerForShowAssetEditorEvent(QObject* parent)
{
    auto* listener = new ListenerForShowAssetEditorEvent(parent);

    // the listener is attached to the parent and will get cleaned up then
    return listener;
}

bool AzAssetBrowserWindow::TreeViewBelongsTo(AzToolsFramework::AssetBrowser::AssetBrowserTreeView* treeView) {
    return m_ui->m_assetBrowserTreeViewWidget == treeView;
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

    const bool isNarrow = resizeEvent->size().width() < s_narrowModeThreshold;
    SetNarrowMode(isNarrow);

    emit SizeChangedSignal(aznumeric_cast<int>(newWidth));
    QWidget::resizeEvent(resizeEvent);
}

void AzAssetBrowserWindow::CreateToolsMenu()
{
    if (m_toolsMenu != nullptr)
    {
        return;
    }

    m_toolsMenu = new QMenu("Asset Browser Mode Selection", this);
    m_ui->m_toolsMenuButton->setMenu(m_toolsMenu);
    m_ui->m_toolsMenuButton->setPopupMode(QToolButton::InstantPopup);

    if (ed_useWIPAssetBrowserDesign)
    {
        auto* collapseAllAction = new QAction(tr("Collapse All"), this);
        connect(collapseAllAction, &QAction::triggered, this, [this] { m_ui->m_assetBrowserTreeViewWidget->collapseAll(); });
        m_toolsMenu->addAction(collapseAllAction);

        m_toolsMenu->addSeparator();
        auto* projectSourceAssets = new QAction(tr("Filter Project and Source Assets"), this);
        projectSourceAssets->setCheckable(true);
        projectSourceAssets->setChecked(true);
        connect(projectSourceAssets, &QAction::triggered, this, [this] { m_ui->m_searchWidget->ToggleProjectSourceAssetFilter(); });
        m_toolsMenu->addAction(projectSourceAssets);

        m_ui->m_searchWidget->GetFilter()->AddFilter(m_ui->m_searchWidget->GetProjectSourceFilter());
        m_ui->m_searchWidget->AddFolderFilter();

        m_assetBrowserDisplayState = AzToolsFramework::AssetBrowser::AssetBrowserDisplayState::TreeViewMode;
        m_ui->m_assetBrowserTableViewWidget->setVisible(false);
        m_ui->m_assetBrowserTreeViewWidget->setVisible(true);
    }
    else
    {
        m_listViewMode = new QAction(tr("List View"), this);
        m_listViewMode->setCheckable(true);
        connect(m_listViewMode, &QAction::triggered, this, &AzAssetBrowserWindow::SetListViewMode);
        m_toolsMenu->addAction(m_listViewMode);

        m_treeViewMode = new QAction(tr("Tree View"), this);
        m_treeViewMode->setCheckable(true);
        connect(m_treeViewMode, &QAction::triggered, this, &AzAssetBrowserWindow::SetTreeViewMode);
        m_toolsMenu->addAction(m_treeViewMode);

        connect(m_toolsMenu, &QMenu::aboutToShow, this, &AzAssetBrowserWindow::UpdateDisplayInfo);

        UpdateDisplayInfo();
    }
}

void AzAssetBrowserWindow::UpdateDisplayInfo()
{
    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    if (m_toolsMenu == nullptr)
    {
        return;
    }

    m_treeViewMode->setChecked(false);
    m_listViewMode->setChecked(false);

    switch (m_assetBrowserDisplayState)
    {
    case AzAssetBrowser::AssetBrowserDisplayState::TreeViewMode:
        {
            m_treeViewMode->setChecked(true);
            break;
        }
    case AzAssetBrowser::AssetBrowserDisplayState::ListViewMode:
        {
            m_listViewMode->setChecked(true);
            break;
        }
    }
}

void AzAssetBrowserWindow::SetNarrowMode(bool narrow)
{
    if (m_inNarrowMode == narrow)
    {
        return;
    }

    // In narrow mode, breadcrumbs are below the search bar and view switching buttons
    m_inNarrowMode = narrow;
    if (narrow)
    {
        m_ui->scrollAreaVerticalLayout->insertWidget(1, m_ui->m_breadcrumbsWrapper);
        m_ui->m_searchWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        m_ui->m_breadcrumbsWrapper->setContentsMargins(0, 0, 0, 5);
    }
    else
    {
        m_ui->horizontalLayout->insertWidget(4, m_ui->m_breadcrumbsWrapper);
        m_ui->m_breadcrumbsWrapper->setContentsMargins(0, 0, 0, 0);
        m_ui->horizontalLayout->setAlignment(m_ui->m_breadcrumbsWrapper, Qt::AlignTop);

        // Once we fully move to new design this cvar will be gone and the condition can be deleted
        if (ed_useWIPAssetBrowserDesign)
        {
            m_ui->m_searchWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        }
    }
}

void AzAssetBrowserWindow::SetTreeViewMode()
{
    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    m_assetBrowserDisplayState = AzAssetBrowser::AssetBrowserDisplayState::TreeViewMode;

    if (m_ui->m_assetBrowserTableViewWidget->isVisible())
    {
        m_ui->m_assetBrowserTableViewWidget->setVisible(false);
        m_ui->m_assetBrowserTreeViewWidget->setVisible(true);
    }
}

void AzAssetBrowserWindow::SetListViewMode()
{
    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    m_assetBrowserDisplayState = AzAssetBrowser::AssetBrowserDisplayState::ListViewMode;
    UpdateWidgetAfterFilter();
}


void AzAssetBrowserWindow::UpdateWidgetAfterFilter()
{
    namespace AzAssetBrowser = AzToolsFramework::AssetBrowser;

    const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
    if (m_assetBrowserDisplayState == AzAssetBrowser::AssetBrowserDisplayState::ListViewMode)
    {
        m_ui->m_assetBrowserTableViewWidget->setVisible(hasFilter);
        m_ui->m_assetBrowserTreeViewWidget->setVisible(!hasFilter);
    }

    if (hasFilter)
    {
        m_ui->m_assetBrowserTreeViewWidget->selectionModel()->select(
            m_ui->m_assetBrowserTreeViewWidget->model()->index(0, 0, {}), QItemSelectionModel::ClearAndSelect);
    }

    if (ed_useNewAssetBrowserTableView)
    {
        if (hasFilter)
        {
            auto thumbnailWidget = m_ui->m_thumbnailView->GetThumbnailViewWidget();
            if (thumbnailWidget)
            {
                thumbnailWidget->setRootIndex(thumbnailWidget->model()->index(0, 0, {}));
            }
        }
    }
}

void AzAssetBrowserWindow::UpdatePreview(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) const
{
    if (selectedEntry)
    {
        m_ui->m_previewerFrame->Display(selectedEntry);
    }
    else
    {
        m_ui->m_previewerFrame->Clear();
    }
}

void AzAssetBrowserWindow::UpdateBreadcrumbs(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) const
{
    using namespace AzToolsFramework::AssetBrowser;

    QString entryPath;
    if (selectedEntry)
    {
        const AssetBrowserEntry* folderEntry = Utils::FolderForEntry(selectedEntry);
        if (folderEntry)
        {
            entryPath = QString::fromUtf8(folderEntry->GetVisiblePath().c_str());
        }
    }
    m_ui->m_pathBreadCrumbs->pushPath(entryPath);
}

void AzAssetBrowserWindow::SetTwoColumnMode(QWidget* viewToShow)
{
    m_ui->m_middleStackWidget->show();
    m_ui->m_middleStackWidget->setCurrentWidget(viewToShow);
    m_ui->m_searchWidget->AddFolderFilter();
}

void AzAssetBrowserWindow::SetOneColumnMode()
{
    m_ui->m_middleStackWidget->hide();
    m_ui->m_searchWidget->RemoveFolderFilter();
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

void AzAssetBrowserWindow::CurrentIndexChangedSlot(const QModelIndex& idx) const
{
    using namespace AzToolsFramework::AssetBrowser;
    auto* entry = idx.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();

    UpdatePreview(entry);
    UpdateBreadcrumbs(entry);
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

//! This slot ignores path change coming from breadcrumbs if we already have a file selected directly in this folder. This may happen
//! in the one column mode where tree view shows files too.
void AzAssetBrowserWindow::BreadcrumbsPathChangedSlot(const QString& path) const
{
    using namespace AzToolsFramework::AssetBrowser;
    const auto* currentEntry =
        m_ui->m_assetBrowserTreeViewWidget->currentIndex().data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();

    const AssetBrowserEntry* folderForCurrent = Utils::FolderForEntry(currentEntry);
    const QString currentFolderPath =
        folderForCurrent ? QString::fromUtf8(folderForCurrent->GetVisiblePath().c_str()).replace('\\', '/') : QString();

    if (path != currentFolderPath)
    {
        m_ui->m_assetBrowserTreeViewWidget->SelectFolder(path.toUtf8().constData());
    }
}

#include <AzAssetBrowser/moc_AzAssetBrowserWindow.cpp>
