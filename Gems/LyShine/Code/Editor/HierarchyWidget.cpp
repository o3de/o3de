/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "AssetDropHelpers.h"
#include "QtHelpers.h"
#include <AzCore/Asset/AssetManager.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>

#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>

HierarchyWidget::HierarchyWidget(EditorWindow* editorWindow)
    : AzQtComponents::StyledTreeWidget()
    , m_isDeleting(false)
    , m_editorWindow(editorWindow)
    , m_entityItemMap()
    , m_itemBeingHovered(nullptr)
    , m_inDragStartState(false)
    , m_selectionChangedBeforeDrag(false)
    , m_signalSelectionChange(true)
    , m_inObjectPickMode(false)
    , m_isInited(false)
{
    setMouseTracking(true);

    // Style.
    {
        setAcceptDrops(true);
        setDropIndicatorShown(true);
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragDrop);
        setSelectionMode(QAbstractItemView::ExtendedSelection);

        setColumnCount(kHierarchyColumnCount);
        setHeader(new HierarchyHeader(this));

        // IMPORTANT: This MUST be done here.
        // This CAN'T be done inside HierarchyHeader.
        header()->setSectionsClickable(true);

        header()->setSectionResizeMode(kHierarchyColumnName, QHeaderView::Stretch);
        header()->setSectionResizeMode(kHierarchyColumnIsVisible, QHeaderView::Fixed);
        header()->setSectionResizeMode(kHierarchyColumnIsSelectable, QHeaderView::Fixed);

        // This controls the width of the last 2 columns; both in the header and in the body of the HierarchyWidget.
        header()->resizeSection(kHierarchyColumnIsVisible, UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE);
        header()->resizeSection(kHierarchyColumnIsSelectable, UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE);
    }

    // Connect signals.
    {
        // Selection change notification.
        QObject::connect(selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            SLOT(CurrentSelectionHasChanged(const QItemSelection &, const QItemSelection &)));

        QObject::connect(model(),
            SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)),
            SLOT(DataHasChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));
    }

    QObject::connect(this,
        &QTreeWidget::itemClicked,
        this,
        [this](QTreeWidgetItem* item, int column)
        {
            HierarchyItem* i = HierarchyItem::RttiCast(item);

            if (column == kHierarchyColumnIsVisible)
            {
                ToggleVisibility(i);
            }
            else if (column == kHierarchyColumnIsSelectable)
            {
                CommandHierarchyItemToggleIsSelectable::Push(m_editorWindow->GetActiveStack(),
                    this,
                    HierarchyItemRawPtrList({i}));
            }
            else if (m_inObjectPickMode)
            {
                PickItem(i);
            }
        });

    QObject::connect(this,
        &QTreeWidget::itemExpanded,
        this,
        [this](QTreeWidgetItem* item)
        {
            CommandHierarchyItemToggleIsExpanded::Push(m_editorWindow->GetActiveStack(),
                this,
                HierarchyItem::RttiCast(item));
        });

    QObject::connect(this,
        &QTreeWidget::itemCollapsed,
        this,
        [this](QTreeWidgetItem* item)
        {
            CommandHierarchyItemToggleIsExpanded::Push(m_editorWindow->GetActiveStack(),
                this,
                HierarchyItem::RttiCast(item));
        });

    EntityHighlightMessages::Bus::Handler::BusConnect();
}

HierarchyWidget::~HierarchyWidget()
{
    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
    EntityHighlightMessages::Bus::Handler::BusDisconnect();
}

void HierarchyWidget::SetIsDeleting(bool b)
{
    m_isDeleting = b;
}

EntityHelpers::EntityToHierarchyItemMap& HierarchyWidget::GetEntityItemMap()
{
    return m_entityItemMap;
}

EditorWindow* HierarchyWidget::GetEditorWindow()
{
    return m_editorWindow;
}

void HierarchyWidget::ActiveCanvasChanged()
{
    EntityContextChanged();
}

void HierarchyWidget::EntityContextChanged()
{
    if (m_inObjectPickMode)
    {
        OnEntityPickModeStopped();
    }

    // Disconnect from the PickModeRequests bus and reconnect with the new entity context
    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
    UiEditorEntityContext* context = m_editorWindow->GetEntityContext();
    if (context)
    {
        AzToolsFramework::EditorPickModeNotificationBus::Handler::BusConnect(context->GetContextId());
    }
}

