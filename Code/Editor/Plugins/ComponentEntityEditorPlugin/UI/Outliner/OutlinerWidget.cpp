/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CryCommon/platform.h>
#include "MainWindow.h"                     // for MainWindow

#include "OutlinerListModel.hxx"
#include "OutlinerSortFilterProxyModel.hxx"
#include "OutlinerWidget.hxx"
#include "OutlinerDisplayOptionsMenu.h"
#include "Objects/SelectionGroup.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings//SettingsRegistryMergeUtils.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/sort.h>

#include <AzQtComponents/Utilities/QtViewPaneEffects.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QDir>
#include <QPainter>
#include <QApplication>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QTimer>
#include <QHeaderView>
#include <QInputDialog>

#include <UI/Outliner/ui_OutlinerWidget.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>

namespace
{
    const int queuedChangeDelay = 16; // milliseconds

    using EntityIdCompareFunc = AZStd::function<bool(AZ::EntityId, AZ::EntityId)>;

    bool CompareEntitiesForSorting(AZ::EntityId leftEntityId, AZ::EntityId rightEntityId, EntityOutliner::DisplaySortMode sortMode)
    {
        AZ::Entity* leftEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(leftEntity, &AZ::ComponentApplicationBus::Events::FindEntity, leftEntityId);
        AZ_Assert(leftEntity, "Could not find entity with id %d", leftEntityId);
        if (!leftEntity)
        {
            return false;
        }

        AZ::Entity* rightEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(rightEntity, &AZ::ComponentApplicationBus::Events::FindEntity, rightEntityId);
        AZ_Assert(rightEntity, "Could not find entity with id %d", rightEntityId);
        if (!rightEntity)
        {
            return true;
        }

        const int compareResult = azstricmp(leftEntity->GetName().c_str(), rightEntity->GetName().c_str());
        const bool isSame = (compareResult == 0);

        if (sortMode == EntityOutliner::DisplaySortMode::AtoZ)
        {
            return (isSame ? 
                leftEntityId < rightEntityId : 
                compareResult < 0);
        }
        else
        {
            return (isSame ? 
                leftEntityId > rightEntityId : 
                compareResult > 0);
        }
    }

    void SortEntityChildren(AZ::EntityId entityId, const EntityIdCompareFunc& comparer, AzToolsFramework::EntityOrderArray* newEntityOrder = nullptr)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(entityId);
        AZStd::sort(entityOrderArray.begin(), entityOrderArray.end(), comparer);
        AzToolsFramework::SetEntityChildOrder(entityId, entityOrderArray);

        if (newEntityOrder)
        {
            *newEntityOrder = AZStd::move(entityOrderArray);
        }
    }

    void SortEntityChildrenRecursively(AZ::EntityId entityId, const EntityIdCompareFunc& comparer)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AzToolsFramework::EntityOrderArray entityOrderArray;
        SortEntityChildren(entityId, comparer, &entityOrderArray);

        for (const AZ::EntityId& childId : entityOrderArray)
        {
            SortEntityChildrenRecursively(childId, comparer);
        }
    }
}

