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
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>

// AzQtComponents
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <AzQtComponents/Components/Widgets/AssetFolderTableView.h>

// Editor
#include "AzAssetBrowser/AzAssetBrowserRequestHandler.h"
#include "LyViewPaneNames.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AzAssetBrowser/ui_AzAssetBrowserWindow.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

AZ_CVAR_EXTERNED(bool, ed_useNewAssetBrowserTableView);

AZ_CVAR(bool, ed_useWIPAssetBrowserDesign, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Use the in-progress new Asset Browser design");

//! When the Asset Browser window is resized to be less than this many pixels in width
//! the layout changes to accomodate its narrow state better. See AzAssetBrowserWindow::SetNarrowMode
static constexpr int s_narrowModeThreshold = 700;
static constexpr int MinimumWidth = 328;

using namespace AzToolsFramework::AssetBrowser;

namespace
{
    inline QString FromStdString(AZStd::string_view string)
    {
        return QString::fromUtf8(string.data(), static_cast<int>(string.size()));
    }
}

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

    this->setMinimumWidth(MinimumWidth);

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

        m_ui->m_assetBrowserTableViewWidget->SetIsAssetBrowserMainView();

        connect(
            m_ui->m_thumbnailView,
            &AssetBrowserThumbnailView::entryDoubleClicked,
            this,
            [this](const AssetBrowserEntry* entry)
            {
                OnDoubleClick(entry);
            });

        connect(
            m_ui->m_thumbnailView,
            &AssetBrowserThumbnailView::showInFolderTriggered,
            this,
            [this](const AssetBrowserEntry* entry)
            {
                if (entry && entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
                {
                    entry = entry->GetParent();
                }

                if (!entry || !entry->GetParent())
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

                auto targetIndex = m_filterModel.data()->mapFromSource(indexForEntry);
                m_ui->m_assetBrowserTreeViewWidget->SetShowIndexAfterUpdate(targetIndex);
            });
    }

    connect(m_ui->m_expandedTableView, &AssetBrowserExpandedTableView::entryDoubleClicked,
        this,
        [this](const AssetBrowserEntry* entry)
        {
            OnDoubleClick(entry);
        });

    if (!ed_useWIPAssetBrowserDesign)
    {
        m_ui->m_breadcrumbsWrapper->hide(); 
        m_ui->m_middleStackWidget->hide();
        m_ui->m_treeViewButton->hide();
        m_ui->m_thumbnailViewButton->hide();
        m_ui->m_expandedTableViewButton->hide();
        m_ui->m_createButton->hide();
        m_ui->m_searchWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    m_ui->horizontalLayout->setAlignment(m_ui->m_toolsMenuButton, Qt::AlignTop);
    m_ui->horizontalLayout->setAlignment(m_ui->m_treeViewButton, Qt::AlignTop);
    m_ui->horizontalLayout->setAlignment(m_ui->m_expandedTableViewButton, Qt::AlignTop);
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
            m_ui->m_pathBreadCrumbs->pushFullPath(FromStdString(folderEntry->GetFullPath()), FromStdString(folderEntry->GetVisiblePath()));
        }
    });

    connect(m_ui->m_thumbnailViewButton, &QAbstractButton::clicked, this, [this] { SetTwoColumnMode(m_ui->m_thumbnailView); });
    connect(m_ui->m_expandedTableViewButton, &QAbstractButton::clicked, this, [this] { SetTwoColumnMode(m_ui->m_expandedTableView); });
    connect(m_ui->m_treeViewButton, &QAbstractButton::clicked, this, &AzAssetBrowserWindow::SetOneColumnMode);

    m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel.data());
    m_ui->m_thumbnailView->SetAssetTreeView(m_ui->m_assetBrowserTreeViewWidget);
    m_ui->m_expandedTableView->SetAssetTreeView(m_ui->m_assetBrowserTreeViewWidget);

    connect(m_ui->m_searchWidget->GetFilter().data(), &AzAssetBrowser::AssetBrowserEntryFilter::updatedSignal,
        m_filterModel.data(), &AzAssetBrowser::AssetBrowserFilterModel::filterUpdatedSlot);

    connect(m_filterModel.data(), &AzAssetBrowser::AssetBrowserFilterModel::filterChanged, this,
        [this]()
        {
            const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
            const bool selectFirstFilteredIndex = false;
            m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
            m_ui->m_expandedTableViewButton->setDisabled(hasFilter);
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
                m_ui->m_createButton->setEnabled(true);
            }
            else
            {
                m_ui->m_createButton->setDisabled(true);
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

    connect(
        m_assetBrowserModel,
        &AzAssetBrowser::AssetBrowserModel::RequestOpenItemForEditing,
        this,
        [this](const QModelIndex& index)
        {
            if (m_ui->m_thumbnailView->GetThumbnailActiveView())
            {
                m_ui->m_thumbnailView->OpenItemForEditing(index);
            }
            else if (m_ui->m_expandedTableView->GetExpandedTableViewActive())
            {
                m_ui->m_expandedTableView->OpenItemForEditing(index);
            }
            m_ui->m_assetBrowserTreeViewWidget->OpenItemForEditing(index);
        });

    connect(this, &AzAssetBrowserWindow::SizeChangedSignal,
        m_ui->m_assetBrowserTableViewWidget, &AzAssetBrowser::AssetBrowserTableView::UpdateSizeSlot);

    m_ui->m_assetBrowserTreeViewWidget->SetIsAssetBrowserMainView();
    m_ui->m_thumbnailView->SetIsAssetBrowserMainView();
    m_ui->m_expandedTableView->SetIsAssetBrowserMainView();
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
    const AssetBrowserEntry* entry = selectedAssets.empty() ? nullptr : selectedAssets.front();
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
    options.preferedDockingArea = Qt::BottomDockWidgetArea;
    AzToolsFramework::RegisterViewPane<AzAssetBrowserWindow>(LyViewPane::AssetBrowser, LyViewPane::CategoryTools, options);

    options.preferedDockingArea = Qt::BottomDockWidgetArea;
    AzToolsFramework::RegisterViewPane<AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget>(LyViewPane::AssetBrowserInspector, LyViewPane::CategoryTools, options);

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

bool AzAssetBrowserWindow::ViewWidgetBelongsTo(QWidget* viewWidget)
{
    return m_ui->m_assetBrowserTreeViewWidget == viewWidget || m_ui->m_assetBrowserTableViewWidget == viewWidget || m_ui->m_thumbnailView == viewWidget;
}

void AzAssetBrowserWindow::resizeEvent(QResizeEvent* resizeEvent)
{
    // leftLayout is the parent of the tableView
    // rightLayout is the parent of the preview window.
    // Workaround: When docking windows this event keeps holding the old size of the widgets instead of the new one
    // but the resizeEvent holds the new size of the whole widget
    // So we have to save the proportions somehow
    const QWidget* leftLayout = m_ui->m_leftLayout;

    const float oldLeftLayoutWidth = aznumeric_cast<float>(leftLayout->geometry().width());
    const float oldWidth = aznumeric_cast<float>(leftLayout->geometry().width());

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
        auto* projectSourceAssets = new QAction(tr("Hide Engine Folders"), this);
        projectSourceAssets->setCheckable(true);
        projectSourceAssets->setChecked(true);
        connect(projectSourceAssets, &QAction::triggered, this,
            [this, projectSourceAssets]
            {
                m_ui->m_searchWidget->ToggleProjectSourceAssetFilter(projectSourceAssets->isChecked());
            });
        m_toolsMenu->addAction(projectSourceAssets);
        m_ui->m_searchWidget->ToggleProjectSourceAssetFilter(projectSourceAssets->isChecked());

        auto* unusableProductAssets = new QAction(tr("Hide Unusable Product Assets"), this);
        unusableProductAssets->setCheckable(true);
        unusableProductAssets->setChecked(true);
        connect(
            unusableProductAssets,
            &QAction::triggered,
            this,
            [this, unusableProductAssets]
            {
                m_ui->m_searchWidget->ToggleUnusableProductsFilter(unusableProductAssets->isChecked());
            });
        m_toolsMenu->addAction(unusableProductAssets);
        m_ui->m_searchWidget->ToggleUnusableProductsFilter(unusableProductAssets->isChecked());

        m_ui->m_searchWidget->AddFolderFilter();

        m_assetBrowserDisplayState = AzToolsFramework::AssetBrowser::AssetBrowserDisplayState::TreeViewMode;
        m_ui->m_assetBrowserTableViewWidget->setVisible(false);
        m_ui->m_assetBrowserTreeViewWidget->setVisible(true);
        m_ui->m_thumbnailView->SetThumbnailActiveView(true);
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
            auto expandedTableWidget = m_ui->m_expandedTableView->GetExpandedTableViewWidget();
            if (expandedTableWidget)
            {
                expandedTableWidget->setRootIndex(expandedTableWidget->model()->index(0, 0, {}));
            }
        }
    }
}