void HierarchyWidget::CreateItems(const LyShine::EntityArray& elements)
{
    std::list<AZ::Entity*> elementList(elements.begin(), elements.end());

    // Build the rest of the list.
    // Note: This is a breadth-first traversal through all child elements.
    for (auto& e : elementList)
    {
        LyShine::EntityArray childElements;
        UiElementBus::EventResult(childElements, e->GetId(), &UiElementBus::Events::GetChildElements);
        elementList.insert(elementList.end(), childElements.begin(), childElements.end());
    }

    // Create the items.
    for (auto& e : elementList)
    {
        AZ::Entity* parentElement = EntityHelpers::GetParentElement(e);
        QTreeWidgetItem* parent = HierarchyHelpers::ElementToItem(this, parentElement, true);
        AZ_Assert(parent, "No parent widget item found for parent entity");

        int childIndex = -1;
        UiElementBus::EventResult(childIndex, parentElement->GetId(), &UiElementBus::Events::GetIndexOfChild, e);

        new HierarchyItem(m_editorWindow,
            *parent,
            childIndex,
            e->GetName().c_str(),
            e);
    }

    // restore the expanded state of all items
    ApplyElementIsExpanded();

    m_isInited = true;
}

void HierarchyWidget::RecreateItems(const LyShine::EntityArray& elements)
{
    // remember the currently selected items so we can restore them
    EntityHelpers::EntityIdList selectedEntityIds = SelectionHelpers::GetSelectedElementIds(this,
        selectedItems(), false);

    ClearItems();

    CreateItems(elements);

    HierarchyHelpers::SetSelectedItems(this, &selectedEntityIds);
}

void HierarchyWidget::ClearItems()
{
    ClearAllHierarchyItemEntityIds();

    // Remove all the items from the list (doesn't delete Entities since we cleared the EntityIds)
    clear();

    // The map needs to be cleared here since HandleItemRemove won't remove the map entry due to the entity Ids being cleared above
    m_entityItemMap.clear();

    m_isInited = false;
}

AZ::Entity* HierarchyWidget::CurrentSelectedElement() const
{
    auto currentItem = HierarchyItem::RttiCast(QTreeWidget::currentItem());
    AZ::Entity* currentElement = (currentItem && currentItem->isSelected()) ? currentItem->GetElement() : nullptr;
    return currentElement;
}

void HierarchyWidget::contextMenuEvent(QContextMenuEvent* ev)
{
    // The context menu.
    if (m_isInited)
    {
        HierarchyMenu contextMenu(this,
            (HierarchyMenu::Show::kCutCopyPaste |
             HierarchyMenu::Show::kNew_EmptyElement |
             HierarchyMenu::Show::kDeleteElement |
             HierarchyMenu::Show::kNewSlice |
             HierarchyMenu::Show::kNew_InstantiateSlice |
             HierarchyMenu::Show::kPushToSlice |
             HierarchyMenu::Show::kFindElements |
             HierarchyMenu::Show::kEditorOnly),
            true);

        contextMenu.exec(ev->globalPos());
    }

    QTreeWidget::contextMenuEvent(ev);
}

void HierarchyWidget::SignalUserSelectionHasChanged(const QTreeWidgetItemRawPtrQList& selectedItems)
{
    HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(this,
            selectedItems);
    SetUserSelection(items.empty() ? nullptr : &items);
}

void HierarchyWidget::CurrentSelectionHasChanged([[maybe_unused]] const QItemSelection& selected,
    [[maybe_unused]] const QItemSelection& deselected)
{
    m_selectionChangedBeforeDrag = true;

    // IMPORTANT: This signal is triggered at the right time, but
    // "selected.indexes()" DOESN'T contain ALL the items currently
    // selected. It ONLY contains the newly selected items. To avoid
    // having to track what's added and removed to the selection,
    // we'll use selectedItems().

    if (m_signalSelectionChange && !m_isDeleting)
    {
        SignalUserSelectionHasChanged(selectedItems());
    }
}

void HierarchyWidget::DataHasChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, [[maybe_unused]] const QVector<int>& roles)
{
    if (topLeft == bottomRight)
    {
        // We only care about text changes, which can ONLY be done one at a
        // time. This implies that topLeft must be the same as bottomRight.

        HierarchyItem* hierarchyItem = HierarchyItem::RttiCast(itemFromIndex(topLeft));
        AZ::Entity* element = hierarchyItem->GetElement();
        AZ_Assert(element, "No entity found for hierarchy item");
        AZ::EntityId entityId = element->GetId();
        QTreeWidgetItem* item = HierarchyHelpers::ElementToItem(this, element, false);
        QString toName(item ? item->text(0) : "");

        CommandHierarchyItemRename::Push(m_editorWindow->GetActiveStack(),
            this,
            entityId,
            element->GetName().c_str(),
            toName);
    }
}

void HierarchyWidget::HandleItemAdd(HierarchyItem* item)
{
    m_entityItemMap[ item->GetEntityId() ] = item;
}

void HierarchyWidget::HandleItemRemove(HierarchyItem* item)
{
    if (item == m_itemBeingHovered)
    {
        m_itemBeingHovered = nullptr;
    }

    m_entityItemMap.erase(item->GetEntityId());
}

void HierarchyWidget::ClearAllHierarchyItemEntityIds()
{
    // as a simple way of going through all the HierarchyItem's we use the
    // EntityHelpers::EntityToHierarchyItemMap
    for (auto mapItem : m_entityItemMap)
    {
        mapItem.second->ClearEntityId();
    }
}