OutlinerWidget::OutlinerWidget(QWidget* pParent, Qt::WindowFlags flags)
    : QWidget(pParent, flags)
    , m_gui(nullptr)
    , m_listModel(nullptr)
    , m_proxyModel(nullptr)
    , m_selectionContextId(0)
    , m_selectedEntityIds()
    , m_inObjectPickMode(false)
    , m_scrollToNewContentQueued(false)
    , m_scrollToEntityId()
    , m_entitiesToSelect()
    , m_entitiesToDeselect()
    , m_selectionChangeQueued(false)
    , m_selectionChangeInProgress(false)
    , m_enableSelectionUpdates(true)
    , m_scrollToSelectedEntity(true)
    , m_expandSelectedEntity(true)
    , m_focusInEntityOutliner(false)
    , m_displayOptionsMenu(new EntityOutliner::DisplayOptionsMenu(this))
    , m_entitiesToSort()
    , m_sortMode(EntityOutliner::DisplaySortMode::Manually)
    , m_sortContentQueued(false)
    , m_dropOperationInProgress(false)
{
    m_gui = new Ui::OutlinerWidgetUI();
    m_gui->setupUi(this);


    AZ::IO::FixedMaxPath engineRootPath;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }
    QDir rootDir = QString::fromUtf8(engineRootPath.c_str(), aznumeric_cast<int>(engineRootPath.Native().size()));
    const auto pathOnDisk = rootDir.absoluteFilePath(QStringLiteral("Code/Editor/Plugins/ComponentEntityEditorPlugin/UI/Outliner/"));
    const auto qrcPath = QStringLiteral(":/EntityOutliner/");

    // Setting the style sheet using both methods allows faster style iteration speed for
    // developers. The style will be loaded from a Qt Resource file if Editor is installed, but
    // developers with the file on disk will be able to modify the style and have it automatically
    // reloaded.
    AzQtComponents::StyleManager::addSearchPaths("EntityOutliner", pathOnDisk, qrcPath, engineRootPath);
    AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("EntityOutliner:EntityOutliner.qss"));

    m_listModel = aznew OutlinerListModel(this);
    m_listModel->SetSortMode(m_sortMode);

    const int autoExpandDelayMilliseconds = 2500;
    m_gui->m_objectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gui->m_objectTree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_gui->m_objectTree->setAutoExpandDelay(autoExpandDelayMilliseconds);
    m_gui->m_objectTree->setDragEnabled(true);
    m_gui->m_objectTree->setDropIndicatorShown(true);
    m_gui->m_objectTree->setAcceptDrops(true);
    m_gui->m_objectTree->setDragDropOverwriteMode(false);
    m_gui->m_objectTree->setDragDropMode(QAbstractItemView::DragDrop);
    m_gui->m_objectTree->setDefaultDropAction(Qt::CopyAction);
    m_gui->m_objectTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gui->m_objectTree->setAutoScrollMargin(20);

    // custom item delegate
    m_gui->m_objectTree->setItemDelegate(aznew OutlinerItemDelegate(m_gui->m_objectTree));

    m_proxyModel = aznew OutlinerSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_listModel);
    m_gui->m_objectTree->setModel(m_proxyModel);

    // Link up signals for informing the model of tree changes using the proxy as an intermediary
    connect(m_gui->m_objectTree, &QTreeView::clicked, this, &OutlinerWidget::OnTreeItemClicked);
    connect(m_gui->m_objectTree, &QTreeView::expanded, this, &OutlinerWidget::OnTreeItemExpanded);
    connect(m_gui->m_objectTree, &QTreeView::collapsed, this, &OutlinerWidget::OnTreeItemCollapsed);
    connect(m_gui->m_objectTree, &OutlinerTreeView::ItemDropped, this, &OutlinerWidget::OnDropEvent);
    connect(m_listModel, &OutlinerListModel::ExpandEntity, this, &OutlinerWidget::OnExpandEntity);
    connect(m_listModel, &OutlinerListModel::SelectEntity, this, &OutlinerWidget::OnSelectEntity);
    connect(m_listModel, &OutlinerListModel::EnableSelectionUpdates, this, &OutlinerWidget::OnEnableSelectionUpdates);
    connect(m_listModel, &OutlinerListModel::ResetFilter, this, &OutlinerWidget::ClearFilter);
    connect(m_listModel, &OutlinerListModel::ReapplyFilter, this, &OutlinerWidget::InvalidateFilter);

    QToolButton* display_options = new QToolButton(this);
    display_options->setObjectName(QStringLiteral("m_display_options"));
    display_options->setPopupMode(QToolButton::InstantPopup);
    display_options->setAutoRaise(true);

    m_gui->m_searchWidget->AddWidgetToSearchWidget(display_options);

    // Set the display options menu
    display_options->setMenu(m_displayOptionsMenu);
    connect(m_displayOptionsMenu, &EntityOutliner::DisplayOptionsMenu::OnSortModeChanged, this, &OutlinerWidget::OnSortModeChanged);
    connect(m_displayOptionsMenu, &EntityOutliner::DisplayOptionsMenu::OnOptionToggled, this, &OutlinerWidget::OnDisplayOptionChanged);

    // Sorting is performed in a very specific way by the proxy model.
    // Disable sort indicator so user isn't confused by the fact
    // that they can't actually change how sorting works.
    m_gui->m_objectTree->hideColumn(OutlinerListModel::ColumnSortIndex);
    m_gui->m_objectTree->setSortingEnabled(true);
    m_gui->m_objectTree->header()->setSortIndicatorShown(false);
    m_gui->m_objectTree->header()->setStretchLastSection(false);

    // resize the icon columns so that the Visibility and Lock toggle icon columns stay right-justified
    m_gui->m_objectTree->header()->setSectionResizeMode(OutlinerListModel::ColumnName, QHeaderView::Stretch);
    m_gui->m_objectTree->header()->setSectionResizeMode(OutlinerListModel::ColumnVisibilityToggle, QHeaderView::ResizeToContents);
    m_gui->m_objectTree->header()->resizeSection(OutlinerListModel::ColumnLockToggle, 24);

    connect(m_gui->m_objectTree->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &OutlinerWidget::OnSelectionChanged);

    connect(m_gui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &OutlinerWidget::OnSearchTextChanged);
    connect(m_gui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &OutlinerWidget::OnFilterChanged);

    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);

    if (serializeContext)
    {
        AzToolsFramework::ComponentPaletteUtil::ComponentDataTable componentDataTable;
        AzToolsFramework::ComponentPaletteUtil::ComponentIconTable componentIconTable;
        AZStd::vector<AZ::ComponentServiceType> serviceFilter;

        AzToolsFramework::ComponentPaletteUtil::BuildComponentTables(serializeContext, AzToolsFramework::AppearsInGameComponentMenu, serviceFilter, componentDataTable, componentIconTable);

        for (const auto& categoryPair : componentDataTable)
        {
            const auto& componentMap = categoryPair.second;
            for (const auto& componentPair : componentMap)
            {
                m_gui->m_searchWidget->AddTypeFilter(categoryPair.first, componentPair.first, QVariant::fromValue<AZ::Uuid>(componentPair.second->m_typeId));
            }
        }
    }

    SetupActions();

    m_emptyIcon = QIcon();
    m_clearIcon = QIcon(":/AssetBrowser/Resources/close.png");

    m_listModel->Initialize();

    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
    EntityHighlightMessages::Bus::Handler::BusConnect();
    OutlinerModelNotificationBus::Handler::BusConnect();
    ToolsApplicationEvents::Bus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusConnect();
    AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
    AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusConnect();
    AzToolsFramework::EditorWindowUIRequestBus::Handler::BusConnect();
}

OutlinerWidget::~OutlinerWidget()
{
    AzToolsFramework::EditorWindowUIRequestBus::Handler::BusDisconnect();
    AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusDisconnect();
    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
    EntityHighlightMessages::Bus::Handler::BusDisconnect();
    OutlinerModelNotificationBus::Handler::BusDisconnect();
    ToolsApplicationEvents::Bus::Handler::BusDisconnect();
    AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();

    delete m_listModel;
    delete m_gui;
}

// Users should be able to drag an entity in the outliner without selecting it.
// This is useful to allow for dragging entities into AZ::EntityId fields in the component editor.
// There are two obvious ways to allow for this:
//  1-Update selection behavior in the outliner's tree view to not occur until mouse release.
//  2-Revert outliner's selection to the Editor's selection on drag actions and focus loss.
//      Also, copy outliner's tree view selection to the Editor's selection on mouse release.
//  Currently, the first behavior is implemented.
void OutlinerWidget::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (m_selectionChangeInProgress || !m_enableSelectionUpdates)
    {
        return;
    }

    AZ_PROFILE_FUNCTION(AzToolsFramework);

    AzToolsFramework::EntityIdList newlySelected;
    ExtractEntityIdsFromSelection(selected, newlySelected);

    AzToolsFramework::EntityIdList newlyDeselected;
    ExtractEntityIdsFromSelection(deselected, newlyDeselected);

    AzToolsFramework::ScopedUndoBatch undo("Select Entity");

    // initialize the selection command here to store the current selection before
    // new entities are selected or deselected below
    // (SelectionCommand calls GetSelectedEntities in the constructor)
    auto selectionCommand =
        AZStd::make_unique<AzToolsFramework::SelectionCommand>(AZStd::vector<AZ::EntityId>{}, "");

    // Add the newly selected and deselected entities from the outliner to the appropriate selection buffer.
    for (const AZ::EntityId& entityId : newlySelected)
    {
        m_entitiesSelectedByOutliner.insert(entityId);
    }

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::MarkEntitiesSelected, newlySelected);

    for (const AZ::EntityId& entityId : newlyDeselected)
    {
        m_entitiesDeselectedByOutliner.insert(entityId);
    }

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::MarkEntitiesDeselected, newlyDeselected);

    // call GetSelectedEntities again after all changes, and then update the selection
    // command  so the 'after' state is valid and up to date
    AzToolsFramework::EntityIdList selectedEntities;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

    selectionCommand->UpdateSelection(selectedEntities);

    selectionCommand->SetParent(undo.GetUndoBatch());
    selectionCommand.release();

    m_entitiesDeselectedByOutliner.clear();
    m_entitiesSelectedByOutliner.clear();
}