void AzAssetBrowserWindow::UpdateBreadcrumbs(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) const
{
    using namespace AzToolsFramework::AssetBrowser;

    QString entryPath;
    QString fullPath;
    if (selectedEntry)
    {
        const AssetBrowserEntry* folderEntry = Utils::FolderForEntry(selectedEntry);
        if (folderEntry)
        {
            entryPath = FromStdString(folderEntry->GetVisiblePath());
            fullPath = FromStdString(folderEntry->GetFullPath());
        }
    }
    m_ui->m_pathBreadCrumbs->pushFullPath(fullPath, entryPath);
}

void AzAssetBrowserWindow::SetTwoColumnMode(QWidget* viewToShow)
{
    auto* thumbnailView = qobject_cast<AssetBrowserThumbnailView*>(viewToShow);
    if (thumbnailView && m_ui->m_thumbnailView->GetThumbnailActiveView())
    {
        return;
    }

    auto* expandedTableView = qobject_cast<AssetBrowserExpandedTableView*>(viewToShow);
    if (expandedTableView && m_ui->m_expandedTableView->GetExpandedTableViewActive())
    {
        return;
    }

    m_ui->m_middleStackWidget->show();
    m_ui->m_middleStackWidget->setCurrentWidget(viewToShow);
    m_ui->m_assetBrowserTreeViewWidget->SetApplySnapshot(false);
    m_ui->m_searchWidget->AddFolderFilter();
    if (thumbnailView)
    {
        m_ui->m_thumbnailView->SetThumbnailActiveView(true);
        m_ui->m_expandedTableView->SetExpandedTableViewActive(false);
        m_ui->m_searchWidget->setEnabled(true);
    }
    else if (expandedTableView)
    {
        m_ui->m_thumbnailView->SetThumbnailActiveView(false);
        m_ui->m_expandedTableView->SetExpandedTableViewActive(true);
        m_ui->m_searchWidget->setDisabled(true);
    }
}