void HierarchyWidget::ApplyElementIsExpanded()
{
    // Seed the list.
    HierarchyItemRawPtrList allItems;
    HierarchyHelpers::AppendAllChildrenToEndOfList(invisibleRootItem(), allItems);

    // Traverse the list.
    blockSignals(true);
    {
        HierarchyHelpers::TraverseListAndAllChildren(allItems,
            [](HierarchyItem* childItem)
            {
                childItem->ApplyElementIsExpanded();
            });
    }
    blockSignals(false);
}

void HierarchyWidget::mousePressEvent(QMouseEvent* ev)
{
    m_selectionChangedBeforeDrag = false;

    HierarchyItem* item = HierarchyItem::RttiCast(itemAt(ev->pos()));
    if (!item)
    {
        // This allows the user to UNSELECT an item
        // by clicking in an empty area of the widget.
        SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);
    }

    // Remember the selected items before the selection change in case a drag is started.
    // When dragging outside the hierarchy, the selection is reverted back to this selection
    m_beforeDragSelection = selectedItems();

    m_signalSelectionChange = false;

    QTreeWidget::mousePressEvent(ev);

    m_signalSelectionChange = true;
}

void HierarchyWidget::mouseDoubleClickEvent(QMouseEvent* ev)
{
    HierarchyItem* item = HierarchyItem::RttiCast(itemAt(ev->pos()));
    if (item)
    {
        // Double-clicking to edit text is only allowed in the FIRST column.
        for (int col = kHierarchyColumnIsVisible; col < kHierarchyColumnCount; ++col)
        {
            QRect r = visualRect(indexFromItem(item, col));
            if (r.contains(ev->pos()))
            {
                // Ignore the event.
                return;
            }
        }
    }

    QTreeWidget::mouseDoubleClickEvent(ev);
}

void HierarchyWidget::startDrag(Qt::DropActions supportedActions)
{
    // This flag is used to determine whether to perform an action on leaveEvent.
    // If an item is dragged really fast outside the hierarchy, this startDrag event is called,
    // but the dragEnterEvent and dragLeaveEvent are replaced with the leaveEvent
    m_inDragStartState = true;

    // Remember the current selection so that we can revert back to it when the items are dragged back into the hierarchy
    m_dragSelection = selectedItems();

    AzQtComponents::StyledTreeWidget::startDrag(supportedActions);
}

void HierarchyWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (!AcceptsMimeData(event->mimeData()))
    {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
    {
        m_inDragStartState = false;

        if (m_selectionChangedBeforeDrag)
        {
            m_signalSelectionChange = false;

            // Set the current selection to the items being dragged
            clearSelection();
            for (auto i : m_dragSelection)
            {
                i->setSelected(true);
            }

            m_signalSelectionChange = true;
        }
    }
    else
    {
        // Dragging an item from outside the hierarchy window
        m_selectionChangedBeforeDrag = false;
    }

    QTreeView::dragEnterEvent(event);
}

void HierarchyWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    // This is called when dragging outside the hierarchy, or when a drag is released inside the hierarchy
    // but a dropEvent isn't called (ex. drop item onto itself or press Esc to cancel a drag)

    // Check if mouse position is inside or outside the hierarchy
    QRect widgetRect = geometry();
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    if (widgetRect.contains(mousePos))
    {
        if (m_selectionChangedBeforeDrag)
        {
            // Treat this event as a mouse release (mouseReleaseEvent is not called in this case)
            SignalUserSelectionHasChanged(selectedItems());
        }
    }
    else
    {
        if (m_selectionChangedBeforeDrag)
        {
            m_signalSelectionChange = false;

            // Set the current selection to the items that were selected before the drag
            clearSelection();
            for (auto i : m_beforeDragSelection)
            {
                i->setSelected(true);
            }

            m_signalSelectionChange = true;
        }
    }

    QTreeView::dragLeaveEvent(event);
}