void OutlinerWidget::EntityHighlightRequested(AZ::EntityId entityId)
{
    if (!entityId.IsValid())
    {
        return;
    }

    // Pulse opacity and duration are hardcoded. It'd be great to sample this from designer-provided settings,
    // such as the globally-applied stylesheet.
    const float pulseOpacityStart = 0.5f;
    const int pulseLengthMilliseconds = 500;

    // Highlighting an entity in the outliner is accomplished by:
    //  1. Make sure the entity's row is actually visible.
    //      Accomplished with the handy "scrollTo" command.
    //  2. Create a single colored widget
    //      accomplished through setting the style sheet with a new background color.
    //  3. Apply an graphics opacity effect to this widget, so it can be made transparent.
    //  4. Creating a property animation to modify the opacity value over time, deleting the widget at 0 opacity.
    //  5. Setting this QWidget as an "index widget" to overlay over the entity row to be highlighted.

    const QModelIndex proxyIndex = GetIndexFromEntityId(entityId);
    if (!proxyIndex.isValid())
    {
        return;
    }

    // Scrolling to the entity will make sure that it is visible.
    // This will automatically open parents.
    m_gui->m_objectTree->scrollTo(proxyIndex);

    QWidget* testWidget = new QWidget(m_gui->m_objectTree);

    testWidget->setProperty("PulseHighlight", true);

    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(testWidget);
    effect->setOpacity(pulseOpacityStart);
    testWidget->setGraphicsEffect(effect);

    QPropertyAnimation* anim = new QPropertyAnimation(testWidget);
    anim->setTargetObject(effect);
    anim->setPropertyName("opacity");
    anim->setDuration(pulseLengthMilliseconds);
    anim->setStartValue(effect->opacity());
    anim->setEndValue(0);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    // Make sure to clean up the indexWidget when the anim is deleted
    connect(anim, &QObject::destroyed, this, [this, entityId, testWidget]()
    {
        const QModelIndex proxyIndex = GetIndexFromEntityId(entityId);
        if (proxyIndex.isValid())
        {
            QWidget* currentWidget = m_gui->m_objectTree->indexWidget(proxyIndex);
            if (currentWidget == testWidget)
            {
                m_gui->m_objectTree->setIndexWidget(proxyIndex, nullptr);
            }
        }
    });

    m_gui->m_objectTree->setIndexWidget(proxyIndex, testWidget);
}

// Currently a "strong highlight" request is handled by replacing the current selection with the requested entity.
void OutlinerWidget::EntityStrongHighlightRequested(AZ::EntityId entityId)
{
    // This can be sent from the Entity Inspector, and changing the selection can affect the state of that control
    // so use singleShot with 0 to queue the response until any other events sent to the Entity Inspector have been processed
    QTimer::singleShot(0, this, [entityId, this]() {
        const QModelIndex proxyIndex = GetIndexFromEntityId(entityId);
        if (!proxyIndex.isValid())
        {
            return;
        }

        // Scrolling to the entity will make sure that it is visible.
        // This will automatically open parents.
        m_gui->m_objectTree->scrollTo(proxyIndex);

        // Not setting "ignoreSignal" because this will set both outliner and world selections.
        m_gui->m_objectTree->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
    });
}

void OutlinerWidget::QueueUpdateSelection()
{
    if (!m_selectionChangeQueued)
    {
        QTimer::singleShot(0, this, &OutlinerWidget::UpdateSelection);
        m_selectionChangeQueued = true;
    }
}

void OutlinerWidget::UpdateSelection()
{
    if (m_selectionChangeQueued)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_selectionChangeInProgress = true;

        if (m_entitiesToDeselect.size() >= 1000)
        {
            // Calling Deselect for a large number of items is very slow,
            // use a single ClearAndSelect call instead.
            AZ_PROFILE_SCOPE(AzToolsFramework, "OutlinerWidget::ModelEntitySelectionChanged:ClearAndSelect");

            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);
            // it is expected that BuildSelectionFrom Entities results in the actual row-and-column selection.
            m_gui->m_objectTree->selectionModel()->select(
                BuildSelectionFromEntities(selectedEntities), QItemSelectionModel::ClearAndSelect);
        }
        else
        {
            {
                AZ_PROFILE_SCOPE(AzToolsFramework, "OutlinerWidget::ModelEntitySelectionChanged:Deselect");
                m_gui->m_objectTree->selectionModel()->select(
                    BuildSelectionFromEntities(m_entitiesToDeselect), QItemSelectionModel::Deselect);
            }
            {
                AZ_PROFILE_SCOPE(AzToolsFramework, "OutlinerWidget::ModelEntitySelectionChanged:Select");
                m_gui->m_objectTree->selectionModel()->select(
                    BuildSelectionFromEntities(m_entitiesToSelect), QItemSelectionModel::Select);
            }
        }

        // Scroll to selected entity when selecting from viewport
        if (m_entitiesToSelect.size() > 0 && m_scrollToSelectedEntity)
        {
            QueueScrollToNewContent(*m_entitiesToSelect.begin());
        }

        m_gui->m_objectTree->update();
        m_entitiesToSelect.clear();
        m_entitiesToDeselect.clear();
        m_selectionChangeQueued = false;
        m_selectionChangeInProgress = false;
    }
}

template <class EntityIdCollection>
QItemSelection OutlinerWidget::BuildSelectionFromEntities(const EntityIdCollection& entityIds)
{
    AZ_PROFILE_FUNCTION(AzToolsFramework);
    QItemSelection selection;

    for (const auto& entityId : entityIds)
    {
        const QModelIndex startOfRow = GetIndexFromEntityId(entityId);
        
        if (startOfRow.isValid())
        {
            // we select from start of row to end of row (ie, the last column), not just the first cell
            const QModelIndex endOfRow = m_proxyModel->index(startOfRow.row(), m_proxyModel->columnCount() - 1, startOfRow.parent());
            selection.select(startOfRow, endOfRow);
        }
    }

    return selection;
}

