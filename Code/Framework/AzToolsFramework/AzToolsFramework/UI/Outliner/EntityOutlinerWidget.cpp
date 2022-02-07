/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Outliner/EntityOutlinerWidget.hxx>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/sort.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerDisplayOptionsMenu.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerListModel.hxx>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerSortFilterProxyModel.hxx>

#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Utilities/QtViewPaneEffects.h>

#include <QApplication>
#include <QDir>
#include <QGraphicsOpacityEffect>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <QToolButton>

#include <AzToolsFramework/UI/Outliner/ui_EntityOutlinerWidget.h>

// This has to live outside of any namespaces due to issues on Linux with calls to Q_INIT_RESOURCE if they are inside a namespace
void initEntityOutlinerWidgetResources()
{
    Q_INIT_RESOURCE(resources);
}

namespace
{
    const int queuedChangeDelay = 16; // milliseconds

    using EntityIdCompareFunc = AZStd::function<bool(AZ::EntityId, AZ::EntityId)>;

    bool CompareEntitiesForSorting(AZ::EntityId leftEntityId, AZ::EntityId rightEntityId, AzToolsFramework::EntityOutliner::DisplaySortMode sortMode)
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

        if (sortMode == AzToolsFramework::EntityOutliner::DisplaySortMode::AtoZ)
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

namespace AzToolsFramework
{
    EntityOutlinerWidget::EntityOutlinerWidget(QWidget* pParent, Qt::WindowFlags flags)
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
        initEntityOutlinerWidgetResources();

        m_editorEntityUiInterface = AZ::Interface<AzToolsFramework::EditorEntityUiInterface>::Get();
        AZ_Assert(m_editorEntityUiInterface != nullptr, "EntityOutlinerWidget requires a EditorEntityUiInterface instance on Initialize.");

        m_gui = new Ui::EntityOutlinerWidgetUI();
        m_gui->setupUi(this);

        AZ::IO::FixedMaxPath engineRootPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        }

        QDir rootDir = QString::fromUtf8(engineRootPath.c_str(), aznumeric_cast<int>(engineRootPath.Native().size()));
        const auto pathOnDisk = rootDir.absoluteFilePath(QStringLiteral("Code/Framework/AzToolsFramework/AzToolsFramework/UI/Outliner/"));
        const QStringView qrcPath = QStringLiteral(":/EntityOutliner");