void HierarchyWidget::dropEvent(QDropEvent* ev)
{
    if (!m_isInited)
    {
        return;
    }

    if (ev->mimeData()->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
    {
        m_inDragStartState = false;

        m_signalSelectionChange = false;

        // Get a list of selected items
        QTreeWidgetItemRawPtrQList selection = selectedItems();

        // Change current selection to only contain top level items. This avoids
        // the default drop behavior from changing the internal hierarchy of 
        // the dragged elements
        QTreeWidgetItemRawPtrQList topLevelSelection;
        SelectionHelpers::GetListOfTopLevelSelectedItems(this, selection, topLevelSelection);
        clearSelection();
        for (auto i : topLevelSelection)
        {
            i->setSelected(true);
        }

        // Set current parent and child index of each selected item
        for (auto i : selection)
        {
            HierarchyItem* item = HierarchyItem::RttiCast(i);
            if (item)
            {
                QModelIndex itemIndex = indexFromItem(item);

                QTreeWidgetItem* baseParentItem = itemFromIndex(itemIndex.parent());
                if (!baseParentItem)
                {
                    baseParentItem = invisibleRootItem();
                }
                HierarchyItem* parentItem = HierarchyItem::RttiCast(baseParentItem);
                AZ::EntityId parentId = (parentItem ? parentItem->GetEntityId() : AZ::EntityId());

                item->SetPreMove(parentId, itemIndex.row());
            }
        }

        // Do the drop event
        ev->setDropAction(Qt::MoveAction);
        QTreeWidget::dropEvent(ev);

        // Make a list of selected items and their parents
        HierarchyItemRawPtrList childItems;
        QTreeWidgetItemRawPtrList baseParentItems;

        bool itemMoved = false;

        for (auto i : selection)
        {
            HierarchyItem* item = HierarchyItem::RttiCast(i);
            if (item)
            {
                QModelIndex index = indexFromItem(item);

                QTreeWidgetItem* baseParentItem = itemFromIndex(index.parent());
                if (!baseParentItem)
                {
                    baseParentItem = invisibleRootItem();
                }
                HierarchyItem* parentItem = HierarchyItem::RttiCast(baseParentItem);
                AZ::EntityId parentId = parentItem ? parentItem->GetEntityId() : AZ::EntityId();

                if ((item->GetPreMoveChildRow() != index.row()) || (item->GetPreMoveParentId() != parentId))
                {
                    // Item has moved
                    itemMoved = true;
                }

                childItems.push_back(item);
                baseParentItems.push_back(baseParentItem);
            }
        }

        if (itemMoved)
        {
            ReparentItems(baseParentItems, childItems);
        }
        else
        {
            // Items didn't move, but they became unselected so they need to be reselected
            for (auto i : childItems)
            {
                i->setSelected(true);
            }
        }

        m_signalSelectionChange = true;

        if (m_selectionChangedBeforeDrag)
        {
            // Signal a selection change on the mouse release
            SignalUserSelectionHasChanged(selectedItems());
        }
    }
    else if (AssetDropHelpers::DoesMimeDataContainSliceOrComponentAssets(ev->mimeData()))
    {
        DropMimeDataAssetsAtHierarchyPosition(ev->mimeData(), ev->pos());

        ev->setDropAction(Qt::CopyAction);
        ev->accept();
        QTreeWidget::dropEvent(ev);

        // Put focus on the hierarchy widget
        activateWindow();
        setFocus();
    }
}

QStringList HierarchyWidget::mimeTypes() const
{
    QStringList list = QTreeWidget::mimeTypes();
    list.append(AzToolsFramework::EditorEntityIdContainer::GetMimeType());
    list.append(AzToolsFramework::AssetBrowser::AssetBrowserEntry::GetMimeType());
    return list;
}

QMimeData* HierarchyWidget::mimeData(const QList<QTreeWidgetItem*> items) const
{
    AzToolsFramework::EditorEntityIdContainer entityIdList;
    for (auto i : items)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(i);
        AZ::EntityId entityId = item->GetEntityId();
        if (entityId.IsValid())
        {
            entityIdList.m_entityIds.push_back(entityId);
        }
    }
    if (entityIdList.m_entityIds.empty())
    {
        return nullptr;
    }

    AZStd::vector<char> encoded;
    if (!entityIdList.ToBuffer(encoded))
    {
        return nullptr;
    }

    QMimeData* mimeDataPtr = new QMimeData();
    QByteArray encodedData;
    encodedData.resize((int)encoded.size());
    memcpy(encodedData.data(), encoded.data(), encoded.size());

    mimeDataPtr->setData(AzToolsFramework::EditorEntityIdContainer::GetMimeType(), encodedData);
    return mimeDataPtr;
}