void OutlinerWidget::contextMenuEvent(QContextMenuEvent* event)
{
    AZ_PROFILE_FUNCTION(Editor);

    bool isDocumentOpen = false;
    EBUS_EVENT_RESULT(isDocumentOpen, AzToolsFramework::EditorRequests::Bus, IsLevelDocumentOpen);
    if (!isDocumentOpen)
    {
        return;
    }

    QMenu* contextMenu = new QMenu(this);

    // Populate global context menu.

    AzToolsFramework::EditorContextMenuBus::Broadcast(&AzToolsFramework::EditorContextMenuEvents::PopulateEditorGlobalContextMenu,
        contextMenu,
        AZ::Vector2::CreateZero(),
        AzToolsFramework::EditorEvents::eECMF_HIDE_ENTITY_CREATION | AzToolsFramework::EditorEvents::eECMF_USE_VIEWPORT_CENTER);

    PrepareSelection();

    // Remove the "Find in Entity Outliner" option from the context menu
    for (QAction* action : contextMenu->actions())
    {
        if (action->text() == "Find in Entity Outliner")
        {
            contextMenu->removeAction(action);
            break;
        }
    }

    //register rename menu action
    if (!m_selectedEntityIds.empty())
    {
        contextMenu->addSeparator();

        contextMenu->addAction(m_actionToRenameSelection);

        if (m_selectedEntityIds.size() == 1)
        {
            AZ::EntityId entityId = m_selectedEntityIds[0];

            AZ::EntityId parentId;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

            AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

            if (entityOrderArray.size() > 1)
            {
                if (AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId) != entityOrderArray.end())
                {
                    if (entityOrderArray.front() != entityId)
                    {
                        contextMenu->addAction(m_actionToMoveEntityUp);
                    }

                    if (entityOrderArray.back() != entityId)
                    {
                        contextMenu->addAction(m_actionToMoveEntityDown);
                    }
                }
            }
        }

        contextMenu->addSeparator();

        bool canGoToEntitiesInViewport = true;
        AzToolsFramework::EditorRequestBus::BroadcastResult(canGoToEntitiesInViewport, &AzToolsFramework::EditorRequestBus::Events::CanGoToSelectedEntitiesInViewports);
        if (!canGoToEntitiesInViewport)
        {
            m_actionGoToEntitiesInViewport->setEnabled(false);
            m_actionGoToEntitiesInViewport->setToolTip(QObject::tr("The selection contains no entities that exist in the viewport."));
        }
        else
        {
            m_actionGoToEntitiesInViewport->setEnabled(true);
            m_actionGoToEntitiesInViewport->setToolTip(QObject::tr("Moves the viewports to the bounding box for the selection."));
        }
        contextMenu->addAction(m_actionGoToEntitiesInViewport);        
    }

    // Register actions related to slice root selection in Outlinear list.
    {
        contextMenu->addSeparator();

        QMenu* selectSliceRootMenu = new QMenu(contextMenu);
        contextMenu->addMenu(selectSliceRootMenu);

        selectSliceRootMenu->setTitle(QObject::tr("Select slice root"));
        selectSliceRootMenu->setToolTip(QObject::tr("Selects a slice root in the Outliner list."));

        selectSliceRootMenu->addAction(m_actionToSelectTopSliceRoot);
        selectSliceRootMenu->addAction(m_actionToSelectSliceRootAboveSelection);
        selectSliceRootMenu->addAction(m_actionToSelectSliceRootBelowSelection);
        selectSliceRootMenu->addAction(m_actionToSelectBottomSliceRoot);

        {
            bool entitySelected = !m_selectedEntityIds.empty();
            m_actionToSelectSliceRootAboveSelection->setEnabled(entitySelected);
            m_actionToSelectSliceRootBelowSelection->setEnabled(entitySelected);
        }
        
        contextMenu->addSeparator();
    }

    contextMenu->exec(event->globalPos());
    delete contextMenu;
}

QString OutlinerWidget::FindCommonSliceAssetName(const AZStd::vector<AZ::EntityId>& entityList) const
{
    if (entityList.size() > 0)
    {
        //  Check if all selected items are in the same slice
        QString commonSliceName = m_listModel->GetSliceAssetName(entityList.front());
        if (!commonSliceName.isEmpty())
        {
            bool hasCommonSliceName = true;
            for (const AZ::EntityId& id : entityList)
            {
                QString sliceName = m_listModel->GetSliceAssetName(id);

                if (sliceName != commonSliceName)
                {
                    hasCommonSliceName = false;
                    break;
                }
            }

            if (hasCommonSliceName)
            {
                return commonSliceName;
            }
        }
    }

    return QString();
}

AzFramework::EntityContextId OutlinerWidget::GetPickModeEntityContextId()
{
    AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
    EBUS_EVENT_RESULT(editorEntityContextId, AzToolsFramework::EditorRequests::Bus, GetEntityContextId);

    return editorEntityContextId;
}

void OutlinerWidget::PrepareSelection()
{
    m_selectedEntityIds.clear();
    EBUS_EVENT_RESULT(m_selectedEntityIds, AzToolsFramework::ToolsApplicationRequests::Bus, GetSelectedEntities);
}

void OutlinerWidget::DoCreateEntity()
{
    PrepareSelection();

    if (m_selectedEntityIds.empty())
    {
        DoCreateEntityWithParent(AZ::EntityId());
        return;
    }

    if (m_selectedEntityIds.size() == 1)
    {
        DoCreateEntityWithParent(m_selectedEntityIds.front());
        return;
    }
}

void OutlinerWidget::DoCreateEntityWithParent(const AZ::EntityId& parentId)
{
    PrepareSelection();

    AZ::EntityId entityId;
    EBUS_EVENT_RESULT(entityId, AzToolsFramework::EditorRequests::Bus, CreateNewEntity, parentId);
}

void OutlinerWidget::DoShowSlice()
{
    PrepareSelection();

    if (!m_selectedEntityIds.empty())
    {
        QString commonSliceName = FindCommonSliceAssetName(m_selectedEntityIds);
        if (!commonSliceName.isEmpty())
        {
            EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, ShowAssetInBrowser, commonSliceName.toStdString().c_str());
        }
    }
}

void OutlinerWidget::DoDuplicateSelection()
{
    PrepareSelection();

    if (!m_selectedEntityIds.empty())
    {
        AzToolsFramework::ScopedUndoBatch undo("Duplicate Entity(s)");

        bool handled = false;
        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, CloneSelection, handled);
    }
}