void AzAssetBrowserWindow::SetOneColumnMode()
{
    if (m_ui->m_thumbnailView->GetThumbnailActiveView() || m_ui->m_expandedTableView->GetExpandedTableViewActive())
    {
        m_ui->m_middleStackWidget->hide();
        m_ui->m_assetBrowserTreeViewWidget->SetApplySnapshot(false);
        m_ui->m_searchWidget->RemoveFolderFilter();
        if (!m_ui->m_assetBrowserTreeViewWidget->selectionModel()->selectedRows().isEmpty())
        {
            m_ui->m_assetBrowserTreeViewWidget->expand(m_ui->m_assetBrowserTreeViewWidget->selectionModel()->selectedRows()[0]);
        }
        m_ui->m_thumbnailView->SetThumbnailActiveView(false);
        m_ui->m_expandedTableView->SetExpandedTableViewActive(false);
        m_ui->m_searchWidget->setEnabled(true);
    }
}

void AzAssetBrowserWindow::OnDoubleClick(const AssetBrowserEntry* entry)
{
    if (!m_ui->m_assetBrowserTreeViewWidget || !entry || !m_assetBrowserModel || !m_filterModel.data())
    {
        return;
    }

    QModelIndex indexForEntry;
    m_assetBrowserModel->GetEntryIndex(const_cast<AssetBrowserEntry*>(entry), indexForEntry);

    if (!indexForEntry.isValid())
    {
        return;
    }

    auto entryType = entry->GetEntryType();

    if (entryType == AssetBrowserEntry::AssetEntryType::Folder)
    {
        auto selectionModel = m_ui->m_assetBrowserTreeViewWidget->selectionModel();
        auto targetIndex = m_filterModel.data()->mapFromSource(indexForEntry);

        selectionModel->select(targetIndex, QItemSelectionModel::ClearAndSelect);

        auto targetIndexAncestor = targetIndex.parent();
        while (targetIndexAncestor.isValid())
        {
            m_ui->m_assetBrowserTreeViewWidget->expand(targetIndexAncestor);
            targetIndexAncestor = targetIndexAncestor.parent();
        }

        m_ui->m_assetBrowserTreeViewWidget->scrollTo(targetIndex, QAbstractItemView::ScrollHint::PositionAtCenter);
    }
    else if (entryType == AssetBrowserEntry::AssetEntryType::Product || entryType == AssetBrowserEntry::AssetEntryType::Source)
    {
        AZ::Data::AssetId assetIdToOpen;
        AZStd::string fullFilePath;

        if (const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productEntry =
                azrtti_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry))
        {
            assetIdToOpen = productEntry->GetAssetId();
            fullFilePath = entry->GetFullPath();
        }
        else if (const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* sourceEntry =
            azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry))
        {
            // manufacture an empty AssetID with the source's UUID
            assetIdToOpen = AZ::Data::AssetId(sourceEntry->GetSourceUuid(), 0);
            fullFilePath = entry->GetFullPath();
        }

        bool handledBySomeone = false;
        if (assetIdToOpen.IsValid())
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
                &AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications::OpenAssetInAssociatedEditor,
                assetIdToOpen,
                handledBySomeone);
        }

        if (!handledBySomeone && !fullFilePath.empty())
        {
            AzAssetBrowserRequestHandler::OpenWithOS(fullFilePath);
        }
    }

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

    UpdateBreadcrumbs(entry);
    AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::PreviewAsset, entry);
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