bool HierarchyWidget::AcceptsMimeData(const QMimeData* mimeData)
{
    if (!mimeData)
    {
        return false;
    }

    if (!m_isInited)
    {
        return false;
    }

    if (mimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
    {
        QByteArray arrayData = mimeData->data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

        AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
        if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
        {
            return false;
        }

        if (entityIdListContainer.m_entityIds.empty())
        {
            return false;
        }

        // Get the entity context that the first dragged entity is attached to
        AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(
            contextId, entityIdListContainer.m_entityIds[0], &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);
        if (contextId.IsNull())
        {
            return false;
        }

        // Check that the entity context is the UI editor entity context
        UiEditorEntityContext* editorEntityContext = m_editorWindow->GetEntityContext();
        if (!editorEntityContext || (editorEntityContext->GetContextId() != contextId))
        {
            return false;
        }

        return true;
    }

    return AssetDropHelpers::DoesMimeDataContainSliceOrComponentAssets(mimeData);
}

void HierarchyWidget::DropMimeDataAssetsAtHierarchyPosition(const QMimeData* mimeData, const QPoint& position)
{
    using namespace AzToolsFramework;

    // Check where the drop indicator is to determine the parent for a new entity
    // or to determine an existing entity for new components
    QTreeWidgetItem* item = itemAt(position);
    DropIndicatorPosition dropPosition = dropIndicatorPosition();

    QTreeWidgetItem* targetWidgetItem = nullptr;
    bool onItem = false;
    int childIndex = -1;
    switch (dropPosition)
    {
    case QAbstractItemView::AboveItem:
        targetWidgetItem = item->parent();
        childIndex = (targetWidgetItem ? targetWidgetItem : invisibleRootItem())->indexOfChild(item);
        break;
    case QAbstractItemView::BelowItem:
        targetWidgetItem = item->parent();
        childIndex = (targetWidgetItem ? targetWidgetItem : invisibleRootItem())->indexOfChild(item) + 1;
        break;
    case QAbstractItemView::OnItem:
        targetWidgetItem = item;
        // Shift modifier enables creating a child entity from the asset
        onItem = !(QApplication::keyboardModifiers() & Qt::ShiftModifier);
        break;
    case QAbstractItemView::OnViewport:
        targetWidgetItem = nullptr;
        break;
    }

    DropMimeDataAssets(mimeData, targetWidgetItem, onItem, childIndex, nullptr);
}

void HierarchyWidget::DropMimeDataAssets(const QMimeData* mimeData,
    QTreeWidgetItem* targetWidgetItem,
    bool onElement,
    int childIndex,
    const QPoint* newElementPosition)
{
    ComponentAssetHelpers::ComponentAssetPairs componentAssetPairs;
    AssetDropHelpers::AssetList sliceAssets;
    AssetDropHelpers::DecodeSliceAndComponentAssetsFromMimeData(mimeData, componentAssetPairs, sliceAssets);

    if (componentAssetPairs.empty() && sliceAssets.empty())
    {
        return;
    }

    // Change current selection so instantiated slices will be parented correctly
    if (targetWidgetItem)
    {
        SetUniqueSelectionHighlight(targetWidgetItem);
    }
    else
    {
        clearSelection();
    }

    // Instantiate dropped slices
    for (const AZ::Data::AssetId& sliceAssetId : sliceAssets)
    {
        // Instantiate slice under currently selected parent
        AZ::Vector2 viewportPosition(-1.0f, -1.0f); // indicates no viewport position specified
        if (newElementPosition)
        {
            viewportPosition = QtHelpers::QPointFToVector2(*newElementPosition);
        }
        GetEditorWindow()->GetSliceManager()->InstantiateSlice(sliceAssetId, viewportPosition, childIndex);
    }

    if (componentAssetPairs.empty())
    {
        return;
    }

    // Add components to the element being hovered or to a newly created element
    if (onElement)
    {
        // Add components to the existing target element which is now the selected element
        AZ_Assert(targetWidgetItem, "Must provide a target item when dropping component assets onto an element");

        // Make a list of the component types to be added
        AZStd::vector<AZ::TypeId> componentTypes;
        componentTypes.reserve(componentAssetPairs.size());
        for (const ComponentAssetHelpers::ComponentAssetPair& pair : componentAssetPairs)
        {
            componentTypes.push_back(pair.first);
        }

        ComponentHelpers::EntityComponentPair firstIncompatibleComponentType = AZStd::make_pair(AZ::EntityId(), AZ::Uuid::CreateNull());
        if (!ComponentHelpers::CanAddComponentsToSelectedEntities(componentTypes, &firstIncompatibleComponentType))
        {
            const AZ::EntityId& entityId = firstIncompatibleComponentType.first;
            const AZ::TypeId& componentTypeId = firstIncompatibleComponentType.second;

            HierarchyItem* targetItem = HierarchyItem::RttiCast(targetWidgetItem);
            AZStd::string entityName(targetItem->GetElement() ? targetItem->GetElement()->GetName() : "<unknown>");

            if (!entityId.IsValid() || componentTypeId.IsNull())
            {
                const AZStd::string message = AZStd::string::format("Failed to add components to target element \"%s\".", entityName.c_str());
                QMessageBox::warning(m_editorWindow, tr("Asset Drop"), QString(message.c_str()));
            }
            else
            {
                AZ::ComponentDescriptor* descriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(descriptor, firstIncompatibleComponentType.second, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                AZStd::string componentName(descriptor ? descriptor->GetName() : "<unknown>");
                const AZStd::string message = AZStd::string::format("Failed to add components to target element \"%s\". Component \"%s\" is not compatible.", entityName.c_str(), componentName.c_str());
                QMessageBox::warning(m_editorWindow, tr("Asset Drop"), QString(message.c_str()));
            }

            return;
        }

        // Batch-add all the components
        ComponentHelpers::AddComponentsWithAssetToSelectedEntities(componentAssetPairs);
    }
    else
    {
        // Create a new element
        QTreeWidgetItemRawPtrQList parentItems;
        if (targetWidgetItem)
        {
            parentItems.append(targetWidgetItem);
        }
        CommandHierarchyItemCreate::Push(m_editorWindow->GetActiveStack(),
            this,
            parentItems,
            childIndex,
            [this, componentAssetPairs, newElementPosition](AZ::Entity* element)
        {
            if (element)
            {
                // Set the element's position
                if (newElementPosition)
                {
                    EntityHelpers::MoveElementToGlobalPosition(element, *newElementPosition);
                }

                // Make a list of the component types to be added
                AZStd::vector<AZ::TypeId> componentTypes;
                componentTypes.reserve(componentAssetPairs.size());
                for (const ComponentAssetHelpers::ComponentAssetPair& pair : componentAssetPairs)
                {
                    componentTypes.push_back(pair.first);
                }

                ComponentHelpers::EntityComponentPair firstIncompatibleComponentType = AZStd::make_pair(AZ::EntityId(), AZ::Uuid::CreateNull());
                if (!ComponentHelpers::CanAddComponentsToEntity(componentTypes, element->GetId(), &firstIncompatibleComponentType))
                {
                    const AZ::TypeId& componentTypeId = firstIncompatibleComponentType.second;
                    if (componentTypeId.IsNull())
                    {
                        QMessageBox::warning(m_editorWindow, tr("Asset Drop"), tr("Failed to add components to new element."));
                    }
                    else
                    {
                        AZ::ComponentDescriptor* descriptor = nullptr;
                        AZ::ComponentDescriptorBus::EventResult(descriptor, firstIncompatibleComponentType.second, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                        AZStd::string componentName(descriptor ? descriptor->GetName() : "<unknown>");
                        const AZStd::string message = AZStd::string::format("Failed to add components to new element. Component \"%s\" is not compatible.", componentName.c_str());
                        QMessageBox::warning(m_editorWindow, tr("Asset Drop"), QString(message.c_str()));
                    }
                    return;
                }

                // Batch-add all the components
                ComponentHelpers::AddComponentsWithAssetToEntity(componentAssetPairs, element->GetId());

                // Name the entity after the first asset
                const ComponentAssetHelpers::ComponentAssetPair& pair = componentAssetPairs.front();
                const AZ::Data::AssetId& assetId = pair.second;
                AZStd::string assetPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetId);
                if (!assetPath.empty())
                {
                    AZStd::string entityName;
                    AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), entityName);

                    // Find a unique name for the new element
                    AZ::EntityId parentEntityId;
                    UiElementBus::EventResult(parentEntityId, element->GetId(), &UiElementBus::Events::GetParentEntityId);

                    AZStd::string uniqueName;
                    UiCanvasBus::EventResult(
                        uniqueName,
                        GetEditorWindow()->GetCanvas(),
                        &UiCanvasBus::Events::GetUniqueChildName,
                        parentEntityId,
                        entityName,
                        nullptr);

                    element->SetName(uniqueName);

                    QTreeWidgetItem* item = HierarchyHelpers::ElementToItem(this, element, false);
                    AZ_Assert(item, "Newly created element doesn't have a hierarchy item");
                    item->setText(0, uniqueName.c_str());
                }
            }
            else
            {
                QMessageBox::warning(m_editorWindow, tr("Asset Drop"), tr("Failed to create a new element to add components to."));
            }
        });
    }
}