void OutlinerWidget::DoDeleteSelection()
{
    PrepareSelection();

    EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, DeleteSelectedEntities, false);

    PrepareSelection();
}

void OutlinerWidget::DoDeleteSelectionAndDescendants()
{
    PrepareSelection();

    EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, DeleteSelectedEntities, true);

    PrepareSelection();
}

void OutlinerWidget::DoRenameSelection()
{
    PrepareSelection();

    if (m_selectedEntityIds.size() == 1)
    {
        const QModelIndex proxyIndex = GetIndexFromEntityId(m_selectedEntityIds.front());
        if (proxyIndex.isValid())
        {
            m_gui->m_objectTree->setCurrentIndex(proxyIndex);
            m_gui->m_objectTree->QTreeView::edit(proxyIndex);
        }
    }
    else
    {
        bool okPressed = false;
        QString outputText = QInputDialog::getText(this, tr("Rename"), tr("Name:"), QLineEdit::Normal, tr(""), &okPressed);
        if (okPressed && !outputText.isEmpty())
        {
            AZStd::string newName(outputText.toUtf8().constData());

            AzToolsFramework::ScopedUndoBatch undo("Rename Entities");
            bool entitiesRenamed = false;

            for (AZ::EntityId entityId : m_selectedEntityIds)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

                if (entity)
                {
                    AZStd::string_view oldName = entity->GetName();

                    if (oldName != newName)
                    {
                        entity->SetName(newName);
                        undo.MarkEntityDirty(entity->GetId());
                        entitiesRenamed = true;
                    }
                }
            }

            if (entitiesRenamed)
            {
                // In all other cases during a rename it triggers an entireTree refresh, so we're going to do the same here
                AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
                    AzToolsFramework::Refresh_EntireTree);
            }
        }
    }
}

void OutlinerWidget::DoMoveEntityUp()
{
    PrepareSelection();

    if (m_selectedEntityIds.size() != 1)
    {
        return;
    }

    AZ::EntityId entityId = m_selectedEntityIds[0];
    AZ_Assert(entityId.IsValid(), "Invalid entity selected to move up");
    if (!entityId.IsValid())
    {
        return;
    }

    AZ::EntityId parentId;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

    AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

    auto entityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId);
    if (entityOrderArray.empty() || entityOrderArray.front() == entityId || entityItr == entityOrderArray.end())
    {
        return;
    }

    AzToolsFramework::ScopedUndoBatch undo("Move Entity Up");

    // Parent is dirty due to sort change
    undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(parentId));

    AZStd::swap(*entityItr, *(entityItr - 1));
    AzToolsFramework::SetEntityChildOrder(parentId, entityOrderArray);
}

void OutlinerWidget::DoMoveEntityDown()
{
    PrepareSelection();

    if (m_selectedEntityIds.size() != 1)
    {
        return;
    }

    AZ::EntityId entityId = m_selectedEntityIds[0];
    AZ_Assert(entityId.IsValid(), "Invalid entity selected to move down");
    if (!entityId.IsValid())
    {
        return;
    }

    AZ::EntityId parentId;
    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

    AzToolsFramework::EntityOrderArray entityOrderArray = AzToolsFramework::GetEntityChildOrder(parentId);

    auto entityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId);
    if (entityOrderArray.empty() || entityOrderArray.back() == entityId || entityItr == entityOrderArray.end())
    {
        return;
    }

    AzToolsFramework::ScopedUndoBatch undo("Move Entity Down");

    // Parent is dirty due to sort change
    undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(parentId));

    AZStd::swap(*entityItr, *(entityItr + 1));
    AzToolsFramework::SetEntityChildOrder(parentId, entityOrderArray);
}

void OutlinerWidget::GoToEntitiesInViewport()
{
    AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::GoToSelectedEntitiesInViewports);
}

void OutlinerWidget::DoSelectSliceRootNextToSelection(bool isTraversalUpwards)
{
    const auto treeView = m_gui->m_objectTree;

    QModelIndex currentIdx = treeView->currentIndex();
    if (!currentIdx.isValid())
    {
        return;
    }

    AZ::EntityId currentEntityId;
    bool currentIdxSelected = false;
    currentEntityId = GetEntityIdFromIndex(currentIdx);

    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
        currentIdxSelected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, currentEntityId);

    if (!currentIdxSelected)
    {
        return;
    }

    AZStd::function<QModelIndex(QModelIndex)> getNextIdxFunction =
        AZStd::bind(isTraversalUpwards ? &QTreeView::indexAbove : &QTreeView::indexBelow, treeView, AZStd::placeholders::_1);
    QModelIndex nextIdx = getNextIdxFunction(currentIdx);
    bool foundSliceRoot = false;

    while (nextIdx.isValid() && !foundSliceRoot)
    {
        currentIdx = nextIdx;
        currentEntityId = GetEntityIdFromIndex(currentIdx);

        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            foundSliceRoot, &AzToolsFramework::ToolsApplicationRequests::IsSliceRootEntity, currentEntityId);

        nextIdx = getNextIdxFunction(currentIdx);
    }

    if (foundSliceRoot)
    {
        SetIndexAsCurrentAndSelected(currentIdx);
    }
}

void OutlinerWidget::DoSelectSliceRootAboveSelection()
{
    DoSelectSliceRootNextToSelection(true);
}


void OutlinerWidget::DoSelectSliceRootBelowSelection()
{
    DoSelectSliceRootNextToSelection(false);
}