        // Setting the style sheet using both methods allows faster style iteration speed for
        // developers. The style will be loaded from a Qt Resource file if Editor is installed, but
        // developers with the file on disk will be able to modify the style and have it automatically
        // reloaded.
        AzQtComponents::StyleManager::addSearchPaths("outliner", pathOnDisk, qrcPath.toString(), engineRootPath);
        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("outliner:EntityOutliner.qss"));

        m_listModel = aznew EntityOutlinerListModel(this);
        m_listModel->SetSortMode(m_sortMode);

        const int autoExpandDelayMilliseconds = 2500;
        m_gui->m_objectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
        SetDefaultTreeViewEditTriggers();
        m_gui->m_objectTree->setAutoExpandDelay(autoExpandDelayMilliseconds);
        m_gui->m_objectTree->setDragEnabled(true);
        m_gui->m_objectTree->setDropIndicatorShown(true);
        m_gui->m_objectTree->setAcceptDrops(true);
        m_gui->m_objectTree->setDragDropOverwriteMode(false);
        m_gui->m_objectTree->setDragDropMode(QAbstractItemView::DragDrop);
        m_gui->m_objectTree->setDefaultDropAction(Qt::CopyAction);
        m_gui->m_objectTree->setExpandsOnDoubleClick(false);
        m_gui->m_objectTree->setContextMenuPolicy(Qt::CustomContextMenu);
        m_gui->m_objectTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_gui->m_objectTree->setAutoScrollMargin(20);
        m_gui->m_objectTree->setIndentation(24);
        m_gui->m_objectTree->setRootIsDecorated(false);
        connect(m_gui->m_objectTree, &QTreeView::customContextMenuRequested, this, &EntityOutlinerWidget::OnOpenTreeContextMenu);

        // custom item delegate
        m_gui->m_objectTree->setItemDelegate(aznew EntityOutlinerItemDelegate(m_gui->m_objectTree));

        m_proxyModel = aznew EntityOutlinerSortFilterProxyModel(this);
        m_proxyModel->setSourceModel(m_listModel);

        m_gui->m_objectTree->setModel(m_proxyModel);

        // Link up signals for informing the model of tree changes using the proxy as an intermediary
        connect(m_gui->m_objectTree, &QTreeView::clicked, this, &EntityOutlinerWidget::OnTreeItemClicked);
        connect(m_gui->m_objectTree, &QTreeView::doubleClicked, this, &EntityOutlinerWidget::OnTreeItemDoubleClicked);
        connect(m_gui->m_objectTree, &QTreeView::expanded, this, &EntityOutlinerWidget::OnTreeItemExpanded);
        connect(m_gui->m_objectTree, &QTreeView::collapsed, this, &EntityOutlinerWidget::OnTreeItemCollapsed);
        connect(m_gui->m_objectTree, &EntityOutlinerTreeView::ItemDropped, this, &EntityOutlinerWidget::OnDropEvent);
        connect(m_listModel, &EntityOutlinerListModel::ExpandEntity, this, &EntityOutlinerWidget::OnExpandEntity);
        connect(m_listModel, &EntityOutlinerListModel::SelectEntity, this, &EntityOutlinerWidget::OnSelectEntity);
        connect(m_listModel, &EntityOutlinerListModel::EnableSelectionUpdates, this, &EntityOutlinerWidget::OnEnableSelectionUpdates);
        connect(m_listModel, &EntityOutlinerListModel::ResetFilter, this, &EntityOutlinerWidget::ClearFilter);
        connect(m_listModel, &EntityOutlinerListModel::ReapplyFilter, this, &EntityOutlinerWidget::InvalidateFilter);

        QToolButton* display_options = new QToolButton(this);
        display_options->setObjectName(QStringLiteral("m_display_options"));
        display_options->setPopupMode(QToolButton::InstantPopup);
        display_options->setAutoRaise(true);

        m_gui->m_searchWidget->AddWidgetToSearchWidget(display_options);

        // Set the display options menu
        display_options->setMenu(m_displayOptionsMenu);
        connect(m_displayOptionsMenu, &EntityOutliner::DisplayOptionsMenu::OnSortModeChanged, this, &EntityOutlinerWidget::OnSortModeChanged);
        connect(m_displayOptionsMenu, &EntityOutliner::DisplayOptionsMenu::OnOptionToggled, this, &EntityOutlinerWidget::OnDisplayOptionChanged);

        // Sorting is performed in a very specific way by the proxy model.
        // Disable sort indicator so user isn't confused by the fact
        // that they can't actually change how sorting works.
        m_gui->m_objectTree->hideColumn(EntityOutlinerListModel::ColumnSortIndex);
        m_gui->m_objectTree->setSortingEnabled(true);
        m_gui->m_objectTree->header()->setSortIndicatorShown(false);
        m_gui->m_objectTree->header()->setStretchLastSection(false);

        // Always expand root entity (level entity) - needed if the widget is re-created while a level is already open.
        m_gui->m_objectTree->expand(m_proxyModel->index(0, 0));

        // resize the icon columns so that the Visibility and Lock toggle icon columns stay right-justified
        m_gui->m_objectTree->header()->setStretchLastSection(false);
        m_gui->m_objectTree->header()->setMinimumSectionSize(0);
        m_gui->m_objectTree->header()->setSectionResizeMode(EntityOutlinerListModel::ColumnName, QHeaderView::Stretch);
        m_gui->m_objectTree->header()->setSectionResizeMode(EntityOutlinerListModel::ColumnVisibilityToggle, QHeaderView::Fixed);
        m_gui->m_objectTree->header()->resizeSection(EntityOutlinerListModel::ColumnVisibilityToggle, 20);
        m_gui->m_objectTree->header()->setSectionResizeMode(EntityOutlinerListModel::ColumnLockToggle, QHeaderView::Fixed);
        m_gui->m_objectTree->header()->resizeSection(EntityOutlinerListModel::ColumnLockToggle, 24);

        connect(m_gui->m_objectTree->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &EntityOutlinerWidget::OnSelectionChanged);

        connect(m_gui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &EntityOutlinerWidget::OnSearchTextChanged);
        connect(m_gui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &EntityOutlinerWidget::OnFilterChanged);

        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);

        if (serializeContext)
        {
            ComponentPaletteUtil::ComponentDataTable componentDataTable;
            ComponentPaletteUtil::ComponentIconTable componentIconTable;
            AZStd::vector<AZ::ComponentServiceType> serviceFilter;

            ComponentPaletteUtil::BuildComponentTables(serializeContext, AppearsInGameComponentMenu, serviceFilter, componentDataTable, componentIconTable);

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

        EditorPickModeNotificationBus::Handler::BusConnect(GetEntityContextId());
        EntityHighlightMessages::Bus::Handler::BusConnect();
        EntityOutlinerModelNotificationBus::Handler::BusConnect();
        ToolsApplicationEvents::Bus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        ViewportEditorModeNotificationsBus::Handler::BusConnect(GetEntityContextId());
        EditorEntityInfoNotificationBus::Handler::BusConnect();
        Prefab::PrefabPublicNotificationBus::Handler::BusConnect();
        EditorWindowUIRequestBus::Handler::BusConnect();
    }

    EntityOutlinerWidget::~EntityOutlinerWidget()
    {
        EditorWindowUIRequestBus::Handler::BusDisconnect();
        Prefab::PrefabPublicNotificationBus::Handler::BusDisconnect();
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
        EditorPickModeNotificationBus::Handler::BusDisconnect();
        EntityHighlightMessages::Bus::Handler::BusDisconnect();
        EntityOutlinerModelNotificationBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();

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
    void EntityOutlinerWidget::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        if (m_selectionChangeInProgress || !m_enableSelectionUpdates
            || (selected.empty() && deselected.empty()))
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AzToolsFramework);

        EntityIdList newlySelected;
        ExtractEntityIdsFromSelection(selected, newlySelected);

        EntityIdList newlyDeselected;
        ExtractEntityIdsFromSelection(deselected, newlyDeselected);

        ScopedUndoBatch undo("Select Entity");

        // initialize the selection command here to store the current selection before
        // new entities are selected or deselected below
        // (SelectionCommand calls GetSelectedEntities in the constructor)
        auto selectionCommand =
            AZStd::make_unique<SelectionCommand>(AZStd::vector<AZ::EntityId>{}, "");

        // Add the newly selected and deselected entities from the outliner to the appropriate selection buffer.
        for (const AZ::EntityId& entityId : newlySelected)
        {
            m_entitiesSelectedByOutliner.insert(entityId);
        }

        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::MarkEntitiesSelected, newlySelected);

        for (const AZ::EntityId& entityId : newlyDeselected)
        {
            m_entitiesDeselectedByOutliner.insert(entityId);
        }

        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::MarkEntitiesDeselected, newlyDeselected);

        // call GetSelectedEntities again after all changes, and then update the selection
        // command  so the 'after' state is valid and up to date
        EntityIdList selectedEntities;
        ToolsApplicationRequests::Bus::BroadcastResult(
            selectedEntities, &ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

        selectionCommand->UpdateSelection(selectedEntities);

        selectionCommand->SetParent(undo.GetUndoBatch());
        selectionCommand.release();

        m_entitiesDeselectedByOutliner.clear();
        m_entitiesSelectedByOutliner.clear();
    }

    void EntityOutlinerWidget::EntityHighlightRequested(AZ::EntityId entityId)
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
    void EntityOutlinerWidget::EntityStrongHighlightRequested(AZ::EntityId entityId)
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

    void EntityOutlinerWidget::QueueUpdateSelection()
    {
        if (!m_selectionChangeQueued)
        {
            QTimer::singleShot(0, this, &EntityOutlinerWidget::UpdateSelection);
            m_selectionChangeQueued = true;
        }
    }

    void EntityOutlinerWidget::UpdateSelection()
    {
        if (m_selectionChangeQueued)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            m_selectionChangeInProgress = true;

            if (m_entitiesToDeselect.size() >= 1000)
            {
                // Calling Deselect for a large number of items is very slow,
                // use a single ClearAndSelect call instead.
                AZ_PROFILE_SCOPE(AzToolsFramework, "EntityOutlinerWidget::ModelEntitySelectionChanged:ClearAndSelect");

                EntityIdList selectedEntities;
                ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::Bus::Events::GetSelectedEntities);
                // it is expected that BuildSelectionFrom Entities results in the actual row-and-column selection.
                m_gui->m_objectTree->selectionModel()->select(
                    BuildSelectionFromEntities(selectedEntities), QItemSelectionModel::ClearAndSelect);
            }
            else
            {
                {
                    AZ_PROFILE_SCOPE(AzToolsFramework, "EntityOutlinerWidget::ModelEntitySelectionChanged:Deselect");
                    m_gui->m_objectTree->selectionModel()->select(
                        BuildSelectionFromEntities(m_entitiesToDeselect), QItemSelectionModel::Deselect);
                }
                {
                    AZ_PROFILE_SCOPE(AzToolsFramework, "EntityOutlinerWidget::ModelEntitySelectionChanged:Select");
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
    QItemSelection EntityOutlinerWidget::BuildSelectionFromEntities(const EntityIdCollection& entityIds)
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

    void EntityOutlinerWidget::OnOpenTreeContextMenu(const QPoint& pos)
    {
        AZ_PROFILE_FUNCTION(Editor);

        bool isDocumentOpen = false;
        EBUS_EVENT_RESULT(isDocumentOpen, EditorRequests::Bus, IsLevelDocumentOpen);
        if (!isDocumentOpen)
        {
            return;
        }

        // Do not display the context menu if the item under the mouse cursor is not selectable.
        if (const QModelIndex& index = m_gui->m_objectTree->indexAt(pos); index.isValid()
            && (index.flags() & Qt::ItemIsSelectable) == 0)
        {
            return;
        }

        QMenu* contextMenu = new QMenu(this);

        // Populate global context menu.
        AzToolsFramework::EditorContextMenuBus::Broadcast(&AzToolsFramework::EditorContextMenuEvents::PopulateEditorGlobalContextMenu,
            contextMenu,
            AZ::Vector2::CreateZero(),
            EditorEvents::eECMF_HIDE_ENTITY_CREATION | EditorEvents::eECMF_USE_VIEWPORT_CENTER);

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

            if (m_selectedEntityIds.size() == 1)
            {
                auto entityId = m_selectedEntityIds.front();
                auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId);

                if (!entityUiHandler || entityUiHandler->CanRename(entityId))
                {
                    contextMenu->addAction(m_actionToRenameSelection);
                }
            }

            if (m_selectedEntityIds.size() == 1)
            {
                AZ::EntityId entityId = m_selectedEntityIds[0];

                AZ::EntityId parentId;
                EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

                EntityOrderArray entityOrderArray = GetEntityChildOrder(parentId);

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
            EditorRequestBus::BroadcastResult(canGoToEntitiesInViewport, &EditorRequestBus::Events::CanGoToSelectedEntitiesInViewports);
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

        contextMenu->exec(m_gui->m_objectTree->mapToGlobal(pos));
        delete contextMenu;
    }

    AzFramework::EntityContextId EntityOutlinerWidget::GetPickModeEntityContextId()
    {
        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        EBUS_EVENT_RESULT(editorEntityContextId, EditorRequests::Bus, GetEntityContextId);

        return editorEntityContextId;
    }

    void EntityOutlinerWidget::PrepareSelection()
    {
        m_selectedEntityIds.clear();
        EBUS_EVENT_RESULT(m_selectedEntityIds, ToolsApplicationRequests::Bus, GetSelectedEntities);
    }

    void EntityOutlinerWidget::DoCreateEntity()
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

    void EntityOutlinerWidget::DoCreateEntityWithParent(const AZ::EntityId& parentId)
    {
        PrepareSelection();

        AZ::EntityId entityId;
        EBUS_EVENT_RESULT(entityId, EditorRequests::Bus, CreateNewEntity, parentId);
    }

    void EntityOutlinerWidget::DoDuplicateSelection()
    {
        PrepareSelection();

        if (!m_selectedEntityIds.empty())
        {
            ScopedUndoBatch undo("Duplicate Entity(s)");

            bool handled = false;
            EBUS_EVENT(EditorRequests::Bus, CloneSelection, handled);
        }
    }

    void EntityOutlinerWidget::DoDeleteSelection()
    {
        PrepareSelection();

        EBUS_EVENT(EditorRequests::Bus, DeleteSelectedEntities, false);

        PrepareSelection();
    }

    void EntityOutlinerWidget::DoDeleteSelectionAndDescendants()
    {
        PrepareSelection();

        EBUS_EVENT(EditorRequests::Bus, DeleteSelectedEntities, true);

        PrepareSelection();
    }

    void EntityOutlinerWidget::DoRenameSelection()
    {
        PrepareSelection();

        if (m_selectedEntityIds.size() == 1)
        {
            auto entityId = m_selectedEntityIds.front();
            auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId);

            if (!entityUiHandler || entityUiHandler->CanRename(entityId))
            {
                const QModelIndex proxyIndex = GetIndexFromEntityId(entityId);
                if (proxyIndex.isValid())
                {
                    m_gui->m_objectTree->setCurrentIndex(proxyIndex);
                    m_gui->m_objectTree->QTreeView::edit(proxyIndex);
                }
            }
        }
    }

    void EntityOutlinerWidget::DoMoveEntityUp()
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
        EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

        EntityOrderArray entityOrderArray = GetEntityChildOrder(parentId);

        auto entityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId);
        if (entityOrderArray.empty() || entityOrderArray.front() == entityId || entityItr == entityOrderArray.end())
        {
            return;
        }

        ScopedUndoBatch undo("Move Entity Up");

        // Parent is dirty due to sort change
        undo.MarkEntityDirty(GetEntityIdForSortInfo(parentId));

        AZStd::swap(*entityItr, *(entityItr - 1));
        SetEntityChildOrder(parentId, entityOrderArray);
    }

    void EntityOutlinerWidget::DoMoveEntityDown()
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
        EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

        EntityOrderArray entityOrderArray = GetEntityChildOrder(parentId);

        auto entityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), entityId);
        if (entityOrderArray.empty() || entityOrderArray.back() == entityId || entityItr == entityOrderArray.end())
        {
            return;
        }

        ScopedUndoBatch undo("Move Entity Down");

        // Parent is dirty due to sort change
        undo.MarkEntityDirty(GetEntityIdForSortInfo(parentId));

        AZStd::swap(*entityItr, *(entityItr + 1));
        SetEntityChildOrder(parentId, entityOrderArray);
    }

    void EntityOutlinerWidget::GoToEntitiesInViewport()
    {
        EditorRequestBus::Broadcast(&EditorRequestBus::Events::GoToSelectedEntitiesInViewports);
    }

    void EntityOutlinerWidget::SetIndexAsCurrentAndSelected(const QModelIndex& index)
    {
        m_gui->m_objectTree->setCurrentIndex(index);

        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, EntityIdList{ entityId });
    }

    void EntityOutlinerWidget::SetupActions()
    {
        m_actionToCreateEntity = new QAction(tr("Create Entity"), this);
        m_actionToCreateEntity->setShortcut(tr("Ctrl+Alt+N"));
        m_actionToCreateEntity->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToCreateEntity, &QAction::triggered, this, &EntityOutlinerWidget::DoCreateEntity);
        addAction(m_actionToCreateEntity);

        m_actionToDeleteSelection = new QAction(tr("Delete"), this);
        m_actionToDeleteSelection->setShortcut(QKeySequence("Shift+Delete"));
        m_actionToDeleteSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToDeleteSelection, &QAction::triggered, this, &EntityOutlinerWidget::DoDeleteSelection);
        addAction(m_actionToDeleteSelection);

        m_actionToDeleteSelectionAndDescendants = new QAction(tr("Delete Selection And Descendants"), this);
        m_actionToDeleteSelectionAndDescendants->setShortcut(QKeySequence::Delete);
        m_actionToDeleteSelectionAndDescendants->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToDeleteSelectionAndDescendants, &QAction::triggered, this, &EntityOutlinerWidget::DoDeleteSelectionAndDescendants);
        addAction(m_actionToDeleteSelectionAndDescendants);

        m_actionToRenameSelection = new QAction(tr("Rename"), this);
    #if defined(Q_OS_MAC)
        // "Alt+Return" translates to Option+Return on macOS
        m_actionToRenameSelection->setShortcut(tr("Alt+Return"));
    #elif defined(Q_OS_WIN)
        m_actionToRenameSelection->setShortcut(tr("F2"));
    #endif
        m_actionToRenameSelection->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToRenameSelection, &QAction::triggered, this, &EntityOutlinerWidget::DoRenameSelection);
        addAction(m_actionToRenameSelection);

        m_actionToMoveEntityUp = new QAction(tr("Move up"), this);
        m_actionToMoveEntityUp->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveEntityUp, &QAction::triggered, this, &EntityOutlinerWidget::DoMoveEntityUp);
        addAction(m_actionToMoveEntityUp);

        m_actionToMoveEntityDown = new QAction(tr("Move down"), this);
        m_actionToMoveEntityDown->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionToMoveEntityDown, &QAction::triggered, this, &EntityOutlinerWidget::DoMoveEntityDown);
        addAction(m_actionToMoveEntityDown);

        m_actionGoToEntitiesInViewport = new QAction(tr("Find in viewport"), this);
        m_actionGoToEntitiesInViewport->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_actionGoToEntitiesInViewport, &QAction::triggered, this, &EntityOutlinerWidget::GoToEntitiesInViewport);
        addAction(m_actionGoToEntitiesInViewport);
    }

    void EntityOutlinerWidget::SetDefaultTreeViewEditTriggers()
    {
        m_gui->m_objectTree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    }

    void EntityOutlinerWidget::OnEntityPickModeStarted()
    {
        m_gui->m_objectTree->setDragEnabled(false);
        m_gui->m_objectTree->setSelectionMode(QAbstractItemView::NoSelection);
        m_gui->m_objectTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_inObjectPickMode = true;
    }

    void EntityOutlinerWidget::OnEntityPickModeStopped()
    {
        m_gui->m_objectTree->setDragEnabled(true);
        m_gui->m_objectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
        SetDefaultTreeViewEditTriggers();
        m_inObjectPickMode = false;
    }

    void EntityOutlinerWidget::OnTreeItemClicked(const QModelIndex& index)
    {
        if (m_inObjectPickMode)
        {
            const AZ::EntityId entityId = GetEntityIdFromIndex(index);
            if (entityId.IsValid())
            {
                EditorPickModeRequestBus::Broadcast(
                    &EditorPickModeRequests::PickModeSelectEntity, entityId);
            }

            EditorPickModeRequestBus::Broadcast(
                &EditorPickModeRequests::StopEntityPickMode);
            return;
        }

        switch (index.column())
        {
            case EntityOutlinerListModel::ColumnVisibilityToggle:
            case EntityOutlinerListModel::ColumnLockToggle:
                // Don't care passing an explicit check state as the model is just toggling on its own
                m_gui->m_objectTree->model()->setData(index, Qt::CheckState(), Qt::CheckStateRole);
                break;
        }
    }

    void EntityOutlinerWidget::OnTreeItemDoubleClicked(const QModelIndex& index)
    {
        if (AZ::EntityId entityId = GetEntityIdFromIndex(index); auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId))
        {
            entityUiHandler->OnEntityDoubleClick(entityId);
        }
    }

    void EntityOutlinerWidget::OnTreeItemExpanded(const QModelIndex& index)
    {
        AZ::EntityId entityId = GetEntityIdFromIndex(index);
        if (auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId))
        {
            entityUiHandler->OnOutlinerItemExpand(index);
        }

        m_listModel->OnEntityExpanded(entityId);
    }

    void EntityOutlinerWidget::OnTreeItemCollapsed(const QModelIndex& index)
    {
        AZ::EntityId entityId = GetEntityIdFromIndex(index);
        if (auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId))
        {
            entityUiHandler->OnOutlinerItemCollapse(index);
        }

        m_listModel->OnEntityCollapsed(entityId);
    }

    void EntityOutlinerWidget::OnExpandEntity(const AZ::EntityId& entityId, bool expand)
    {
        m_gui->m_objectTree->setExpanded(GetIndexFromEntityId(entityId), expand);
    }

    void EntityOutlinerWidget::OnSelectEntity(const AZ::EntityId& entityId, bool selected)
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

    void EntityOutlinerWidget::OnEnableSelectionUpdates(bool enable)
    {
        m_enableSelectionUpdates = enable;
    }

    void EntityOutlinerWidget::OnDropEvent()
    {
        m_dropOperationInProgress = true;
        m_listModel->SetDropOperationInProgress(true);
    }

    void EntityOutlinerWidget::OnEditorEntityCreated(const AZ::EntityId& entityId)
    {
        QueueContentUpdateSort(entityId);
        QueueScrollToNewContent(entityId);

        // When a new entity is created we need to make sure to apply the filter
        m_listModel->FilterEntity(entityId);
    }

    void EntityOutlinerWidget::ScrollToNewContent()
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
                EditorEntityInfoRequestBus::EventResult(parentId, m_scrollToEntityId, &EditorEntityInfoRequestBus::Events::GetParent);
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

    void EntityOutlinerWidget::QueueScrollToNewContent(const AZ::EntityId& entityId)
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
                QTimer::singleShot(queuedChangeDelay, this, &EntityOutlinerWidget::ScrollToNewContent);
            }
        }
    }

    AZ::EntityId EntityOutlinerWidget::GetEntityIdFromIndex(const QModelIndex& index) const
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

    QModelIndex EntityOutlinerWidget::GetIndexFromEntityId(const AZ::EntityId& entityId) const
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

    void EntityOutlinerWidget::ExtractEntityIdsFromSelection(const QItemSelection& selection, EntityIdList& entityIdList) const
    {
        entityIdList.reserve(selection.indexes().count());
        for (const auto& index : selection.indexes())
        {
            // Skip any column except the main name column...
            if (index.column() == EntityOutlinerListModel::ColumnName)
            {
                const AZ::EntityId entityId = GetEntityIdFromIndex(index);
                if (entityId.IsValid())
                {
                    entityIdList.push_back(entityId);
                }
            }
        }
    }

    void EntityOutlinerWidget::OnSearchTextChanged(const QString& activeTextFilter)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AZStd::string filterString = activeTextFilter.toUtf8().data();

        m_listModel->SearchStringChanged(filterString);
        m_proxyModel->UpdateFilter();
    }

    void EntityOutlinerWidget::OnFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
    {
        AZStd::vector<EntityOutlinerListModel::ComponentTypeValue> componentFilters;
        componentFilters.reserve(activeTypeFilters.count());

        for (auto filter : activeTypeFilters)
        {
            EntityOutlinerListModel::ComponentTypeValue value;
            value.m_uuid = qvariant_cast<AZ::Uuid>(filter.metadata);
            value.m_globalVal = filter.globalFilterValue;
            componentFilters.push_back(value);
        }

        m_listModel->SearchFilterChanged(componentFilters);
        m_proxyModel->UpdateFilter();
    }

    void EntityOutlinerWidget::InvalidateFilter()
    {
        m_listModel->InvalidateFilter();
        m_proxyModel->UpdateFilter();
    }

    void EntityOutlinerWidget::ClearFilter()
    {
        m_gui->m_searchWidget->ClearTextFilter();
        m_gui->m_searchWidget->ClearTypeFilter();
    }

    void EntityOutlinerWidget::OnStartPlayInEditor()
    {
        setEnabled(false);
    }

    void EntityOutlinerWidget::OnStopPlayInEditor()
    {
        setEnabled(true);
    }

    static void SetEntityOutlinerState(Ui::EntityOutlinerWidgetUI* entityOutlinerUi, const bool on)
    {
        AzQtComponents::SetWidgetInteractEnabled(entityOutlinerUi->m_objectTree, on);
        AzQtComponents::SetWidgetInteractEnabled(entityOutlinerUi->m_searchWidget, on);
    }

    void EntityOutlinerWidget::EnableUi(bool enable)
    {
        SetEntityOutlinerState(m_gui, enable);
        setEnabled(enable);
    }

    void EntityOutlinerWidget::SetEditorUiEnabled(bool enable)
    {
        EnableUi(enable);
    }

    void EntityOutlinerWidget::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            EnableUi(false);
        }
    }

    void EntityOutlinerWidget::OnEditorModeDeactivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            EnableUi(true);
        }
    }

    void EntityOutlinerWidget::OnPrefabInstancePropagationBegin()
    {
        m_gui->m_objectTree->setUpdatesEnabled(false);
    }

    void EntityOutlinerWidget::OnPrefabInstancePropagationEnd()
    {
        QTimer::singleShot(1, this, [this]() {
            m_gui->m_objectTree->setUpdatesEnabled(true);
            m_gui->m_objectTree->expand(m_proxyModel->index(0,0));
        });
    }

    void EntityOutlinerWidget::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId /*parentId*/, AZ::EntityId childId)
    {
        QueueContentUpdateSort(childId);
    }

    void EntityOutlinerWidget::OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& /*name*/)
    {
        QueueContentUpdateSort(entityId);
    }

    void EntityOutlinerWidget::QueueContentUpdateSort(const AZ::EntityId& entityId)
    {
        if (m_sortMode != EntityOutliner::DisplaySortMode::Manually && entityId.IsValid())
        {
            m_entitiesToSort.insert(entityId);
            if (!m_sortContentQueued)
            {
                m_sortContentQueued = true;
                QTimer::singleShot(queuedChangeDelay, this, &EntityOutlinerWidget::SortContent);
            }
        }
    }

    void EntityOutlinerWidget::SortContent()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_sortContentQueued = false;

        // kick out early if we somehow managed to get here with manual sorting set
        if (m_sortMode == EntityOutliner::DisplaySortMode::Manually)
        {
            m_entitiesToSort.clear();
            return;
        }

        EntityIdSet parentsToSort;
        for (const AZ::EntityId& entityId : m_entitiesToSort)
        {
            AZ::EntityId parentId;
            EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);
            parentsToSort.insert(parentId);
        }
        m_entitiesToSort.clear();

        auto comparer = AZStd::bind(&CompareEntitiesForSorting, AZStd::placeholders::_1, AZStd::placeholders::_2, m_sortMode);
        for (const AZ::EntityId& entityId : parentsToSort)
        {
            SortEntityChildren(entityId, comparer);
        }
    }

    void EntityOutlinerWidget::OnSortModeChanged(EntityOutliner::DisplaySortMode sortMode)
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

    void EntityOutlinerWidget::OnDisplayOptionChanged(EntityOutliner::DisplayOption displayOption, bool enable)
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

    void EntityOutlinerWidget::OnFocusInEntityOutliner(const EntityIdList& entityIdList)
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

}

#include <UI/Outliner/moc_EntityOutlinerWidget.cpp>