void HierarchyWidget::mouseMoveEvent(QMouseEvent* ev)
{
    HierarchyItem* itemBeingHovered = HierarchyItem::RttiCast(itemAt(ev->pos()));
    if (itemBeingHovered)
    {
        // Hovering.

        if (m_itemBeingHovered)
        {
            if (itemBeingHovered == m_itemBeingHovered)
            {
                // Still hovering over the same item.
                // Nothing to do.
            }
            else
            {
                // Hover start over a different item.

                // Hover ends over the previous item.
                m_itemBeingHovered->SetMouseIsHovering(false);

                // Hover starts over the current item.
                m_itemBeingHovered = itemBeingHovered;
                m_itemBeingHovered->SetMouseIsHovering(true);
            }
        }
        else
        {
            // Hover start.
            m_itemBeingHovered = itemBeingHovered;
            m_itemBeingHovered->SetMouseIsHovering(true);
        }
    }
    else
    {
        // Not hovering.

        if (m_itemBeingHovered)
        {
            // Hover end.
            m_itemBeingHovered->SetMouseIsHovering(false);
            m_itemBeingHovered = nullptr;
        }
        else
        {
            // Still not hovering.
            // Nothing to do.
        }
    }

    QTreeWidget::mouseMoveEvent(ev);
}

void HierarchyWidget::mouseReleaseEvent(QMouseEvent* ev)
{
    if (m_selectionChangedBeforeDrag)
    {
        // Signal a selection change on the mouse release
        SignalUserSelectionHasChanged(selectedItems());
    }

    QTreeWidget::mouseReleaseEvent(ev);

    // In pick mode, the user can click on an item and drag the mouse to change the current item.
    // In this case, a click event is not sent on a mouse release, so set the current item as the
    // picked item here
    if (m_inObjectPickMode)
    {
        // If there is a current item, set that as picked
        if (currentIndex() != QModelIndex()) // check for a valid index
        {
            QTreeWidgetItem* item = itemFromIndex(currentIndex());
            if (item)
            {
                PickItem(HierarchyItem::RttiCast(item));
            }
        }
    }
}