void OutlinerWidget::DoSelectEdgeSliceRoot(bool shouldSelectTopMostSlice)
{
    const auto treeView = m_gui->m_objectTree;
    const auto itemModel = treeView->model();
    if (itemModel->rowCount() == 0)
    {
        return;
    }

    QModelIndex currentIdx;
    AZStd::function<QModelIndex(QModelIndex)> getNextIdxFunction;
    if (shouldSelectTopMostSlice)
    {
        currentIdx = itemModel->index(0, OutlinerListModel::ColumnName);

        getNextIdxFunction =
            AZStd::bind(&QTreeView::indexBelow, treeView, AZStd::placeholders::_1);
    }
    else
    {
        currentIdx = itemModel->index(itemModel->rowCount() - 1, OutlinerListModel::ColumnName);
        while (itemModel->rowCount(currentIdx) > 0 && treeView->isExpanded(currentIdx))
        {
            currentIdx = itemModel->index(itemModel->rowCount(currentIdx) - 1, OutlinerListModel::ColumnName, currentIdx);
        }

        getNextIdxFunction =
            AZStd::bind(&QTreeView::indexAbove, treeView, AZStd::placeholders::_1);
    }

    QModelIndex nextIdx = currentIdx;
    bool foundSliceRoot = false;
    AZ::EntityId currentEntityId;
    do
    {
        currentIdx = nextIdx;
        currentEntityId = GetEntityIdFromIndex(currentIdx);

        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            foundSliceRoot, &AzToolsFramework::ToolsApplicationRequests::IsSliceRootEntity, currentEntityId);
        nextIdx = getNextIdxFunction(currentIdx);
    } while (nextIdx.isValid() && !foundSliceRoot);

    if (foundSliceRoot)
    {
        SetIndexAsCurrentAndSelected(currentIdx);
    }
}


void OutlinerWidget::DoSelectTopSliceRoot()
{
    DoSelectEdgeSliceRoot(true);
}

void OutlinerWidget::DoSelectBottomSliceRoot()
{
    DoSelectEdgeSliceRoot(false);
}

void OutlinerWidget::SetIndexAsCurrentAndSelected(const QModelIndex& index)
{
    m_gui->m_objectTree->setCurrentIndex(index);

    AZ::EntityId entityId = GetEntityIdFromIndex(index);

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{ entityId });
}

void OutlinerWidget::SetupActions()
{
    m_actionToShowSlice = new QAction(tr("Show Slice"), this);
    m_actionToShowSlice->setShortcut(tr("Ctrl+Shift+F"));
    m_actionToShowSlice->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToShowSlice, &QAction::triggered, this, &OutlinerWidget::DoShowSlice);
    addAction(m_actionToShowSlice);

    m_actionToCreateEntity = new QAction(tr("Create Entity"), this);
    m_actionToCreateEntity->setShortcut(tr("Ctrl+Alt+N"));
    m_actionToCreateEntity->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToCreateEntity, &QAction::triggered, this, &OutlinerWidget::DoCreateEntity);
    addAction(m_actionToCreateEntity);

    m_actionToDeleteSelection = new QAction(tr("Delete"), this);
    m_actionToDeleteSelection->setShortcut(QKeySequence("Shift+Delete"));
    m_actionToDeleteSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToDeleteSelection, &QAction::triggered, this, &OutlinerWidget::DoDeleteSelection);
    addAction(m_actionToDeleteSelection);

    m_actionToDeleteSelectionAndDescendants = new QAction(tr("Delete Selection And Descendants"), this);
    m_actionToDeleteSelectionAndDescendants->setShortcut(QKeySequence::Delete);
    m_actionToDeleteSelectionAndDescendants->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToDeleteSelectionAndDescendants, &QAction::triggered, this, &OutlinerWidget::DoDeleteSelectionAndDescendants);
    addAction(m_actionToDeleteSelectionAndDescendants);

    m_actionToRenameSelection = new QAction(tr("Rename"), this);
#ifdef AZ_PLATFORM_MAC
    // "Alt+Return" translates to Option+Return on macOS
    m_actionToRenameSelection->setShortcut(tr("Alt+Return"));
#else
    m_actionToRenameSelection->setShortcut(tr("F2"));
#endif
    m_actionToRenameSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToRenameSelection, &QAction::triggered, this, &OutlinerWidget::DoRenameSelection);
    addAction(m_actionToRenameSelection);

    m_actionToMoveEntityUp = new QAction(tr("Move up"), this);
    m_actionToMoveEntityUp->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToMoveEntityUp, &QAction::triggered, this, &OutlinerWidget::DoMoveEntityUp);
    addAction(m_actionToMoveEntityUp);

    m_actionToMoveEntityDown = new QAction(tr("Move down"), this);
    m_actionToMoveEntityDown->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToMoveEntityDown, &QAction::triggered, this, &OutlinerWidget::DoMoveEntityDown);
    addAction(m_actionToMoveEntityDown);

    m_actionGoToEntitiesInViewport = new QAction(tr("Find in viewport"), this);
    m_actionGoToEntitiesInViewport->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionGoToEntitiesInViewport, &QAction::triggered, this, &OutlinerWidget::GoToEntitiesInViewport);
    addAction(m_actionGoToEntitiesInViewport);

    m_actionToSelectSliceRootAboveSelection = new QAction(tr("Up"), this);
    m_actionToSelectSliceRootAboveSelection->setToolTip(tr("Select the first slice root above the selection in the Outliner list."));
    m_actionToSelectSliceRootAboveSelection->setShortcut(QKeySequence("Ctrl+Up"));
    m_actionToSelectSliceRootAboveSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToSelectSliceRootAboveSelection, &QAction::triggered, this, &OutlinerWidget::DoSelectSliceRootAboveSelection);
    addAction(m_actionToSelectSliceRootAboveSelection);

    m_actionToSelectSliceRootBelowSelection = new QAction(tr("Down"), this);
    m_actionToSelectSliceRootBelowSelection->setToolTip(tr("Select the first slice root below the selection in the Outliner list."));
    m_actionToSelectSliceRootBelowSelection->setShortcut(QKeySequence("Ctrl+Down"));
    m_actionToSelectSliceRootBelowSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToSelectSliceRootBelowSelection, &QAction::triggered, this, &OutlinerWidget::DoSelectSliceRootBelowSelection);
    addAction(m_actionToSelectSliceRootBelowSelection);

    m_actionToSelectTopSliceRoot = new QAction(tr("Top"), this);
    m_actionToSelectTopSliceRoot->setToolTip(tr("Select the slice root at the top of the Outliner list."));
    m_actionToSelectTopSliceRoot->setShortcut(QKeySequence("Ctrl+Home"));
    m_actionToSelectTopSliceRoot->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToSelectTopSliceRoot, &QAction::triggered, this, &OutlinerWidget::DoSelectTopSliceRoot);
    addAction(m_actionToSelectTopSliceRoot);

    m_actionToSelectBottomSliceRoot = new QAction(tr("Bottom"), this);
    m_actionToSelectBottomSliceRoot->setToolTip(tr("Select the slice root at the bottom of the Outliner list."));
    m_actionToSelectBottomSliceRoot->setShortcut(QKeySequence("Ctrl+End"));
    m_actionToSelectBottomSliceRoot->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_actionToSelectBottomSliceRoot, &QAction::triggered, this, &OutlinerWidget::DoSelectBottomSliceRoot);
    addAction(m_actionToSelectBottomSliceRoot);
}

