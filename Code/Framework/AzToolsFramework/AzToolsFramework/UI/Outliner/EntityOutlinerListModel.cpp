/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityOutlinerListModel.hxx"

#include <QApplication>
#include <QBitmap>
#include <QCheckBox>
#include <QEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QTimer>
#include <QTextDocument>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Outcome/Outcome.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserSourceDropBus.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerDisplayOptionsMenu.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerSortFilterProxyModel.hxx>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerTreeView.hxx>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerCacheBus.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

////////////////////////////////////////////////////////////////////////////
// EntityOutlinerListModel
////////////////////////////////////////////////////////////////////////////

namespace AzToolsFramework
{

    bool EntityOutlinerListModel::s_paintingName = false;

    EntityOutlinerListModel::EntityOutlinerListModel(QObject* parent)
        : QAbstractItemModel(parent)
        , m_entitySelectQueue()
        , m_entityExpandQueue()
        , m_entityChangeQueue()
        , m_entityChangeQueued(false)
        , m_entityLayoutQueued(false)
        , m_entityExpansionState()
        , m_entityFilteredState()
    {
        m_focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        AZ_Assert(m_focusModeInterface != nullptr, "EntityOutlinerListModel requires a FocusModeInterface instance on construction.");
    }

    EntityOutlinerListModel::~EntityOutlinerListModel()
    {
        ContainerEntityNotificationBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        EntityCompositionNotificationBus::Handler::BusDisconnect();
        AZ::EntitySystemBus::Handler::BusDisconnect();
        EditorEntityRuntimeActivationChangeNotificationBus::Handler::BusDisconnect();
    }

    void EntityOutlinerListModel::Initialize()
    {
        EditorEntityRuntimeActivationChangeNotificationBus::Handler::BusConnect();
        ToolsApplicationEvents::Bus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        EditorEntityInfoNotificationBus::Handler::BusConnect();
        EntityCompositionNotificationBus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

        ContainerEntityNotificationBus::Handler::BusConnect(editorEntityContextId);

        m_editorEntityUiInterface = AZ::Interface<AzToolsFramework::EditorEntityUiInterface>::Get();
        AZ_Assert(m_editorEntityUiInterface != nullptr,
            "EntityOutlinerListModel requires a EditorEntityUiInterface instance on Initialize.");
    }

    int EntityOutlinerListModel::rowCount(const QModelIndex& parent) const
    {
        // For QTreeView models, non-0 columns shouldn't have children
        if (parent.isValid() && parent.column() != 0)
        {
            return 0;
        }

        auto parentId = GetEntityFromIndex(parent);

        AZStd::size_t childCount = 0;
        EditorEntityInfoRequestBus::EventResult(childCount, parentId, &EditorEntityInfoRequestBus::Events::GetChildCount);
        return (int)childCount;
    }

    int EntityOutlinerListModel::columnCount(const QModelIndex& /*parent*/) const
    {
        return ColumnCount;
    }

    QModelIndex EntityOutlinerListModel::index(int row, int column, const QModelIndex& parent) const
    {
        auto parentId = GetEntityFromIndex(parent);

        // We have the row and column, so we just need the child ID to construct our index
        AZ::EntityId childId;
        EditorEntityInfoRequestBus::EventResult(childId, parentId, &EditorEntityInfoRequestBus::Events::GetChild, row);
        AZ_Assert(childId.IsValid(), "No child found for parent");
        return createIndex(row, column, static_cast<AZ::u64>(childId));
    }

    QVariant EntityOutlinerListModel::data(const QModelIndex& index, int role) const
    {
        auto id = GetEntityFromIndex(index);
        if (id.IsValid())
        {
            switch (index.column())
            {
            case ColumnName:
                return dataForName(index, role);

            case ColumnVisibilityToggle:
                return dataForVisibility(index, role);

            case ColumnLockToggle:
                return dataForLock(index, role);

            case ColumnSortIndex:
                return dataForSortIndex(index, role);
            }
        }

        return QVariant();
    }

    QMap<int, QVariant> EntityOutlinerListModel::itemData(const QModelIndex &index) const
    {
        QMap<int, QVariant> roles = QAbstractItemModel::itemData(index);
        for (int i = Qt::UserRole; i < RoleCount; ++i)
        {
            QVariant variantData = data(index, i);
            if (variantData.isValid())
            {
                roles.insert(i, variantData);
            }
        }

        return roles;
    }

    QVariant EntityOutlinerListModel::dataForAll(const QModelIndex& index, int role) const
    {
        auto id = GetEntityFromIndex(index);

        switch (role)
        {
        case EntityIdRole:
            return static_cast<AZ::u64>(id);

        case SelectedRole:
            {
                bool isSelected = false;
                EditorEntityInfoRequestBus::EventResult(isSelected, id, &EditorEntityInfoRequestBus::Events::IsSelected);
                return isSelected;
            }

        case ChildSelectedRole:
            return HasSelectedDescendant(id);

        case PartiallyVisibleRole:
            return !AreAllDescendantsSameVisibleState(id);

        case PartiallyLockedRole:
            return !AreAllDescendantsSameLockState(id);

        case LockedAncestorRole:
            return IsInLayerWithProperty(id, LayerProperty::Locked);

        case InvisibleAncestorRole:
            return IsInLayerWithProperty(id, LayerProperty::Invisible);

        case ChildCountRole:
            {
                AZStd::size_t childCount = 0;
                EditorEntityInfoRequestBus::EventResult(childCount, id, &EditorEntityInfoRequestBus::Events::GetChildCount);
                return (int)childCount;
            }

        case ExpandedRole:
            return IsExpanded(id);

        case VisibilityRole:
            return !IsFiltered(id);

        }

        return QVariant();
    }

    QVariant EntityOutlinerListModel::dataForName(const QModelIndex& index, int role) const
    {
        auto id = GetEntityFromIndex(index);

        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            {
                AZStd::string name;
                EditorEntityInfoRequestBus::EventResult(name, id, &EditorEntityInfoRequestBus::Events::GetName);
                QString label{ name.data() };
                if (s_paintingName && !m_filterString.empty())
                {
                    // highlight characters in filter
                    int highlightTextIndex = 0;
                    do
                    {
                        highlightTextIndex = label.lastIndexOf(QString(m_filterString.c_str()), highlightTextIndex - 1, Qt::CaseInsensitive);
                        if (highlightTextIndex >= 0)
                        {
                            const QString BACKGROUND_COLOR{ "#707070" };
                            label.insert(highlightTextIndex + static_cast<int>(m_filterString.length()), "</span>");
                            label.insert(highlightTextIndex, "<span style=\"background-color: " + BACKGROUND_COLOR + "\">");
                        }
                    } while(highlightTextIndex > 0);
                }
                return label;
            }
            break;
        case Qt::ToolTipRole:
            {
                return GetEntityTooltip(id);
            }
            break;
        case Qt::ForegroundRole:
            {
                // We use the parent's palette because the GUI Application palette is returning the wrong colors
                auto parentWidgetPtr = static_cast<QWidget*>(QObject::parent());
                return QBrush(parentWidgetPtr->palette().color(QPalette::Text));
            }
            break;
        case Qt::DecorationRole:
            {
                return GetEntityIcon(id);
            }
            break;
        }