void HierarchyWidget::leaveEvent(QEvent* ev)
{
    ClearItemBeingHovered();

    // If an item is dragged really fast outside the hierarchy, the startDrag event is called,
    // but the dragEnterEvent and dragLeaveEvent are replaced with the leaveEvent.
    // In this case, perform the dragLeaveEvent here
    if (m_inDragStartState)
    {
        if (m_selectionChangedBeforeDrag)
        {
            m_signalSelectionChange = false;

            // Set the current selection to the items that were selected before the drag
            clearSelection();
            for (auto i : m_beforeDragSelection)
            {
                i->setSelected(true);
            }

            m_signalSelectionChange = true;
        }

        m_inDragStartState = false;
    }

    QTreeWidget::leaveEvent(ev);
}

void HierarchyWidget::ClearItemBeingHovered()
{
    if (!m_itemBeingHovered)
    {
        // Nothing to do.
        return;
    }

    m_itemBeingHovered->SetMouseIsHovering(false);
    m_itemBeingHovered = nullptr;
}

void HierarchyWidget::UpdateSliceInfo()
{
    // Update the slice information (color, font, tooltip) for all elements.
    // As a simple way of going through all the HierarchyItem's we use the
    // EntityHelpers::EntityToHierarchyItemMap
    for (auto mapItem : m_entityItemMap)
    {
        mapItem.second->UpdateSliceInfo();
    }
}

void HierarchyWidget::DropMimeDataAssets(const QMimeData* mimeData,
    const AZ::EntityId& targetEntityId,
    bool onElement,
    int childIndex,
    const QPoint* newElementPosition)
{
    if (!m_isInited)
    {
        return;
    }

    QTreeWidgetItem* targetWidgetItem = targetEntityId.IsValid() ? HierarchyHelpers::ElementToItem(this, targetEntityId, false) : nullptr;
    DropMimeDataAssets(mimeData, targetWidgetItem, onElement, childIndex, newElementPosition);
}

void HierarchyWidget::DeleteSelectedItems()
{
    DeleteSelectedItems(selectedItems());
}

void HierarchyWidget::OnEntityPickModeStarted()
{
    setDragEnabled(false);
    m_currentItemBeforePickMode = currentIndex();
    m_selectionModeBeforePickMode = selectionMode();
    setSelectionMode(QAbstractItemView::NoSelection);
    m_editTriggersBeforePickMode = editTriggers();
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setCursor(m_editorWindow->GetEntityPickerCursor());
    m_inObjectPickMode = true;
}

void HierarchyWidget::OnEntityPickModeStopped()
{
    if (m_inObjectPickMode)
    {
        setCurrentIndex(m_currentItemBeforePickMode);
        setDragEnabled(true);
        setSelectionMode(m_selectionModeBeforePickMode);
        setEditTriggers(m_editTriggersBeforePickMode);
        setCursor(Qt::ArrowCursor);
        m_inObjectPickMode = false;
    }
}

void HierarchyWidget::EntityHighlightRequested([[maybe_unused]] AZ::EntityId entityId)
{
}

void HierarchyWidget::EntityStrongHighlightRequested(AZ::EntityId entityId)
{
    // Check if this entity is in the same entity context
    if (!IsEntityInEntityContext(entityId))
    {
        return;
    }

    QTreeWidgetItem* item = HierarchyHelpers::ElementToItem(this, entityId, false);
    if (!item)
    {
        return;
    }

    // Scrolling to the entity will make sure that it is visible.
    // This will automatically open parents
    scrollToItem(item);

    // Select the entity
    SetUniqueSelectionHighlight(item);
}

void HierarchyWidget::PickItem(HierarchyItem* item)
{
    const AZ::EntityId entityId = item->GetEntityId();
    if (entityId.IsValid())
    {
        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::PickModeSelectEntity, entityId);

        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);
    }
}

bool HierarchyWidget::IsEntityInEntityContext(AZ::EntityId entityId)
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    AzFramework::EntityIdContextQueryBus::EventResult(
        contextId, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

    if (!contextId.IsNull())
    {
        UiEditorEntityContext* editorEntityContext = m_editorWindow->GetEntityContext();
        if (editorEntityContext && editorEntityContext->GetContextId() == contextId)
        {
            return true;
        }
    }

    return false;
}

void HierarchyWidget::ReparentItems(const QTreeWidgetItemRawPtrList& baseParentItems,
    const HierarchyItemRawPtrList& childItems)
{
    CommandHierarchyItemReparent::Push(m_editorWindow->GetActiveStack(),
        this,
        childItems,
        baseParentItems);
}