void OutlinerWidget::SelectSliceRoot()
{
   MainWindow::instance()->OnGotoSliceRoot();
}

void OutlinerWidget::OnEntityPickModeStarted()
{
    m_gui->m_objectTree->setDragEnabled(false);
    m_gui->m_objectTree->setSelectionMode(QAbstractItemView::NoSelection);
    m_gui->m_objectTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_inObjectPickMode = true;
}

void OutlinerWidget::OnEntityPickModeStopped()
{
    m_gui->m_objectTree->setDragEnabled(true);
    m_gui->m_objectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gui->m_objectTree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_inObjectPickMode = false;
}

void OutlinerWidget::OnTreeItemClicked(const QModelIndex &index)
{
    if (m_inObjectPickMode)
    {
        const AZ::EntityId entityId = GetEntityIdFromIndex(index);
        if (entityId.IsValid())
        {
            AzToolsFramework::EditorPickModeRequestBus::Broadcast(
                &AzToolsFramework::EditorPickModeRequests::PickModeSelectEntity, entityId);
        }

        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);
    }

    switch (index.column())
    {
        case OutlinerListModel::ColumnVisibilityToggle:
        case OutlinerListModel::ColumnLockToggle:
            // Don't care passing an explicit check state as the model is just toggling on its own
            m_gui->m_objectTree->model()->setData(index, Qt::CheckState(), Qt::CheckStateRole);
            break;
    }
}

void OutlinerWidget::OnTreeItemExpanded(const QModelIndex &index)
{
    m_listModel->OnEntityExpanded(GetEntityIdFromIndex(index));
}

void OutlinerWidget::OnTreeItemCollapsed(const QModelIndex &index)
{
    m_listModel->OnEntityCollapsed(GetEntityIdFromIndex(index));
}

void OutlinerWidget::OnExpandEntity(const AZ::EntityId& entityId, bool expand)
{
    m_gui->m_objectTree->setExpanded(GetIndexFromEntityId(entityId), expand);
}

void OutlinerWidget::OnSelectEntity(const AZ::EntityId& entityId, bool selected)
{
    if (selected)
    {
        if (m_entitiesSelectedByOutliner.find(entityId) == m_entitiesSelectedByOutliner.end())
        {
            m_entitiesToSelect.insert(entityId);
            m_entitiesToDeselect.erase(entityId);
        }
    }
    else
    {
        if (m_entitiesDeselectedByOutliner.find(entityId) == m_entitiesDeselectedByOutliner.end())
        {
            m_entitiesToSelect.erase(entityId);
            m_entitiesToDeselect.insert(entityId);
        }
    }
    QueueUpdateSelection();
}

void OutlinerWidget::OnEnableSelectionUpdates(bool enable)
{
    m_enableSelectionUpdates = enable;
}

void OutlinerWidget::OnDropEvent()
{
    m_dropOperationInProgress = true;
    m_listModel->SetDropOperationInProgress(true);
}

void OutlinerWidget::OnEditorEntityCreated(const AZ::EntityId& entityId)
{
    QueueContentUpdateSort(entityId);
    QueueScrollToNewContent(entityId);
 
    // When a new entity is created we need to make sure to apply the filter
    m_listModel->FilterEntity(entityId);
}

void OutlinerWidget::ScrollToNewContent()
{
    m_scrollToNewContentQueued = false;
    
    // Scroll to the parent entity when auto-expand is disabled and the user doesn't click on the "Find in Entity Outliner" option in the context menu
    if (!m_expandSelectedEntity && !m_focusInEntityOutliner)
    {
        AZ::EntityId parentId = m_scrollToEntityId;
        while (parentId.IsValid())
        {
            m_scrollToEntityId = parentId;
            parentId.SetInvalid();
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, m_scrollToEntityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
        }
    }

    const QModelIndex proxyIndex = GetIndexFromEntityId(m_scrollToEntityId);
    if (!proxyIndex.isValid())
    {
        QueueScrollToNewContent(m_scrollToEntityId);
        return;
    }

    m_gui->m_objectTree->scrollTo(proxyIndex, QAbstractItemView::PositionAtTop);
    m_focusInEntityOutliner = false;
}

void OutlinerWidget::QueueScrollToNewContent(const AZ::EntityId& entityId)
{
    if (m_dropOperationInProgress)
    {
        //don't scroll as the result of a drop operation, in order to stop new parent auto expanding. You can only drop in visible areas anyway. 
        m_dropOperationInProgress = false;
        m_listModel->SetDropOperationInProgress(false);
        return;
    }

    if (entityId.IsValid())
    {
        m_scrollToEntityId = entityId;
        if (!m_scrollToNewContentQueued)
        {
            m_scrollToNewContentQueued = true;
            QTimer::singleShot(queuedChangeDelay, this, &OutlinerWidget::ScrollToNewContent);
        }
    }
}

AZ::EntityId OutlinerWidget::GetEntityIdFromIndex(const QModelIndex& index) const
{
    if (index.isValid())
    {
        const QModelIndex modelIndex = m_proxyModel->mapToSource(index);
        if (modelIndex.isValid())
        {
            return m_listModel->GetEntityFromIndex(modelIndex);
        }
    }

    return AZ::EntityId();
}

QModelIndex OutlinerWidget::GetIndexFromEntityId(const AZ::EntityId& entityId) const
{
    if (entityId.IsValid())
    {
        QModelIndex modelIndex = m_listModel->GetIndexFromEntity(entityId, 0);
        if (modelIndex.isValid())
        {
            return m_proxyModel->mapFromSource(modelIndex);
        }
    }

    return QModelIndex();
}

void OutlinerWidget::ExtractEntityIdsFromSelection(const QItemSelection& selection, AzToolsFramework::EntityIdList& entityIdList) const
{
    entityIdList.reserve(selection.indexes().count());
    for (const auto& index : selection.indexes())
    {
        // Skip any column except the main name column...
        if (index.column() == OutlinerListModel::ColumnName)
        {
            const AZ::EntityId entityId = GetEntityIdFromIndex(index);
            if (entityId.IsValid())
            {
                entityIdList.push_back(entityId);
            }
        }
    }
}