        return dataForAll(index, role);
    }

    QVariant EntityOutlinerListModel::GetEntityIcon(const AZ::EntityId& id) const
    {
        auto entityUiHandler = m_editorEntityUiInterface->GetHandler(id);
        QIcon icon;

        // Retrieve the icon from the handler
        if (entityUiHandler != nullptr)
        {
            icon = entityUiHandler->GenerateItemIcon(id);
        }

        if (!icon.isNull())
        {
            return icon;
        }

        // If no icon was returned by the handler, use the default one.
        bool isEditorOnly = false;
        EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, id, &EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

        if (isEditorOnly)
        {
            return QIcon(QString(":/Entity/entity_editoronly.svg"));
        }

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        const bool isInitiallyActive = entity ? entity->IsRuntimeActiveByDefault() : true;

        if (!isInitiallyActive)
        {
            return QIcon(QString(":/Entity/entity_notactive.svg"));
        }

        return QIcon(QString(":/Entity/entity.svg"));
    }

    QVariant EntityOutlinerListModel::GetEntityTooltip(const AZ::EntityId& id) const
    {
        auto entityUiHandler = m_editorEntityUiInterface->GetHandler(id);
        QString tooltip;

        // Retrieve the tooltip from the handler
        if (entityUiHandler != nullptr)
        {
            tooltip = entityUiHandler->GenerateItemTooltip(id);
        }

        // Add Entity Status information to tooltip
        bool isEditorOnly = false;
        EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, id, &EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

        if (isEditorOnly)
        {
            tooltip = QString("(%1) %2").arg(tr("Editor-only"), tooltip);
        }
        else
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
            if (!entity->IsRuntimeActiveByDefault())
            {
                tooltip = QString("(%1) %2").arg(tr("Not Active on Start"), tooltip);
            }
        }

        return tooltip;
    }

    QVariant EntityOutlinerListModel::dataForVisibility(const QModelIndex& index, int role) const
    {
        auto entityId = GetEntityFromIndex(index);
        auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId);

        if (!entityUiHandler || entityUiHandler->CanToggleLockVisibility(entityId))
        {
            switch (role)
            {
            case Qt::CheckStateRole:
                {
                    return IsEntitySetToBeVisible(entityId) ? Qt::Checked : Qt::Unchecked;
                }
            case Qt::ToolTipRole:
                {
                    return QString("Show/Hide Entity");
                }
            case Qt::SizeHintRole:
                {
                    return QSize(20, 20);
                }
            }
            return dataForAll(index, role);
        }

        return QVariant();
    }

    QVariant EntityOutlinerListModel::dataForLock(const QModelIndex& index, int role) const
    {
        auto entityId = GetEntityFromIndex(index);
        auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId);

        if (!entityUiHandler || entityUiHandler->CanToggleLockVisibility(entityId))
        {
            switch (role)
            {
            case Qt::CheckStateRole:
                {
                    bool isLocked = false;
                    // Lock state is tracked in 3 places:
                    // EditorLockComponent, EditorEntityModel, and ComponentEntityObject.
                    // In addition to that, entities that are in layers can have the layer's lock state override their own.
                    // Retrieving the lock state from the lock component is ideal for drawing the lock icon in the outliner because
                    // the outliner needs to show that specific entity's lock state, and not the actual final lock state including the layer
                    // behavior. The EditorLockComponent only knows about the specific entity's lock state and not the hierarchy.
                    EditorLockComponentRequestBus::EventResult(isLocked, entityId, &EditorLockComponentRequests::GetLocked);

                    return isLocked ? Qt::Checked : Qt::Unchecked;
                }
            case Qt::ToolTipRole:
                {
                    return QString("Lock/Unlock Entity (Locked means the entity cannot be moved in the viewport)");
                }
            case Qt::SizeHintRole:
                {
                    return QSize(20, 20);
                }
            }

            return dataForAll(index, role);
        }

        return QVariant();
    }

    QVariant EntityOutlinerListModel::dataForSortIndex(const QModelIndex& index, int role) const
    {
        auto id = GetEntityFromIndex(index);

        switch (role)
        {
        case Qt::DisplayRole:
            AZ::u64 sortIndex = 0;
            EditorEntityInfoRequestBus::EventResult(sortIndex, id, &EditorEntityInfoRequestBus::Events::GetIndexForSorting);
            return sortIndex;
        }

        return dataForAll(index, role);
    }

    bool EntityOutlinerListModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        switch (role)
        {
        case Qt::CheckStateRole:
        {
            if (value.canConvert<Qt::CheckState>())
            {
                const auto entityId = GetEntityFromIndex(index);
                auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId);

                if (!entityUiHandler || entityUiHandler->CanToggleLockVisibility(entityId))
                {
                    switch (index.column())
                    {
                    case ColumnVisibilityToggle:
                        ToggleEntityVisibility(entityId);
                        break;

                    case ColumnLockToggle:
                        ToggleEntityLockState(entityId);
                        break;
                    }
                }
            }
        }
        break;

        case Qt::EditRole:
        {
            if (index.column() == ColumnName && !value.toString().isEmpty())
            {
                auto id = GetEntityFromIndex(index);
                if (id.IsValid())
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, id);

                    if (entity)
                    {
                        AZStd::string oldName = entity->GetName();
                        AZStd::string newName = value.toString().toUtf8().constData();

                        if (oldName != newName)
                        {
                            ScopedUndoBatch undo("Rename Entity");

                            entity->SetName(newName);
                            undo.MarkEntityDirty(entity->GetId());

                            EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_EntireTree);
                        }
                    }
                    else
                    {
                        AZStd::string name;
                        EditorEntityInfoRequestBus::EventResult(name, id, &EditorEntityInfoRequestBus::Events::GetName);
                        AZ_Assert(entity, "Outliner entry cannot locate entity with name: %s", name.c_str());
                    }
                }
            }
        }
        break;
        }

        return QAbstractItemModel::setData(index, value, role);
    }

    QModelIndex EntityOutlinerListModel::parent(const QModelIndex& index) const
    {
        // invalid indices have no parent
        if (!index.isValid())
        {
            return QModelIndex();
        }

        auto id = GetEntityFromIndex(index);
        if (id.IsValid())
        {
            AZ::EntityId parentId;
            EditorEntityInfoRequestBus::EventResult(parentId, id, &EditorEntityInfoRequestBus::Events::GetParent);
            return GetIndexFromEntity(parentId, 0);
        }
        return QModelIndex();
    }

    Qt::ItemFlags EntityOutlinerListModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::ItemIsDropEnabled;
        }

        Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);
        switch (index.column())
        {
        case ColumnVisibilityToggle:
        case ColumnLockToggle:
            itemFlags |= Qt::ItemIsEnabled;
            break;

        case ColumnName:
            itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
            break;

        default:
            itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
            break;
        }

        if (AZ::EntityId entityId = GetEntityFromIndex(index); !m_focusModeInterface->IsInFocusSubTree(entityId))
        {
            itemFlags &= !Qt::ItemIsEnabled;
        }

        return itemFlags;
    }

    Qt::DropActions EntityOutlinerListModel::supportedDropActions() const
    {
        return Qt::CopyAction;
    }

    Qt::DropActions EntityOutlinerListModel::supportedDragActions() const
    {
        return Qt::CopyAction;
    }


    bool EntityOutlinerListModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
    {
        if (data)
        {
            if (canDropMimeDataForEntityIds(data, action, row, column, parent))
            {
                return true;
            }

            if (CanDropMimeDataAssets(data, action, row, column, parent))
            {
                return true;
            }
        }

        return false;
    }

    bool EntityOutlinerListModel::canDropMimeDataForEntityIds(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
    {
        (void)action;
        (void)column;
        (void)row;

        if (!data || !data->hasFormat(EditorEntityIdContainer::GetMimeType()) || column > 0)
        {
            return false;
        }

        QByteArray arrayData = data->data(EditorEntityIdContainer::GetMimeType());

        EditorEntityIdContainer entityIdListContainer;
        if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
        {
            return false;
        }

        AZ::EntityId newParentId = GetEntityFromIndex(parent);
        if (!CanReparentEntities(newParentId, entityIdListContainer.m_entityIds))
        {
            return false;
        }

        return true;
    }

    // Expected behavior when dropping an entity in the outliner view model:
    //  DragEntity onto DropEntity, neither in each other's hierarchy:
    //      DropEntity becomes parent of DragEntity
    //  DragEntity onto DropEntity, DropEntity is DragEntity's parent:
    //      DragEntity sets its parent to DropEntity's parent.
    //  DragEntity onto DropEntity, DragEntity is DropEntity's parent:
    //      No change occurs.
    //  DragEntity onto DropEntity, DropEntity is DragEntity's grandfather (or deeper):
    //      DragEntity's parent becomes DropEntity.
    //  DragEntity onto DropEntity, DragEntity is DropEntity's grandfather (or deeper):
    //      No change occurs.
    //  DragEntity onto DragEntity
    //      No change occurs.
    //  DragEntity and DragEntityChild onto DropEntity:
    //      DragEntity behaves as define previously.
    //      DragEntityChild behaves as a second DragEntity, following normal drag rules.
    //      Example: DragEntity and DragEntityChild dropped onto DragEntity:
    //          DragEntity has no change occur.
    //          DragEntityChild follows the rule "DragEntity onto DropEntity, DropEntity is DragEntity's parent"
    bool EntityOutlinerListModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
    {
        if (action == Qt::IgnoreAction)
        {
            return true;
        }

        if (parent.isValid() && row == -1 && column == -1)
        {
            // Handle drop from the component palette window.
            if (data->hasFormat(ComponentTypeMimeData::GetMimeType()))
            {
                return dropMimeDataComponentPalette(data, action, row, column, parent);
            }
        }

        if (data->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            return DropMimeDataAssets(data, action, row, column, parent);
        }

        if (data->hasFormat(EditorEntityIdContainer::GetMimeType()))
        {
            return dropMimeDataEntities(data, action, row, column, parent);
        }

        return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
    }

    bool EntityOutlinerListModel::dropMimeDataComponentPalette(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
    {
        (void)action;
        (void)row;
        (void)column;

        AZ::EntityId dropTargetId = GetEntityFromIndex(parent);
        EntityIdList entityIds{ dropTargetId };

        ComponentTypeMimeData::ClassDataContainer classDataContainer;
        ComponentTypeMimeData::Get(data, classDataContainer);

        AZ::ComponentTypeList componentsToAdd;
        for (const auto& classData : classDataContainer)
        {
            if (classData)
            {
                componentsToAdd.push_back(classData->m_typeId);
            }
        }

        EntityCompositionRequests::AddComponentsOutcome addedComponentsResult = AZ::Failure(AZStd::string("Failed to call AddComponentsToEntities on EntityCompositionRequestBus"));
        EntityCompositionRequestBus::BroadcastResult(addedComponentsResult, &EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

        if (addedComponentsResult.IsSuccess())
        {
            ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::InvalidatePropertyDisplay, Refresh_EntireTree_NewContent);
        }

        return false;
    }
    
    bool EntityOutlinerListModel::DecodeAssetMimeData(const QMimeData* data, AZStd::optional<ComponentAssetPairs*> componentAssetPairs,
        AZStd::optional<AZStd::vector<AZStd::string>*> sourceFiles) const
    {
        bool canHandleData = false;
        AZStd::vector<AssetBrowser::AssetBrowserEntry*> entries;
        AssetBrowser::AssetBrowserEntry::FromMimeData(data, entries);

        AZStd::vector<const AssetBrowser::ProductAssetBrowserEntry*> products;

        // Go through all entries and determine if they have products or are source-only.
        for (AssetBrowser::AssetBrowserEntry* entry : entries)
        {
            const AssetBrowser::ProductAssetBrowserEntry* productEntry = azrtti_cast<const AssetBrowser::ProductAssetBrowserEntry*>(entry);

            if (productEntry)
            {
                products.push_back(productEntry);
            }
            else
            {
                // If it's a source entry AND that extension is handled, use it directly
                const AssetBrowser::SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const AssetBrowser::SourceAssetBrowserEntry*>(entry);

                if (sourceEntry && AssetBrowser::AssetBrowserSourceDropBus::HasHandlers(sourceEntry->GetExtension()))
                {
                    if (sourceFiles.has_value())
                    {
                        sourceFiles.value()->push_back(sourceEntry->GetFullPath());
                    }
                    canHandleData = true;
                }
                else
                {
                    // If it's not a product entry, but it has product children, just use those instead.
                    AZStd::vector<const AssetBrowser::ProductAssetBrowserEntry*> children;
                    entry->GetChildren<AssetBrowser::ProductAssetBrowserEntry>(children);

                    for (auto child : children)
                    {
                        products.push_back(child);
                    }
                }
            }
        }

        // Handle all Product assets.
        for (const auto* product : products)
        {
            // Determine if the product type has an associated component.
            // If so, store the componentType->assetId pair.
            bool canCreateComponent = false;
            AZ::AssetTypeInfoBus::EventResult(canCreateComponent, product->GetAssetType(), &AZ::AssetTypeInfo::CanCreateComponent, product->GetAssetId());

            AZ::TypeId componentType;
            AZ::AssetTypeInfoBus::EventResult(componentType, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);

            if (canCreateComponent && !componentType.IsNull())
            {
                if (componentAssetPairs.has_value())
                {
                    componentAssetPairs.value()->push_back(AZStd::make_pair(componentType, product->GetAssetId()));
                }
                canHandleData = true;
            }
        }

        return canHandleData;
    }

    bool EntityOutlinerListModel::CanDropMimeDataAssets(
        const QMimeData* data,
        [[maybe_unused]] Qt::DropAction action,
        [[maybe_unused]] int row,
        [[maybe_unused]] int column,
        const QModelIndex& parent) const
    {
        // Disable dropping assets on closed container entities.
        AZ::EntityId parentId = GetEntityFromIndex(parent);
        if (auto containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
            !containerEntityInterface->IsContainerOpen(parentId))
        {
            return false;
        }

        if (data->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            return DecodeAssetMimeData(data);
        }

        return false;
    }

    bool EntityOutlinerListModel::DropMimeDataAssets(const QMimeData* data, [[maybe_unused]] Qt::DropAction action, int row, [[maybe_unused]] int column, const QModelIndex& parent)
    {
        ComponentAssetPairs componentAssetPairs;
        AZStd::vector<AZStd::string> sourceFiles;
        AZ::EntityId assignParentId = GetEntityFromIndex(parent);

        bool hasValidAssets = DecodeAssetMimeData(data, &componentAssetPairs, &sourceFiles);
        if (!hasValidAssets)
        {
            return false;
        }

        // If the parent entity is a closed container, bail.
        if (auto containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
            !containerEntityInterface->IsContainerOpen(assignParentId))
        {
            return false;
        }

        // Source Files
        if (!sourceFiles.empty())
        {
            // Get position (center of viewport). If no viewport is available, (0,0,0) will be used.
            AZ::Vector3 viewportCenterPosition = AZ::Vector3::CreateZero();
            EditorRequestBus::BroadcastResult(viewportCenterPosition, &EditorRequestBus::Events::GetWorldPositionAtViewportCenter);

            for (AZStd::string& sourceFile : sourceFiles)
            {
                AZStd::string extension;
                AZ::StringFunc::Path::GetExtension(sourceFile.c_str(), extension);

                AssetBrowser::AssetBrowserSourceDropBus::Event(
                    extension, &AssetBrowser::AssetBrowserSourceDropEvents::HandleSourceFileType, sourceFile, assignParentId, viewportCenterPosition
                );
            }
        }

        // Product Assets
        if (componentAssetPairs.size() > 0)
        {
            ScopedUndoBatch undo("Create/Modify Entities for Asset Drop");

            AZ::Vector3 viewportCenter = AZ::Vector3::CreateZero();
            EditorRequestBus::BroadcastResult(viewportCenter, &EditorRequestBus::Events::GetWorldPositionAtViewportCenter);

            // Only resolve an existing if the drop was directly on it. Otherwise we'll create a new entity.
            AZ::EntityId targetEntityId = (row < 0) ? GetEntityFromIndex(parent) : AZ::EntityId();
            bool createdNewEntity = false;

            // Shift modifier enables creating a child entity from the asset.
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
            {
                assignParentId = targetEntityId;
                targetEntityId.SetInvalid();
            }

            if (!targetEntityId.IsValid())
            {
                // Only set the entity instantiation position if a new entity will be created. Otherwise, the next entity to be created will
                // be given this position.
                ToolsApplicationNotificationBus::Broadcast(
                    &ToolsApplicationNotificationBus::Events::SetEntityInstantiationPosition, assignParentId,
                    GetEntityFromIndex(index(row, 0, parent)));

                EditorRequests::Bus::BroadcastResult(targetEntityId, &EditorRequests::CreateNewEntity, assignParentId);
                if (!targetEntityId.IsValid())
                {
                    // Clear the entity instantiation position because this entity failed to be created.
                    // Otherwise, the next entity to be created will be given the wrong parent in the outliner.
                    ToolsApplicationNotificationBus::Broadcast(&ToolsApplicationNotificationBus::Events::ClearEntityInstantiationPosition);
                    QMessageBox::warning(
                        AzToolsFramework::GetActiveWindow(), tr("Asset Drop Failed"), tr("A new entity could not be created for the specified asset."));
                    return false;
                }

                createdNewEntity = true;
            }

            AZ::Entity* targetEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(targetEntity, &AZ::ComponentApplicationBus::Events::FindEntity, targetEntityId);
            if (!targetEntity)
            {
                QMessageBox::warning(AzToolsFramework::GetActiveWindow(), tr("Asset Drop Failed"), tr("Failed to locate target entity."));
                return false;
            }

            // Batch-add all the components.
            AZ::ComponentTypeList componentsToAdd;
            componentsToAdd.reserve(componentAssetPairs.size());
            for (const ComponentAssetPair& pair : componentAssetPairs)
            {
                const AZ::TypeId& componentType = pair.first;

                componentsToAdd.push_back(componentType);
            }

            AZStd::vector<AZ::EntityId> entityIds = {targetEntityId};
            EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string());
            EntityCompositionRequestBus::BroadcastResult(
                addComponentsOutcome, &EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

            if (!addComponentsOutcome.IsSuccess())
            {
                QMessageBox::warning(
                    AzToolsFramework::GetActiveWindow(), tr("Asset Drop Failed"),
                    QStringLiteral("Components could not be added to the target entity \"%1\".\n\nDetails:\n%2.")
                        .arg(targetEntity->GetName().c_str())
                        .arg(addComponentsOutcome.GetError().c_str()));

                if (createdNewEntity)
                {
                    EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::DestroyEditorEntity, targetEntityId);
                }

                return false;
            }

            // Assign asset associated with each created component.
            for (const ComponentAssetPair& pair : componentAssetPairs)
            {
                const AZ::TypeId& componentType = pair.first;
                const AZ::Data::AssetId& assetId = pair.second;

                // Name the entity after the first asset.
                if (createdNewEntity && &pair == &componentAssetPairs.front())
                {
                    AZStd::string assetPath;
                    EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);
                    if (!assetPath.empty())
                    {
                        AZStd::string entityName;
                        AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), entityName);
                        targetEntity->SetName(entityName);
                    }
                }

                AZ::Component* componentAdded = targetEntity->FindComponent(componentType);
                if (componentAdded)
                {
                    Components::EditorComponentBase* editorComponent = GetEditorComponent(componentAdded);
                    if (editorComponent)
                    {
                        editorComponent->SetPrimaryAsset(assetId);
                    }
                }
            }

            if (createdNewEntity)
            {
                const EntityIdList selection = {targetEntityId};
                ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selection);
            }

            if (IsSelected(targetEntityId))
            {
                ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::InvalidatePropertyDisplay, Refresh_EntireTree_NewContent);
            }
        }

        return true;
    }

    bool EntityOutlinerListModel::dropMimeDataEntities(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
    {
        (void)action;
        (void)column;

        QByteArray arrayData = data->data(EditorEntityIdContainer::GetMimeType());

        EditorEntityIdContainer entityIdListContainer;
        if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
        {
            return false;
        }
        
        const int count = rowCount(parent);
        AZ::EntityId newParentId = GetEntityFromIndex(parent);
        AZ::EntityId beforeEntityId = (row >= 0 && row < count) ? GetEntityFromIndex(index(row, 0, parent)) : AZ::EntityId();
        EntityIdList topLevelEntityIds;
        topLevelEntityIds.reserve(entityIdListContainer.m_entityIds.size());
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::FindTopLevelEntityIdsInactive, entityIdListContainer.m_entityIds, topLevelEntityIds);
        const auto appendActionForInvalid = newParentId.IsValid() && (row >= count) ? AppendEnd : AppendBeginning;
        if (!ReparentEntities(newParentId, topLevelEntityIds, beforeEntityId, appendActionForInvalid))
        {
            return false;
        }

        return true;
    }

    bool EntityOutlinerListModel::CanReparentEntities(const AZ::EntityId& newParentId, const EntityIdList &selectedEntityIds) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (selectedEntityIds.empty())
        {
            return false;
        }

        // Disable reparenting to the root level
        if (!newParentId.IsValid())
        {
            return false;
        }

        // If the new parent is a closed container, bail.
        if (auto containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get(); !containerEntityInterface->IsContainerOpen(newParentId))
        {
            return false;
        }

        // Ignore entities not owned by the editor context. It is assumed that all entities belong
        // to the same context since multiple selection doesn't span across views.
        for (const AZ::EntityId& entityId : selectedEntityIds)
        {
            bool isEditorEntity = false;
            EditorEntityContextRequestBus::BroadcastResult(isEditorEntity, &EditorEntityContextRequests::IsEditorEntity, entityId);
            if (!isEditorEntity)
            {
                return false;
            }

            bool isEntityEditable = true;
            EBUS_EVENT_RESULT(isEntityEditable, ToolsApplicationRequests::Bus, IsEntityEditable, entityId);
            if (!isEntityEditable)
            {
                return false;
            }

            // when in a forced sort mode, reject reordering under the same parent
            if (m_sortMode != EntityOutliner::DisplaySortMode::Manually)
            {
                AZ::EntityId currentParentId;
                AZ::TransformBus::EventResult(currentParentId, entityId, &AZ::TransformBus::Events::GetParentId);
                if (currentParentId == newParentId)
                {
                    return false;
                }
            }

            bool isLayerEntity = false;
            Layers::EditorLayerComponentRequestBus::EventResult(
                isLayerEntity,
                entityId,
                &Layers::EditorLayerComponentRequestBus::Events::HasLayer);
            // Layers can only have other layers as parents, or have no parent.
            if (isLayerEntity)
            {
                bool newParentIsLayer = false;
                Layers::EditorLayerComponentRequestBus::EventResult(
                    newParentIsLayer,
                    newParentId,
                    &Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                if (!newParentIsLayer)
                {
                    return false;
                }
            }
        }

        //Only check the entity pointer if the entity id is valid because
        //we want to allow dragging items to unoccupied parts of the tree to un-parent them
        AZ::Entity* newParentEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newParentEntity, &AZ::ComponentApplicationRequests::FindEntity, newParentId);
        if (!newParentEntity)
        {
            return false;
        }

        //reject dragging on to yourself or your children
        AZ::EntityId currParentId = newParentId;
        while (currParentId.IsValid())
        {
            if (AZStd::find(selectedEntityIds.begin(), selectedEntityIds.end(), currParentId) != selectedEntityIds.end())
            {
                return false;
            }

            AZ::EntityId tempParentId;
            AZ::TransformBus::EventResult(tempParentId, currParentId, &AZ::TransformBus::Events::GetParentId);
            currParentId = tempParentId;
        }

        return true;
    }

    bool EntityOutlinerListModel::ReparentEntities(const AZ::EntityId& newParentId, const EntityIdList &selectedEntityIds, const AZ::EntityId& beforeEntityId, ReparentForInvalid forInvalid)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (!CanReparentEntities(newParentId, selectedEntityIds))
        {
            return false;
        }

        m_isFilterDirty = true;

        //capture child entity order before re-parent operation, which will automatically add order info if not present
        EntityOrderArray entityOrderArray = GetEntityChildOrder(newParentId);

        //search for the insertion entity in the order array
        const auto beforeEntityItr = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), beforeEntityId);
        const bool hasInvalidIndex = beforeEntityItr == entityOrderArray.end();
        if (hasInvalidIndex && forInvalid == None) 
        {
            return false;
        }

        ScopedUndoBatch undo("Reparent Entities");
        // The new parent is dirty due to sort change(s)
        undo.MarkEntityDirty(GetEntityIdForSortInfo(newParentId));

        EntityIdList processedEntityIds;
        {
            ScopedUndoBatch undo2("Reparent Entities");

            for (AZ::EntityId entityId : selectedEntityIds)
            {
                AZ::EntityId oldParentId;
                EBUS_EVENT_ID_RESULT(oldParentId, entityId, AZ::TransformBus, GetParentId);

                //  Guarding this to prevent the entity from being marked dirty when the parent doesn't change.
                EBUS_EVENT_ID(entityId, AZ::TransformBus, SetParent, newParentId);

                // The old parent is dirty due to sort change
                undo2.MarkEntityDirty(GetEntityIdForSortInfo(oldParentId));

                // The reparented entity is dirty due to parent change
                undo2.MarkEntityDirty(entityId);

                processedEntityIds.push_back(entityId);

                ComponentEntityEditorRequestBus::Event(
                    entityId, &ComponentEntityEditorRequestBus::Events::RefreshVisibilityAndLock);
            }
        }

    
        //replace order info matching selection with bad values rather than remove to preserve layout
        for (auto& id : entityOrderArray)
        {
            if (AZStd::find(processedEntityIds.begin(), processedEntityIds.end(), id) != processedEntityIds.end())
            {
                id = AZ::EntityId();
            }
        }

        //if adding to a valid parent entity, insert at the found entity location or at the head/tail depending on placeAtTail flag
        if (hasInvalidIndex) 
        {
            switch(forInvalid)
            {
                case AppendEnd:
                    entityOrderArray.insert(entityOrderArray.end(), processedEntityIds.begin(), processedEntityIds.end());
                    break;
                case AppendBeginning:
                    entityOrderArray.insert(entityOrderArray.begin(), processedEntityIds.begin(), processedEntityIds.end());
                    break;
                default:
                    AZ_Assert(false, "Unexpected type for ReparentForInvalid");
                    break;
            }
        } 
        else 
        {
            entityOrderArray.insert(beforeEntityItr, processedEntityIds.begin(), processedEntityIds.end());
        }

        //remove placeholder entity ids
        entityOrderArray.erase(AZStd::remove(entityOrderArray.begin(), entityOrderArray.end(), AZ::EntityId()), entityOrderArray.end());

        //update order array
        SetEntityChildOrder(newParentId, entityOrderArray);

        // reselect the entities to ensure they're visible if appropriate
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, processedEntityIds);

        EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_Values);
        return true;
    }

    QMimeData* EntityOutlinerListModel::mimeData(const QModelIndexList& indexes) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        EditorEntityIdContainer entityIdList;
        for (const QModelIndex& index : indexes)
        {
            if (index.column() == 0) // ignore all but one cell in row
            {
                AZ::EntityId entityId = GetEntityFromIndex(index);
                if (entityId.IsValid())
                {
                    entityIdList.m_entityIds.push_back(entityId);
                }
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

        mimeDataPtr->setData(EditorEntityIdContainer::GetMimeType(), encodedData);
        return mimeDataPtr;
    }

    QStringList EntityOutlinerListModel::mimeTypes() const
    {
        QStringList list = QAbstractItemModel::mimeTypes();
        list.append(EditorEntityIdContainer::GetMimeType());
        list.append(ComponentTypeMimeData::GetMimeType());
        list.append(ComponentAssetMimeDataContainer::GetMimeType());
        return list;
    }

    void EntityOutlinerListModel::QueueEntityUpdate(AZ::EntityId entityId)
    {
        if (m_layoutResetQueued)
        {
            return;
        }
        if (!m_entityChangeQueued)
        {
            m_entityChangeQueued = true;
            QTimer::singleShot(0, this, &EntityOutlinerListModel::ProcessEntityUpdates);
        }
        m_entityChangeQueue.insert(entityId);
    }

    void EntityOutlinerListModel::QueueAncestorUpdate(AZ::EntityId entityId)
    {
        //primarily needed for ancestors that reflect child state (selected, locked, hidden)
        AZ::EntityId parentId;
        EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);
        for (AZ::EntityId currentId = parentId; currentId.IsValid(); currentId = parentId)
        {
            QueueEntityUpdate(currentId);
            parentId.SetInvalid();
            EditorEntityInfoRequestBus::EventResult(parentId, currentId, &EditorEntityInfoRequestBus::Events::GetParent);
        }
    }

    void EntityOutlinerListModel::QueueEntityToExpand(AZ::EntityId entityId, bool expand)
    {
        m_entityExpansionState[entityId] = expand;
        m_entityExpandQueue.insert(entityId);
        QueueEntityUpdate(entityId);
    }

    // Helper struct for tracking a range of indices.
    struct ModelIndexRange
    {
    public:
        QModelIndex m_start;
        QModelIndex m_end;
    };


    void EntityOutlinerListModel::ProcessEntityUpdates()
    {
        AZ_PROFILE_FUNCTION(Editor);
        if (!m_entityChangeQueued)
        {
            return;
        }
        m_entityChangeQueued = false;
        if (m_layoutResetQueued)
        {
            return;
        }

        {
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModel::ProcessEntityUpdates:ExpandQueue");
            for (auto entityId : m_entityExpandQueue)
            {
                emit ExpandEntity(entityId, IsExpanded(entityId));
            };
            m_entityExpandQueue.clear();
        }

        {
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModel::ProcessEntityUpdates:SelectQueue");
            for (auto entityId : m_entitySelectQueue)
            {
                emit SelectEntity(entityId, IsSelected(entityId));
            };
            m_entitySelectQueue.clear();
        }

        if (!m_entityChangeQueue.empty())
        {
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModel::ProcessEntityUpdates:ChangeQueue");

            for (auto entityId : m_entityChangeQueue)
            {
                if (entityId.IsValid())
                {
                    const QModelIndex beginIndex = GetIndexFromEntity(entityId, ColumnName);
                    const QModelIndex endIndex = createIndex(beginIndex.row(), VisibleColumnCount - 1, beginIndex.internalId());
                    emit dataChanged(beginIndex, endIndex);
                }
            }
        
            m_entityChangeQueue.clear();
        }

        {
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModel::ProcessEntityUpdates:LayoutChanged");
            if (m_entityLayoutQueued)
            {
                emit layoutAboutToBeChanged();
                emit layoutChanged();
            }
            m_entityLayoutQueued = false;
        }

        {
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModel::ProcessEntityUpdates:InvalidateFilter");
            if (m_isFilterDirty)
            {
                InvalidateFilter();
            }
        }
    }

    void EntityOutlinerListModel::OnEntityInfoResetBegin()
    {
        emit EnableSelectionUpdates(false);
        beginResetModel();
    }

    void EntityOutlinerListModel::OnEntityInfoResetEnd()
    {
        m_layoutResetQueued = true;
        endResetModel();
        QTimer::singleShot(0, this, &EntityOutlinerListModel::ProcessEntityInfoResetEnd);
    }

    void EntityOutlinerListModel::ProcessEntityInfoResetEnd()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        m_layoutResetQueued = false;
        m_entityChangeQueued = false;
        m_entityChangeQueue.clear();
        QueueEntityUpdate(AZ::EntityId());
        emit EnableSelectionUpdates(true);
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedAddChildBegin(AZ::EntityId parentId, AZ::EntityId childId)
    {
        //add/remove operations trigger selection change signals which assert and break undo/redo operations in progress in inspector etc.
        //so disallow selection updates until change is complete
        emit EnableSelectionUpdates(false);
        auto parentIndex = GetIndexFromEntity(parentId);
        auto childIndex = GetIndexFromEntity(childId);
        beginInsertRows(parentIndex, childIndex.row(), childIndex.row());
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
    {
        (void)parentId;
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        endInsertRows();

        //expand ancestors if a new descendant is already selected
        if ((IsSelected(childId) || HasSelectedDescendant(childId)) && !m_dropOperationInProgress)
        {
            ExpandAncestors(childId);
        }

        //restore selection and expansion state for previously registered entity ids (for the view/model only)
        RestoreDescendantSelection(childId);
        RestoreDescendantExpansion(childId);

        //must refresh partial lock/visibility of parents
        m_isFilterDirty = true;
        QueueAncestorUpdate(childId);
        emit EnableSelectionUpdates(true);
    }

    void EntityOutlinerListModel::OnEntityRuntimeActivationChanged(AZ::EntityId entityId, [[maybe_unused]] bool activeOnStart)
    {
        QueueEntityUpdate(entityId);
    }

    void EntityOutlinerListModel::OnContainerEntityStatusChanged(AZ::EntityId entityId, [[maybe_unused]] bool open)
    {
        QModelIndex changedIndex = GetIndexFromEntity(entityId);

        // Trigger a refresh of all direct children so that they can be shown or hidden appropriately.
        int numChildren = rowCount(changedIndex);
        if (numChildren > 0)
        {
            emit dataChanged(index(0, 0, changedIndex), index(numChildren - 1, ColumnCount - 1, changedIndex));
        }

        // Always expand containers
        QueueEntityToExpand(entityId, true);
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedRemoveChildBegin([[maybe_unused]] AZ::EntityId parentId, [[maybe_unused]] AZ::EntityId childId)
    {
        //add/remove operations trigger selection change signals which assert and break undo/redo operations in progress in inspector etc.
        //so disallow selection updates until change is complete
        emit EnableSelectionUpdates(false);

        auto parentIndex = GetIndexFromEntity(parentId);
        auto childIndex = GetIndexFromEntity(childId);
        beginRemoveRows(parentIndex, childIndex.row(), childIndex.row());
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
    {
        (void)childId;
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        endRemoveRows();

        //must refresh partial lock/visibility of parents
        m_isFilterDirty = true;
        QueueAncestorUpdate(parentId);
        emit EnableSelectionUpdates(true);

        // Remove any pending updates for this removed entity.
        m_entityChangeQueue.erase(childId);
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedOrderBegin(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index)
    {
        (void)parentId;
        (void)childId;
        (void)index;
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedOrderEnd(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index)
    {
        AZ_PROFILE_FUNCTION(Editor);
        (void)index;
        m_entityLayoutQueued = true;
        QueueEntityUpdate(parentId);
        QueueEntityUpdate(childId);
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedSelection(AZ::EntityId entityId, bool selected)
    {
        //update all ancestors because they will show highlight if ancestor is selected
        QueueAncestorUpdate(entityId);

        //expand ancestors upon new selection
        if (selected && m_autoExpandEnabled)
        {
            ExpandAncestors(entityId);
        }

        //notify observers
        emit SelectEntity(entityId, selected);
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedLocked(AZ::EntityId entityId, bool /*locked*/)
    {
        //update all ancestors because they will show partial state for descendants
        QueueEntityUpdate(entityId);
        QueueAncestorUpdate(entityId);
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool /*visible*/)
    {
        //update all ancestors because they will show partial state for descendants
        QueueEntityUpdate(entityId);
        QueueAncestorUpdate(entityId);
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name)
    {
        (void)name;
        QueueEntityUpdate(entityId);

        bool isSelected = false;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            isSelected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, entityId);

        if (isSelected)
        {
            // Ask the system to scroll to the entity in case it is off screen after the rename
            EntityOutlinerModelNotificationBus::Broadcast(&EntityOutlinerModelNotifications::QueueScrollToNewContent, entityId);
        }
    }

    void EntityOutlinerListModel::OnEntityInfoUpdatedUnsavedChanges(AZ::EntityId entityId)
    {
        QueueEntityUpdate(entityId);
    }

    QModelIndex EntityOutlinerListModel::GetIndexFromEntity(const AZ::EntityId& entityId, int column) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (entityId.IsValid())
        {
            AZ::EntityId parentId;
            EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

            AZStd::size_t row = 0;
            EditorEntityInfoRequestBus::EventResult(row, parentId, &EditorEntityInfoRequestBus::Events::GetChildIndex, entityId);
            return createIndex(static_cast<int>(row), column, static_cast<AZ::u64>(entityId));
        }

        return QModelIndex();
    }

    AZ::EntityId EntityOutlinerListModel::GetEntityFromIndex(const QModelIndex& index) const
    {
        return index.isValid() ? AZ::EntityId(static_cast<AZ::u64>(index.internalId())) : AZ::EntityId();
    }

    void EntityOutlinerListModel::SearchStringChanged(const AZStd::string& filter)
    {
        m_isFilterDirty = true;
        if (filter.size() > 0)
        {
            CacheSelectionIfAppropriate();
        }

        m_filterString = filter;
        InvalidateFilter();

        RestoreSelectionIfAppropriate();
    }

    void EntityOutlinerListModel::SearchFilterChanged(const AZStd::vector<ComponentTypeValue>& componentFilters)
    {
        m_isFilterDirty = true;
        if (componentFilters.size() > 0)
        {
            CacheSelectionIfAppropriate();
        }

        m_componentFilters = AZStd::move(componentFilters);
        InvalidateFilter();

        RestoreSelectionIfAppropriate();
    }

    bool EntityOutlinerListModel::ShouldOverrideUnfilteredSelection()
    {
        EntityIdList currentSelection;
        ToolsApplicationRequests::Bus::BroadcastResult(currentSelection, &ToolsApplicationRequests::GetSelectedEntities);

        // If new selection is greater, then we need to override
        if (currentSelection.size() > m_unfilteredSelectionEntityIds.size())
        {
            return true;
        }

        for (auto& entityId : currentSelection)
        {
            // If one element in the current selection doesn't exist in our unfiltered selection, we need to override with the new selection
            auto it = AZStd::find(m_unfilteredSelectionEntityIds.begin(), m_unfilteredSelectionEntityIds.end(), entityId);
            if (it == m_unfilteredSelectionEntityIds.end())
            {
                return true;
            }
        }

        if (currentSelection.empty())
        {
            for (auto& entityId : m_unfilteredSelectionEntityIds)
            {
                // If any of the entities are visible, then the user must have forcibly cleared selection
                if (!IsFiltered(entityId))
                {
                    return true;
                }
            }
        }

        return false;
    }

    void EntityOutlinerListModel::CacheSelectionIfAppropriate()
    {
        if (ShouldOverrideUnfilteredSelection())
        {
            ToolsApplicationRequests::Bus::BroadcastResult(m_unfilteredSelectionEntityIds, &ToolsApplicationRequests::GetSelectedEntities);
        }
    }

    void EntityOutlinerListModel::RestoreSelectionIfAppropriate()
    {
        if (m_unfilteredSelectionEntityIds.size() > 0)
        {
            // Store these in a temp list so it doesn't get cleared mid-loop by external logic
            EntityIdList tempList = m_unfilteredSelectionEntityIds;

            for (auto& entityId : tempList)
            {
                if (!IsFiltered(entityId))
                {
                    ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::MarkEntitySelected, entityId);
                }
            }

            m_unfilteredSelectionEntityIds = tempList;
        }

        if (m_unfilteredSelectionEntityIds.size() > 0 && m_componentFilters.size() == 0 && m_filterString.size() == 0)
        {
            m_unfilteredSelectionEntityIds.clear();
        }
    }

    void EntityOutlinerListModel::AfterEntitySelectionChanged(const EntityIdList&, const EntityIdList&)
    {
        if (m_unfilteredSelectionEntityIds.size() > 0)
        {
            if (ShouldOverrideUnfilteredSelection())
            {
                m_unfilteredSelectionEntityIds.clear();
            }
        }
    }

    void EntityOutlinerListModel::OnEntityExpanded(const AZ::EntityId& entityId)
    {
        m_isFilterDirty = true;
        m_entityExpansionState[entityId] = true;
        QueueEntityUpdate(entityId);
    }

    void EntityOutlinerListModel::OnEntityCollapsed(const AZ::EntityId& entityId)
    {
        m_isFilterDirty = true;
        m_entityExpansionState[entityId] = false;
        QueueEntityUpdate(entityId);
    }

    void EntityOutlinerListModel::InvalidateFilter()
    {
        FilterEntity(AZ::EntityId());

        // Emit data changed directly as it is immediately valid
        auto modelIndex = GetIndexFromEntity(AZ::EntityId());
        if (modelIndex.isValid())
        {
            emit dataChanged(modelIndex, modelIndex, { VisibilityRole });
        }
        m_isFilterDirty = false;
    }

    void EntityOutlinerListModel::OnEditorEntityDuplicated(const AZ::EntityId& oldEntity, const AZ::EntityId& newEntity)
    {
        AZStd::list_iterator<AZStd::pair<AZ::EntityId, bool>> expansionIter = m_entityExpansionState.find(oldEntity);
        QueueEntityToExpand(newEntity, expansionIter != m_entityExpansionState.end() && expansionIter->second);
    }

    void EntityOutlinerListModel::ExpandAncestors(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //typically to reveal selected entities, expand all parent entities
        if (entityId.IsValid())
        {
            AZ::EntityId parentId;
            EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);
            QueueEntityToExpand(parentId, true);
            ExpandAncestors(parentId);
        }
    }

    bool EntityOutlinerListModel::IsExpanded(const AZ::EntityId& entityId) const
    {
        auto expandedItr = m_entityExpansionState.find(entityId);
        return expandedItr != m_entityExpansionState.end() && expandedItr->second;
    }

    void EntityOutlinerListModel::RestoreDescendantExpansion(const AZ::EntityId& entityId)
    {
        //re-expand/collapse entities in the model that may have been previously removed or rearranged, resulting in new model indices
        if (entityId.IsValid())
        {
            QueueEntityToExpand(entityId, IsExpanded(entityId));

            EntityIdList children;
            EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);
            for (auto childId : children)
            {
                RestoreDescendantExpansion(childId);
            }
        }
    }

    void EntityOutlinerListModel::RestoreDescendantSelection(const AZ::EntityId& entityId)
    {
        //re-select entities in the model that may have been previously removed or rearranged, resulting in new model indices
        if (entityId.IsValid())
        {
            m_entitySelectQueue.insert(entityId);
            QueueEntityUpdate(entityId);

            EntityIdList children;
            EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);
            for (auto childId : children)
            {
                RestoreDescendantSelection(childId);
            }
        }
    }

    bool EntityOutlinerListModel::FilterEntity(const AZ::EntityId& entityId)
    {
        bool isFilterMatch = true;

        if (m_filterString.size() > 0)
        {
            AZStd::string name;
            EditorEntityInfoRequestBus::EventResult(name, entityId, &EditorEntityInfoRequestBus::Events::GetName);

            if (AzFramework::StringFunc::Find(name.c_str(), m_filterString.c_str()) == AZStd::string::npos
                && AZStd::to_string(static_cast<AZ::u64>(entityId)) != m_filterString)
            {
                isFilterMatch = false;
            }
            else
            {
                QueueEntityUpdate(entityId);
            }
        }

        int numberOfGlobalFlags = 0;
        // First check if any of the global flags are valid
        if (isFilterMatch && m_componentFilters.size() > 0)
        {
            int globalFlags = 0;

            for (ComponentTypeValue& componentType : m_componentFilters)
            {
                if (componentType.m_globalVal >= 0)
                {
                    switch (componentType.m_globalVal)
                    {
                    case 0:
                        globalFlags |= static_cast<int>(GlobalSearchCriteriaFlags::Unlocked);
                        break;
                    case 1:
                        globalFlags |= static_cast<int>(GlobalSearchCriteriaFlags::Locked);
                        break;
                    case 2:
                        globalFlags |= static_cast<int>(GlobalSearchCriteriaFlags::Visible);
                        break;
                    case 3:
                        globalFlags |= static_cast<int>(GlobalSearchCriteriaFlags::Hidden);
                        break;
                    }
                    ++numberOfGlobalFlags;
                }
            }
            if ((globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Unlocked)) && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Locked)))
            {
                globalFlags &= (static_cast<int>(GlobalSearchCriteriaFlags::Visible) | static_cast<int>(GlobalSearchCriteriaFlags::Hidden));
            }
            if ((globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Visible)) && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Hidden)))
            {
                globalFlags &= (static_cast<int>(GlobalSearchCriteriaFlags::Unlocked) | static_cast<int>(GlobalSearchCriteriaFlags::Locked));
            }
            if (globalFlags)
            {
                bool visibleFlag = false;
                EditorVisibilityRequestBus::EventResult(visibleFlag, entityId, &EditorVisibilityRequests::GetVisibilityFlag);
                if (visibleFlag && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Hidden)))
                {
                    isFilterMatch = false;
                }
                if (!visibleFlag && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Visible)))
                {
                    isFilterMatch = false;
                }
                bool isLocked = false;
                EditorLockComponentRequestBus::EventResult(isLocked, entityId, &EditorLockComponentRequests::GetLocked);
                if (isLocked && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Unlocked)))
                {
                    isFilterMatch = false;
                }
                if (!isLocked && (globalFlags & static_cast<int>(GlobalSearchCriteriaFlags::Locked)))
                {
                    isFilterMatch = false;
                }
            }
        }

        // If we matched the filter string and have any component filters, make sure we match those too
        if (isFilterMatch && m_componentFilters.size() > numberOfGlobalFlags)
        {
            bool hasMatchingComponent = false;

            for (ComponentTypeValue& componentType : m_componentFilters)
            {
                if (componentType.m_globalVal < 0)
                {
                if (EntityHasComponentOfType(entityId, componentType.m_uuid, true, true))
                    {
                        hasMatchingComponent = true;
                        break;
                    }
                }
            }

            isFilterMatch &= hasMatchingComponent;
        }

        EntityIdList children;
        EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);
        for (auto childId : children)
        {
            isFilterMatch |= FilterEntity(childId);
        }

        // If we now match the filter and we previously didn't and we're set in an expanded state
        // we need to queue an expand again so that the treeview state matches our internal saved state
        if (isFilterMatch && m_entityFilteredState[entityId] && IsExpanded(entityId))
        {
            QueueEntityToExpand(entityId, true);
        }

        m_entityFilteredState[entityId] = !isFilterMatch;

        return isFilterMatch;
    }

    bool EntityOutlinerListModel::IsFiltered(const AZ::EntityId& entityId) const
    {
        auto hiddenItr = m_entityFilteredState.find(entityId);
        return hiddenItr != m_entityFilteredState.end() && hiddenItr->second;
    }

    void EntityOutlinerListModel::EnableAutoExpand(bool enable)
    {
        m_autoExpandEnabled = enable;
    }

    void EntityOutlinerListModel::SetDropOperationInProgress(bool inProgress)
    {
        m_dropOperationInProgress = inProgress;
    }

    bool EntityOutlinerListModel::HasSelectedDescendant(const AZ::EntityId& entityId) const
    {
        //TODO result can be cached in mutable map and cleared when any descendant changes to avoid recursion in deep hierarchies
        EntityIdList children;
        EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);
        for (auto childId : children)
        {
            bool isSelected = false;
            EditorEntityInfoRequestBus::EventResult(isSelected, childId, &EditorEntityInfoRequestBus::Events::IsSelected);
            if (isSelected || HasSelectedDescendant(childId))
            {
                return true;
            }
        }
        return false;
    }

    bool EntityOutlinerListModel::AreAllDescendantsSameLockState(const AZ::EntityId& entityId) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //TODO result can be cached in mutable map and cleared when any descendant changes to avoid recursion in deep hierarchies
        bool isLocked = false;
        EditorEntityInfoRequestBus::EventResult(isLocked, entityId, &EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);

        EntityIdList children;
        EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);
        for (auto childId : children)
        {
            bool isLockedChild = false;
            EditorEntityInfoRequestBus::EventResult(isLockedChild, childId, &EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
            if (isLocked != isLockedChild || !AreAllDescendantsSameLockState(childId))
            {
                return false;
            }
        }
        return true;
    }

    bool EntityOutlinerListModel::AreAllDescendantsSameVisibleState(const AZ::EntityId& entityId) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //TODO result can be cached in mutable map and cleared when any descendant changes to avoid recursion in deep hierarchies

        bool isVisible = IsEntitySetToBeVisible(entityId);

        EntityIdList children;
        EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);
        for (auto childId : children)
        {
            bool isVisibleChild = IsEntitySetToBeVisible(childId);
            if (isVisible != isVisibleChild || !AreAllDescendantsSameVisibleState(childId))
            {
                return false;
            }
        }
        return true;
    }

    bool EntityOutlinerListModel::IsInLayerWithProperty(AZ::EntityId entityId, const LayerProperty& layerProperty) const
    {
        while (entityId.IsValid())
        {
            AZ::EntityId parentId;
            EditorEntityInfoRequestBus::EventResult(
                parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);

            bool isParentLayer = false;
            Layers::EditorLayerComponentRequestBus::EventResult(
                isParentLayer,
                parentId,
                &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

            if (isParentLayer)
            {
                if (layerProperty == LayerProperty::Locked)
                {
                    bool isParentLayerLocked = false;
                    EditorEntityInfoRequestBus::EventResult(
                        isParentLayerLocked, parentId, &EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
                    if (isParentLayerLocked)
                    {
                        return true;
                    }
                    // If this layer wasn't locked, keep checking the hierarchy, a layer above this one may be locked.
                }
                else if (layerProperty == LayerProperty::Invisible)
                {
                    bool isParentVisible = IsEntitySetToBeVisible(parentId);
                    if (!isParentVisible)
                    {
                        return true;
                    }
                    // If this layer was visible, keep checking the hierarchy, a layer above this one may be invisible.
                }
            }

            entityId = parentId;
        }
        return false;
    }

    void EntityOutlinerListModel::OnEntityInitialized(const AZ::EntityId& entityId)
    {
        bool isEditorEntity = false;
        EditorEntityContextRequestBus::BroadcastResult(isEditorEntity, &EditorEntityContextRequests::IsEditorEntity, entityId);
        if (!isEditorEntity)
        {
            return;
        }

        if (!m_beginStartPlayInEditor && (m_filterString.size() > 0 || m_componentFilters.size() > 0))
        {
            m_isFilterDirty = true;
            emit ReapplyFilter();
        }
    }

    void EntityOutlinerListModel::OnEntityCompositionChanged(const EntityIdList& /*entityIds*/)
    {
        if (m_componentFilters.size() > 0)
        {
            m_isFilterDirty = true;
            emit ReapplyFilter();
        }
    }

    void EntityOutlinerListModel::OnContextReset()
    {
        if (m_filterString.size() > 0 || m_componentFilters.size() > 0)
        {
            m_isFilterDirty = true;
            emit ResetFilter();
        }
    }

    void EntityOutlinerListModel::OnStartPlayInEditorBegin()
    {
        m_beginStartPlayInEditor = true;
    }

    void EntityOutlinerListModel::OnStartPlayInEditor()
    {
        m_beginStartPlayInEditor = false;
    }

    ////////////////////////////////////////////////////////////////////////////
    // OutlinerItemDelegate
    ////////////////////////////////////////////////////////////////////////////
    EntityOutlinerItemDelegate::EntityOutlinerItemDelegate(QWidget* parent)
        : QStyledItemDelegate(parent)
        , m_visibilityCheckBoxes(parent, "Visibility", EntityOutlinerListModel::PartiallyVisibleRole, EntityOutlinerListModel::InvisibleAncestorRole)
        , m_lockCheckBoxes(parent, "Lock", EntityOutlinerListModel::PartiallyLockedRole, EntityOutlinerListModel::LockedAncestorRole)
    {
        m_editorEntityFrameworkInterface = AZ::Interface<AzToolsFramework::EditorEntityUiInterface>::Get();
        AZ_Assert((m_editorEntityFrameworkInterface != nullptr),
            "EntityOutlinerItemDelegate requires a EditorEntityFrameworkInterface instance on Construction.");

        m_readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
        AZ_Assert(
            (m_readOnlyEntityPublicInterface != nullptr),
            "EntityOutlinerItemDelegate requires a ReadOnlyEntityPublicInterface instance on Construction.");
    }

    EntityOutlinerItemDelegate::CheckboxGroup::CheckboxGroup(QWidget* parent, AZStd::string prefix,
        const EntityOutlinerListModel::Roles mixedRole, const EntityOutlinerListModel::Roles overriddenRole)
        : m_default(parent)
        , m_mixed(parent)
        , m_overridden(parent)
        , m_defaultHover(parent)
        , m_mixedHover(parent)
        , m_overriddenHover(parent)
        , m_mixedRole(mixedRole)
        , m_overriddenRole(overriddenRole)
    {
        m_default.setObjectName(prefix.data());
        m_mixed.setObjectName((prefix + "Mixed").data());
        m_overridden.setObjectName((prefix + "Overridden").data());

        m_defaultHover.setObjectName((prefix + "Hover").data());
        m_mixedHover.setObjectName((prefix + "MixedHover").data());
        m_overriddenHover.setObjectName((prefix + "OverriddenHover").data());
    }

    EntityOutlinerCheckBox* EntityOutlinerItemDelegate::CheckboxGroup::SelectCheckboxToRender(const QModelIndex& index, bool isHovered)
    {
        // Ancestor Override
        if (index.data(m_overriddenRole).value<bool>())
        {
            if (isHovered)
            {
                return &m_overriddenHover;
            }
            else
            {
                return &m_overridden;
            }
        }
        // Mixed
        else if (index.data(m_mixedRole).value<bool>())
        {
            if (isHovered)
            {
                return &m_mixedHover;
            }
            else
            {
                return &m_mixed;
            }
        }
        //Default
        if (isHovered)
        {
            return &m_defaultHover;
        }
        else
        {
            return &m_default;
        }
    }

    void EntityOutlinerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        // Customize option to prevent Qt from painting the default focus rectangle
        QStyleOptionViewItem customOption{option};
        if (customOption.state & QStyle::State_HasFocus)
        {
            customOption.state ^= QStyle::State_HasFocus;
        }

        // Retrieve the Entity UI Handler
        auto firstColumnIndex = index.siblingAtColumn(0);
        AZ::EntityId entityId(firstColumnIndex.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
        auto entityUiHandler = m_editorEntityFrameworkInterface->GetHandler(entityId);

        const bool isSelected = (option.state & QStyle::State_Selected);
        const bool isHovered = (option.state & QStyle::State_MouseOver) && (option.state & QStyle::State_Enabled);

        // Paint the Selection/Hover Rect
        PaintSelectionHoverRect(painter, option, index, isSelected, isHovered);

        // Paint Ancestor Backgrounds
        PaintAncestorBackgrounds(painter, option, index);

        // Paint the Background
        if (entityUiHandler != nullptr)
        {
            entityUiHandler->PaintItemBackground(painter, option, index);
        }

        switch (index.column())
        {
            case EntityOutlinerListModel::ColumnVisibilityToggle:
            case EntityOutlinerListModel::ColumnLockToggle:
            {
                if (!entityUiHandler || entityUiHandler->CanToggleLockVisibility(entityId))
                {
                    // Paint the Visibility and Lock state checkboxes
                    PaintCheckboxes(painter, option, index, isHovered);
                }
            }
            break;
            case EntityOutlinerListModel::ColumnName:
            {
                // Draw a dashed line around any visible, collapsed entry in the outliner that has
                // children underneath it currently selected.
                if (!isSelected && !index.data(EntityOutlinerListModel::ExpandedRole).template value<bool>()
                    && index.data(EntityOutlinerListModel::ChildSelectedRole).template value<bool>())
                {
                    PaintDescendantSelectedRect(painter, option, index);
                }

                PaintEntityNameAsRichText(painter, customOption, index);

                // Paint Read-Only icon if necessary
                if (m_readOnlyEntityPublicInterface->IsReadOnly(entityId))
                {
                    PaintReadOnlyIcon(painter, option, index);
                }
            }
            break;
            default:
            {
                QStyledItemDelegate::paint(painter, customOption, index);
            }
            break;
        }

        // Paint Ancestor Foregrounds
        PaintAncestorForegrounds(painter, option, index);

        // Paint the Foreground
        if (entityUiHandler != nullptr)
        {
            entityUiHandler->PaintItemForeground(painter, option, index);
        }
    }

    void EntityOutlinerItemDelegate::PaintAncestorBackgrounds(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        // Go through ancestors and add them to the stack
        AZStd::stack<QModelIndex> handlerStack;

        for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
        {
            handlerStack.push(ancestorIndex);
        }

        // Apply the ancestor overrides from top to bottom
        while (!handlerStack.empty())
        {
            QModelIndex ancestorIndex = handlerStack.top();
            handlerStack.pop();

            AZ::EntityId ancestorEntityId(ancestorIndex.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
            auto ancestorUiHandler = m_editorEntityFrameworkInterface->GetHandler(ancestorEntityId);

            if (ancestorUiHandler != nullptr)
            {
                ancestorUiHandler->PaintDescendantBackground(painter, option, ancestorIndex, index);
            }
        }
    }

    void EntityOutlinerItemDelegate::PaintSelectionHoverRect(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& /*index*/,
        bool isSelected, bool isHovered) const
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);

        if (isSelected || isHovered)
        {
            QPainterPath backgroundPath;
            QRect backgroundRect(option.rect);

            backgroundPath.addRect(backgroundRect);

            QColor backgroundColor = s_hoverColor;
            if (isSelected)
            {
                backgroundColor = s_selectedColor;
            }

            painter->fillPath(backgroundPath, backgroundColor);
        }

        painter->restore();
    }

    void EntityOutlinerItemDelegate::PaintAncestorForegrounds(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        // Ancestor foregrounds are painted on top of the childrens'.
        for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
        {
            AZ::EntityId ancestorEntityId(ancestorIndex.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
            auto ancestorUiHandler = m_editorEntityFrameworkInterface->GetHandler(ancestorEntityId);

            if (ancestorUiHandler != nullptr)
            {
                ancestorUiHandler->PaintDescendantForeground(painter, option, ancestorIndex, index);
            }
        }
    }

    void EntityOutlinerItemDelegate::PaintCheckboxes(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
        bool isHovered) const
    {
        QPalette checkboxPalette;
        QColor transparentColor(0, 0, 0, 0);
        checkboxPalette.setColor(QPalette::ColorRole::Window, transparentColor);

        // We're only using these check boxes as renderers so their actual state doesn't matter.
        // We can set it right before we draw using information from the model data.
        if (index.column() == EntityOutlinerListModel::ColumnVisibilityToggle)
        {
            painter->save();
            painter->translate(option.rect.topLeft());

            EntityOutlinerCheckBox* checkboxToRender = m_visibilityCheckBoxes.SelectCheckboxToRender(index, isHovered);
            checkboxToRender->setChecked(index.data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked);
            checkboxToRender->setPalette(checkboxPalette);
            checkboxToRender->render(painter);

            painter->restore();
        }

        if (index.column() == EntityOutlinerListModel::ColumnLockToggle)
        {
            painter->save();
            painter->translate(option.rect.topLeft());

            EntityOutlinerCheckBox* checkboxToRender = m_lockCheckBoxes.SelectCheckboxToRender(index, isHovered);
            checkboxToRender->setChecked(index.data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked);
            checkboxToRender->setPalette(checkboxPalette);
            checkboxToRender->render(painter);

            painter->restore();
        }
    }

    void EntityOutlinerItemDelegate::PaintDescendantSelectedRect(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
    {
        painter->save();

        QPainterPath path;

        auto newRect = option.rect;
        newRect.setHeight(newRect.height() - 1);
        path.addRect(newRect);

        // Get the foreground color of the current object to draw our sub-object-selected box
        auto targetColor = index.data(Qt::ForegroundRole).value<QBrush>().color();
        QPen pen(targetColor, 1);

        // Alter the dash pattern available for better visual appeal
        QVector<qreal> dashes;
        dashes << 8 << 2;
        pen.setStyle(Qt::PenStyle::DashLine);
        pen.setDashPattern(dashes);

        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(pen);
        painter->drawPath(path);
        painter->restore();
    }

    void EntityOutlinerItemDelegate::PaintEntityNameAsRichText(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        EntityOutlinerListModel::s_paintingName = true;
        // standard painter can't handle rich text so we have to handle it
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QStyleOptionViewItem optionV4{ option };
        initStyleOption(&optionV4, index);
        optionV4.state &= ~(QStyle::State_HasFocus | QStyle::State_Selected);

        // get the info string for this entity
        AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());
        auto entityUiHandler = m_editorEntityFrameworkInterface->GetHandler(entityId);
        QString infoString;

        // Retrieve the tooltip from the handler
        if (entityUiHandler != nullptr)
        {
            infoString = entityUiHandler->GenerateItemInfoString(entityId);
        }

        QRect textRect = optionV4.widget->style()->proxy()->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);

        QRegularExpression htmlMarkupRegex("<[^>]*>");

        // Start with the raw rich text for the entity name.
        QString entityNameRichText = optionV4.text;

        // If there is any HTML markup in the entity name, don't elide.
        if (!htmlMarkupRegex.match(entityNameRichText).hasMatch())
        {
            QFontMetrics fontMetrics(optionV4.font);
            int textWidthAvailable = textRect.width();
            // Qt uses "..." for elide, but there doesn't seem to be a way to retrieve this exact string from Qt.
            // Subtract the elide string from the width available, so it can actually appear.
            textWidthAvailable -= fontMetrics.horizontalAdvance(QObject::tr("..."));
            if (!infoString.isEmpty())
            {
                QString htmlStripped = infoString;
                htmlStripped.remove(htmlMarkupRegex);
                textWidthAvailable -= fontMetrics.horizontalAdvance(htmlStripped) + 5;
            }

            entityNameRichText = fontMetrics.elidedText(optionV4.text, Qt::TextElideMode::ElideRight, textWidthAvailable);
        }

        if (!infoString.isEmpty())
        {
            entityNameRichText = QString("<table width=\"100%\" style=\"border-collapse: collapse; border-spacing: 0;\">")
                + QString("<tr><td>")
                    + QString(entityNameRichText)
                + QString("</td><td align=\"right\">")
                    + QString(infoString)
                + QString("</td></tr></table>");
        }

        // delete the text from the item so we can use the standard painter to draw the icon
        optionV4.text.clear();
        optionV4.widget->style()->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter);

        // Now we setup a Text Document so it can draw the rich text
        QTextDocument textDoc;
        textDoc.setDefaultFont(optionV4.font);
        if (option.state & QStyle::State_Enabled)
        {
            textDoc.setDefaultStyleSheet("body {color: white}");
        }
        else
        {
            textDoc.setDefaultStyleSheet("body {color: #7C7C7C}");
        }
        textDoc.setHtml("<body>" + entityNameRichText + "</body>");
        painter->translate(textRect.topLeft());
        textDoc.setTextWidth(textRect.width());
        textDoc.drawContents(painter, QRectF(0, 0, textRect.width(), textRect.height()));

        painter->restore();
        EntityOutlinerListModel::s_paintingName = false;
    }

    void EntityOutlinerItemDelegate::PaintReadOnlyIcon(QPainter* painter, const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const
    {
        // Build the rect that will be used to paint the icon
        QRect readOnlyRect = QRect(option.rect.topLeft() + s_readOnlyOffset, QSize(s_readOnlyRadius * 2, s_readOnlyRadius * 2));

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(s_readOnlyBackgroundColor);
        painter->drawEllipse(readOnlyRect.center(), s_readOnlyRadius, s_readOnlyRadius);
        s_readOnlyIcon.paint(painter, readOnlyRect);
        painter->restore();
    }

    QSize EntityOutlinerItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
    {
        // Get the height of a tall character...
        // we do this only once per 'tick'
        if (m_cachedBoundingRectOfTallCharacter.isNull())
        {
            m_cachedBoundingRectOfTallCharacter = option.fontMetrics.boundingRect("|");
            // schedule it to be reset so that if font changes sometime soon, it updates.
            auto resetFunction = [this]() 
            { 
                m_cachedBoundingRectOfTallCharacter = QRect(); 
            };

            QTimer::singleShot(0, resetFunction);
        }
  
        return QSize(0, m_cachedBoundingRectOfTallCharacter.height() + EntityOutlinerListModel::s_OutlinerSpacing);
    }

    bool EntityOutlinerItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        if (event->type() == QEvent::MouseButtonPress &&
            (index.column() == EntityOutlinerListModel::Column::ColumnVisibilityToggle || index.column() == EntityOutlinerListModel::Column::ColumnLockToggle))
        {
            // Do not propagate click to TreeView if the user clicks the visibility or lock toggles
            // This prevents selection from changing if a toggle is clicked
            return true;
        }

        if (event->type() == QEvent::MouseButtonPress)
        {
            AZ::EntityId entityId(index.data(EntityOutlinerListModel::EntityIdRole).value<AZ::u64>());

            if (auto editorEntityUiInterface = AZ::Interface<EditorEntityUiInterface>::Get(); editorEntityUiInterface != nullptr)
            {
                auto mouseEvent = static_cast<QMouseEvent*>(event);

                auto entityUiHandler = editorEntityUiInterface->GetHandler(entityId);

                if (entityUiHandler && entityUiHandler->OnOutlinerItemClick(mouseEvent->pos(), option, index))
                {                
                    return true;
                }
            }
        }

        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    EntityOutlinerCheckBox::EntityOutlinerCheckBox(QWidget* parent)
        : QCheckBox(parent)
    {
        ensurePolished();
        hide();
    }

    void EntityOutlinerCheckBox::draw(QPainter* painter)
    {
        QStyleOptionButton opt;
        initStyleOption(&opt);
        opt.rect.setWidth(m_toggleColumnWidth);
        style()->drawControl(QStyle::CE_CheckBox, &opt, painter, this);
    }

}

#include <UI/Outliner/moc_EntityOutlinerListModel.cpp>