void HierarchyWidget::ToggleVisibility(HierarchyItem* hierarchyItem)
{
    bool isItemVisible = true;
    AZ::EntityId itemEntityId = hierarchyItem->GetEntityId();
    UiEditorBus::EventResult(isItemVisible, itemEntityId, &UiEditorBus::Events::GetIsVisible);

    // There is one exception to toggling the visibility. If the clicked item has invisible ancestors,
    // then we make that item and all its ancestors visible regardless of the item's visibility

    // Make a list of items to modify
    HierarchyItemRawPtrList items;

    // Look for invisible ancestors
    AZ::EntityId parent;
    UiElementBus::EventResult(parent, itemEntityId, &UiElementBus::Events::GetParentEntityId);
    while (parent.IsValid())
    {
        bool isParentVisible = true;
        UiEditorBus::EventResult(isParentVisible, parent, &UiEditorBus::Events::GetIsVisible);

        if (!isParentVisible)
        {
            items.push_back(m_entityItemMap[parent]);
        }

        AZ::EntityId newParent = parent;
        parent.SetInvalid();
        UiElementBus::EventResult(parent, newParent, &UiElementBus::Events::GetParentEntityId);
    }

    bool makeVisible = items.size() > 0 ? true : !isItemVisible;

    // Add the item that was clicked
    if (makeVisible != isItemVisible)
    {
        items.push_back(m_entityItemMap[itemEntityId]);
    }

    CommandHierarchyItemToggleIsVisible::Push(m_editorWindow->GetActiveStack(),
        this,
        items);
}

void HierarchyWidget::DeleteSelectedItems(const QTreeWidgetItemRawPtrQList& selectedItems)
{
    CommandHierarchyItemDelete::Push(m_editorWindow->GetActiveStack(),
        this,
        selectedItems);

    // This ensures there's no "current item".
    SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);

    // IMPORTANT: This is necessary to indirectly trigger detach()
    // in the PropertiesWidget.
    SetUserSelection(nullptr);
}

void HierarchyWidget::Cut()
{
    QTreeWidgetItemRawPtrQList selection = selectedItems();

    HierarchyClipboard::CopySelectedItemsToClipboard(this,
        selection);
    DeleteSelectedItems(selection);
}

void HierarchyWidget::Copy()
{
    HierarchyClipboard::CopySelectedItemsToClipboard(this,
        selectedItems());
}

void HierarchyWidget::PasteAsSibling()
{
    HierarchyClipboard::CreateElementsFromClipboard(this,
        selectedItems(),
        false);
}

void HierarchyWidget::PasteAsChild()
{
    HierarchyClipboard::CreateElementsFromClipboard(this,
        selectedItems(),
        true);
}

void HierarchyWidget::SetEditorOnlyForSelectedItems(bool editorOnly)
{
    QTreeWidgetItemRawPtrQList selection = selectedItems();
    if (!selection.empty())
    {
        SerializeHelpers::SerializedEntryList preChangeState;
        HierarchyClipboard::BeginUndoableEntitiesChange(m_editorWindow, preChangeState);

        for (auto item : selection)
        {
            HierarchyItem* i = HierarchyItem::RttiCast(item);

            AzToolsFramework::EditorOnlyEntityComponentRequestBus::Event(i->GetEntityId(), &AzToolsFramework::EditorOnlyEntityComponentRequests::SetIsEditorOnlyEntity, editorOnly);

            i->UpdateEditorOnlyInfo();
        }

        HierarchyClipboard::EndUndoableEntitiesChange(m_editorWindow, "editor only selection", preChangeState);

        emit editorOnlyStateChangedOnSelectedElements();
    }
}

void HierarchyWidget::AddElement(const QTreeWidgetItemRawPtrQList& selectedItems, const QPoint* optionalPos)
{
    const int childIndex = -1;
    CommandHierarchyItemCreate::Push(m_editorWindow->GetActiveStack(),
        this,
        selectedItems,
        childIndex,
        [this, optionalPos](AZ::Entity* element)
        {
            if (optionalPos)
            {
                // Convert position to render viewport coords
                QPoint scaledPosition = *optionalPos * GetEditorWindow()->GetViewport()->WidgetToViewportFactor();
                EntityHelpers::MoveElementToGlobalPosition(element, scaledPosition);
            }
        });
}

void HierarchyWidget::SetUniqueSelectionHighlight(QTreeWidgetItem* item)
{
    // Stop object pick mode when an action explicitly wants to set the hierarchy's selected items
    AzToolsFramework::EditorPickModeRequestBus::Broadcast(
        &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);

    clearSelection();

    setCurrentIndex(indexFromItem(item));
}

void HierarchyWidget::SetUniqueSelectionHighlight(const AZ::Entity* element)
{
    SetUniqueSelectionHighlight(HierarchyHelpers::ElementToItem(this, element, false));
}

#include <moc_HierarchyWidget.cpp>