void OutlinerWidget::OnSearchTextChanged(const QString& activeTextFilter)
{
    AZ_PROFILE_FUNCTION(AzToolsFramework);
    AZStd::string filterString = activeTextFilter.toUtf8().data();

    m_listModel->SearchStringChanged(filterString);
    m_proxyModel->UpdateFilter();
}

void OutlinerWidget::OnFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
{
    AZStd::vector<OutlinerListModel::ComponentTypeValue> componentFilters;
    componentFilters.reserve(activeTypeFilters.count());

    for (auto filter : activeTypeFilters)
    {
        OutlinerListModel::ComponentTypeValue value;
        value.m_uuid = qvariant_cast<AZ::Uuid>(filter.metadata);
        value.m_globalVal = filter.globalFilterValue;
        componentFilters.push_back(value);
    }

    m_listModel->SearchFilterChanged(componentFilters);
    m_proxyModel->UpdateFilter();
}

void OutlinerWidget::InvalidateFilter()
{
    m_listModel->InvalidateFilter();
    m_proxyModel->UpdateFilter();
}

void OutlinerWidget::ClearFilter()
{
    m_gui->m_searchWidget->ClearTextFilter();
    m_gui->m_searchWidget->ClearTypeFilter();
}

void OutlinerWidget::OnStartPlayInEditor()
{
    setEnabled(false);
}

void OutlinerWidget::OnStopPlayInEditor()
{
    setEnabled(true);
}

static void SetEntityOutlinerState(Ui::OutlinerWidgetUI* entityOutlinerUi, const bool on)
{
    AzQtComponents::SetWidgetInteractEnabled(entityOutlinerUi->m_objectTree, on);
    AzQtComponents::SetWidgetInteractEnabled(entityOutlinerUi->m_searchWidget, on);
}

void OutlinerWidget::EnableUi(bool enable)
{
    SetEntityOutlinerState(m_gui, enable);
    setEnabled(enable);
}

void OutlinerWidget::SetEditorUiEnabled(bool enable)
{
    EnableUi(enable);
}

void OutlinerWidget::OnEditorModeActivated(
    [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
{
    if (mode == AzToolsFramework::ViewportEditorMode::Component)
    {
        EnableUi(false);
    }
}

void OutlinerWidget::OnEditorModeDeactivated(
    [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
{
    if (mode == AzToolsFramework::ViewportEditorMode::Component)
    {
        EnableUi(true);
    }
}

void OutlinerWidget::OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& /*ticket*/)
{
    const AZ::SliceComponent::SliceReference* reference = sliceAddress.GetReference();
    const AZ::SliceComponent::SliceInstance* instance = sliceAddress.GetInstance();
    if (reference && instance)
    {
        // guard against special case slices (e.g. default slice) that produce technically valid, but garbage, slice instances
        const AZ::SliceComponent::SliceReference::SliceInstances& sliceInstances = reference->GetInstances();
        if (sliceInstances.find(*instance) != sliceInstances.end())
        {
            for (const AZ::Entity* entity : instance->GetInstantiated()->m_entities)
            {
                QueueContentUpdateSort(entity->GetId());
            }
        }
    }
}

void OutlinerWidget::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId /*parentId*/, AZ::EntityId childId)
{
    QueueContentUpdateSort(childId);
}

void OutlinerWidget::OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& /*name*/)
{
    QueueContentUpdateSort(entityId);   
}

void OutlinerWidget::QueueContentUpdateSort(const AZ::EntityId& entityId)
{
    if (m_sortMode != EntityOutliner::DisplaySortMode::Manually && entityId.IsValid())
    {
        m_entitiesToSort.insert(entityId);
        if (!m_sortContentQueued)
        {
            m_sortContentQueued = true;
            QTimer::singleShot(queuedChangeDelay, this, &OutlinerWidget::SortContent);
        }
    }
}

void OutlinerWidget::SortContent()
{
    AZ_PROFILE_FUNCTION(AzToolsFramework);

    m_sortContentQueued = false;

    // kick out early if we somehow managed to get here with manual sorting set
    if (m_sortMode == EntityOutliner::DisplaySortMode::Manually)
    {
        m_entitiesToSort.clear();
        return;
    }

    AzToolsFramework::EntityIdSet parentsToSort;
    for (const AZ::EntityId& entityId : m_entitiesToSort)
    {
        AZ::EntityId parentId; 
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(parentId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
        parentsToSort.insert(parentId);
    }
    m_entitiesToSort.clear();

    auto comparer = AZStd::bind(&CompareEntitiesForSorting, AZStd::placeholders::_1, AZStd::placeholders::_2, m_sortMode);
    for (const AZ::EntityId& entityId : parentsToSort)
    {
        SortEntityChildren(entityId, comparer);
    }
}

void OutlinerWidget::OnSortModeChanged(EntityOutliner::DisplaySortMode sortMode)
{
    if (m_sortMode == sortMode)
    {
        return;
    }

    if (sortMode != EntityOutliner::DisplaySortMode::Manually)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        auto comparer = AZStd::bind(&CompareEntitiesForSorting, AZStd::placeholders::_1, AZStd::placeholders::_2, sortMode);
        SortEntityChildrenRecursively(AZ::EntityId(), comparer);
    }

    m_sortMode = sortMode;
    m_listModel->SetSortMode(m_sortMode);
}

void OutlinerWidget::OnDisplayOptionChanged(EntityOutliner::DisplayOption displayOption, bool enable)
{
    switch (displayOption)
    {
        case EntityOutliner::DisplayOption::AutoScroll:
            m_scrollToSelectedEntity = enable;
            break;

        case EntityOutliner::DisplayOption::AutoExpand:
            m_expandSelectedEntity = enable;
            m_listModel->EnableAutoExpand(enable);
            break;

        default:
            AZ_Assert(false, "Unknown display option event received (%d)", static_cast<int>(displayOption));
            break;
    }
}

void OutlinerWidget::OnFocusInEntityOutliner(const AzToolsFramework::EntityIdList& entityIdList)
{
    m_focusInEntityOutliner = true;

    QModelIndex firstSelectedEntityIndex = GetIndexFromEntityId(entityIdList.front());
    for (const AZ::EntityId& entityId : entityIdList)
    {
        QModelIndex index = GetIndexFromEntityId(entityId);
        firstSelectedEntityIndex = index < firstSelectedEntityIndex ? index : firstSelectedEntityIndex;
    }

    QueueScrollToNewContent(GetEntityIdFromIndex(firstSelectedEntityIndex));
}

#include <UI/Outliner/moc_OutlinerWidget.cpp>
