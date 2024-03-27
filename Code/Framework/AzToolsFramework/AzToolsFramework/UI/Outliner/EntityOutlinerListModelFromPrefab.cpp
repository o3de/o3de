/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityOutlinerListModelFromPrefab.hxx"

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
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerDisplayOptionsMenu.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerDragAndDropContext.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerSortFilterProxyModel.hxx>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerTreeView.hxx>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerCacheBus.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Editor/RichTextHighlighter.h>


#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

////////////////////////////////////////////////////////////////////////////
// EntityOutlinerListModelFromPrefab
////////////////////////////////////////////////////////////////////////////

namespace AzToolsFramework
{

    bool EntityOutlinerListModelFromPrefab::s_paintingName = false;

    EntityOutlinerListModelFromPrefab::EntityOutlinerListModelFromPrefab(QObject* parent)
        : QAbstractItemModel(parent)
        , m_entitySelectQueue()
        , m_entityChangeQueue()
        , m_entityChangeQueued(false)
        , m_entityLayoutQueued(false)
        , m_entityExpansionState()
        , m_entityFilteredState()
    {
        m_focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        AZ_Assert(m_focusModeInterface != nullptr, "EntityOutlinerListModelFromPrefab requires a FocusModeInterface instance on construction.");

        m_prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        AZ_Assert(
            m_prefabEditorEntityOwnershipInterface != nullptr,
            "EntityOutlinerListModelFromPrefab requires a PrefabEditorEntityOwnershipInterface instance on construction.");

        m_prefabSystemComponentInterface = AZ::Interface<Prefab::PrefabSystemComponentInterface>::Get();
        AZ_Assert(
            m_prefabSystemComponentInterface != nullptr,
            "EntityOutlinerListModelFromPrefab requires a PrefabSystemComponentInterface instance on construction.");

        // Gather the Template Id of the root prefab being edited.
        Prefab::InstanceOptionalReference rootPrefabInstance = m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        if (rootPrefabInstance.has_value())
        {
            m_rootTemplateId = rootPrefabInstance->get().GetTemplateId();
            m_rootInstance = rootPrefabInstance;
            Generate();
        }
    }

    EntityOutlinerListModelFromPrefab::~EntityOutlinerListModelFromPrefab()
    {
        FocusModeNotificationBus::Handler::BusDisconnect();
        ContainerEntityNotificationBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        EntityCompositionNotificationBus::Handler::BusDisconnect();
        AZ::EntitySystemBus::Handler::BusDisconnect();
        EditorEntityRuntimeActivationChangeNotificationBus::Handler::BusDisconnect();
    }

    void EntityOutlinerListModelFromPrefab::Initialize()
    {
        EditorEntityRuntimeActivationChangeNotificationBus::Handler::BusConnect();
        ToolsApplicationEvents::Bus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        EntityCompositionNotificationBus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

        ContainerEntityNotificationBus::Handler::BusConnect(editorEntityContextId);
        FocusModeNotificationBus::Handler::BusConnect(editorEntityContextId);

        m_editorEntityUiInterface = AZ::Interface<AzToolsFramework::EditorEntityUiInterface>::Get();
        AZ_Assert(m_editorEntityUiInterface != nullptr, "EntityOutlinerListModelFromPrefab requires a EditorEntityUiInterface instance on Initialize.");

        m_readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
        AZ_Assert(
            (m_readOnlyEntityPublicInterface != nullptr),
            "EntityOutlinerListModelFromPrefab requires a ReadOnlyEntityPublicInterface instance on Initialize.");
    }

    int EntityOutlinerListModelFromPrefab::rowCount(const QModelIndex& parent) const
    {
        if (m_indices.contains(parent))
        {
            return m_indices[parent].size();
        }

        return 0;
    }

    int EntityOutlinerListModelFromPrefab::columnCount(const QModelIndex& /*parent*/) const
    {
        return ColumnCount;
    }

    QModelIndex EntityOutlinerListModelFromPrefab::index(int row, int /* column */, const QModelIndex& parent) const
    {
        if (m_indices.contains(parent) && m_indices[parent].contains(row))
        {
            return m_indices[parent][row];
        }

        return QModelIndex();
    }

    QVariant EntityOutlinerListModelFromPrefab::data(const QModelIndex& index, int role) const
    {
        if (index.column() == ColumnName)
        {
            return dataForName(index, role);
        }

        /*
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
        */

        return QVariant();
    }

    QMap<int, QVariant> EntityOutlinerListModelFromPrefab::itemData(const QModelIndex &index) const
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

    QVariant EntityOutlinerListModelFromPrefab::dataForAll(const QModelIndex& index, int role) const
    {
        switch (role)
        {
        case EntityIdRole:
            // return static_cast<AZ::u64>(id);
            return static_cast<AZ::u64>(GetEntityFromIndex(index));

        case SelectedRole:
            {
                //bool isSelected = false;
                //EditorEntityInfoRequestBus::EventResult(isSelected, id, &EditorEntityInfoRequestBus::Events::IsSelected);
                //return isSelected;
                return false;
            }

        case ChildSelectedRole:
            // return HasSelectedDescendant(id);
            return false;

        case PartiallyVisibleRole:
            //return !AreAllDescendantsSameVisibleState(id);
            return false;

        case PartiallyLockedRole:
            //return !AreAllDescendantsSameLockState(id);
            return false;

        case LockedAncestorRole:
            return false;

        case InvisibleAncestorRole:
            return false;

        case ChildCountRole:
            {
                if (m_indices.contains(index))
                {
                    return m_indices[index].size();
                }
                return 0;
            }

        case ExpandedRole:
            // return IsExpanded(id);
            return true;

        case VisibilityRole:
            // return !IsFiltered(id);
            return true;

        }

        return QVariant();
    }

    QVariant EntityOutlinerListModelFromPrefab::dataForName(const QModelIndex& index, int role) const
    {
        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            {
                /*
                if (s_paintingName && !m_filterString.empty())
                {
                    // highlight characters in filter
                    label = AzToolsFramework::RichTextHighlighter::HighlightText(label, m_filterString.c_str());
                }
                */
                return m_itemNames[index];
            }
            break;
        case Qt::ToolTipRole:
            {
                //return GetEntityTooltip(id);
                return m_itemNames[index];
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
                //return GetEntityIcon(id);
                return QIcon(QString(":/Entity/entity.svg"));
            }
            break;
        }

        return dataForAll(index, role);
    }

    QVariant EntityOutlinerListModelFromPrefab::GetEntityIcon(const AZ::EntityId& id) const
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

    QVariant EntityOutlinerListModelFromPrefab::GetEntityTooltip(const AZ::EntityId& id) const
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

    QVariant EntityOutlinerListModelFromPrefab::dataForVisibility(const QModelIndex& index, int role) const
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

    QVariant EntityOutlinerListModelFromPrefab::dataForLock(const QModelIndex& index, int role) const
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
                    // Retrieving the lock state from the lock component is ideal for drawing the lock icon in the outliner
                    // because the outliner needs to show that specific entity's lock state, and not the actual final lock state.
                    // The EditorLockComponent only knows about the specific entity's lock state and not the hierarchy.
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

    QVariant EntityOutlinerListModelFromPrefab::dataForSortIndex(const QModelIndex& index, int role) const
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

    bool EntityOutlinerListModelFromPrefab::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        switch (role)
        {
        case Qt::CheckStateRole:
        {
            if (value.canConvert<Qt::CheckState>())
            {
                const auto entityId = GetEntityFromIndex(index);

                // Disable lock and visibility toggling if the UI Handler blocked it.
                auto entityUiHandler = m_editorEntityUiInterface->GetHandler(entityId);
                bool canToggleLockVisibility = !entityUiHandler || entityUiHandler->CanToggleLockVisibility(entityId);

                // Disable lock and visibility toggling for read-only entities.
                bool isReadOnly = m_readOnlyEntityPublicInterface->IsReadOnly(entityId);

                if (canToggleLockVisibility && !isReadOnly)
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

                            ToolsApplicationEvents::Bus::Broadcast(
                                &ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, Refresh_EntireTree);
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

    QModelIndex EntityOutlinerListModelFromPrefab::parent(const QModelIndex& index) const
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

    Qt::ItemFlags EntityOutlinerListModelFromPrefab::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::ItemIsDropEnabled;
        }

        AZ::EntityId entityId = GetEntityFromIndex(index);

        // Only allow renaming the entity if the UI Handler did not block it.
        auto entityUiHandler = m_editorEntityUiInterface ? m_editorEntityUiInterface->GetHandler(entityId) : nullptr;
        bool canRename = !entityUiHandler || entityUiHandler->CanRename(entityId);

        // Disable renaming for read-only entities.
        bool isReadOnly = m_readOnlyEntityPublicInterface ? m_readOnlyEntityPublicInterface->IsReadOnly(entityId) : false;

        Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);
        switch (index.column())
        {
        case ColumnVisibilityToggle:
        case ColumnLockToggle:
            itemFlags |= Qt::ItemIsEnabled;
            break;

        case ColumnName:
            if (canRename && !isReadOnly)
            {
                itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
            }
            else
            {
                if (isReadOnly)
                {
                    itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
                }
                else
                {
                    itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
                }
            }
            break;

        default:
            itemFlags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
            break;
        }

        // Disable entities outside the focus subtree.
        if (!m_focusModeInterface->IsInFocusSubTree(entityId))
        {
            itemFlags &= !Qt::ItemIsEnabled;
        }

        return itemFlags;
    }

    Qt::DropActions EntityOutlinerListModelFromPrefab::supportedDropActions() const
    {
        return Qt::CopyAction;
    }

    Qt::DropActions EntityOutlinerListModelFromPrefab::supportedDragActions() const
    {
        return Qt::CopyAction;
    }


    bool EntityOutlinerListModelFromPrefab::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
    {
        if (data)
        {
            if (canDropMimeDataForEntityIds(data, action, row, column, parent))
            {
                return true;
            }

            if (CanDropMimeData(data, action, row, column, parent))
            {
                return true;
            }
        }

        return false;
    }

    bool EntityOutlinerListModelFromPrefab::canDropMimeDataForEntityIds(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
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
    bool EntityOutlinerListModelFromPrefab::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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

        // if its entity ids (internal drags of entities onto each other in the outliner), we handle this internally
        if (data->hasFormat(EditorEntityIdContainer::GetMimeType()))
        {
            return dropMimeDataEntities(data, action, row, column, parent);
        }

        // if its not what we handle internally, try handling it externally
        if (data->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            if (DropMimeData(data, action, row, column, parent))
            {
                return true;
            }
        }

        // nothing handled it, so let Qt have it
        return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
    }

    bool EntityOutlinerListModelFromPrefab::dropMimeDataComponentPalette(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
    
    bool EntityOutlinerListModelFromPrefab::CanDropMimeData(
        const QMimeData* data,
        [[maybe_unused]] Qt::DropAction action,
        [[maybe_unused]] int row,
        [[maybe_unused]] int column,
        const QModelIndex& parent) const
    {
        // Can only drop assets if a level is loaded!
        if (rowCount() == 0)
        {
            return false;
        }

        if (action != Qt::DropAction::CopyAction)
        {
            // we can only 'move' entityIds, and that will already be handled at this point
            // so if we get here, and its not a 'copy' operation, we must refuse.  Accepting
            // a 'move' operation for example will cause the sender to try to delete the object
            // if it is accepted.  (So for example dragging a file from windows explorer to
            // a view that accepts a 'move' operation will cause windows explorer to delete the
            // file, as it will be satisfied that the file has been safely 'moved' to the target.
            return false;
        }

        AZ::EntityId parentId = GetEntityFromIndex(parent);

        if (parentId.IsValid())
        {
            // Disable dropping assets on closed container entities.
            if (auto containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
                !containerEntityInterface->IsContainerOpen(parentId))
            {
                return false;
            }

            // Disable dropping assets on read-only entities.
            if (m_readOnlyEntityPublicInterface->IsReadOnly(parentId))
            {
                return false;
            }
        }

        // if the program gets here, it means that the thing being dropped on
        // is either the empty space, or its in something we are allowed to modify.
        // So, delegate responsibility to bus listeners.
        EntityOutlinerDragAndDropContext context;
        context.m_dataBeingDropped = data;
        context.m_parentEntity = parentId;
        bool accepted = false;
        AzQtComponents::DragAndDropItemViewEventsBus::Event(
            AzQtComponents::DragAndDropContexts::EntityOutliner,
            &AzQtComponents::DragAndDropItemViewEvents::CanDropItemView,
            accepted,
            context);

        return accepted;
    }

    bool EntityOutlinerListModelFromPrefab::DropMimeData(const QMimeData* data, [[maybe_unused]] Qt::DropAction action, int row, [[maybe_unused]] int column, const QModelIndex& parent)
    {
        AZ::EntityId assignParentId = GetEntityFromIndex(parent);

        // calling CanDropMimeDataAssets means not having to repeat redundant code here, orr forgetting to
        // add additional conditions that were added to CanDrop into the Do Drop.
        if (!CanDropMimeData(data, action, row, column, parent))
        {
            return false;
        }

        // delegate responsibility to listeners
        EntityOutlinerDragAndDropContext context;
        context.m_dataBeingDropped = data;
        context.m_parentEntity = assignParentId;
        bool accepted = false;
        AzQtComponents::DragAndDropItemViewEventsBus::Event(
            AzQtComponents::DragAndDropContexts::EntityOutliner, &AzQtComponents::DragAndDropItemViewEvents::DoDropItemView, accepted, context);

        return accepted;
    }

    bool EntityOutlinerListModelFromPrefab::dropMimeDataEntities(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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

    bool EntityOutlinerListModelFromPrefab::CanReparentEntities(const AZ::EntityId& newParentId, const EntityIdList &selectedEntityIds) const
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

        if (!EntitiesBelongToSamePrefab(selectedEntityIds, newParentId))
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
            ToolsApplicationRequests::Bus::BroadcastResult(
                isEntityEditable, &ToolsApplicationRequests::Bus::Events::IsEntityEditable, entityId);
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

    bool EntityOutlinerListModelFromPrefab::ReparentEntities(const AZ::EntityId& newParentId, const EntityIdList &selectedEntityIds, const AZ::EntityId& beforeEntityId, ReparentForInvalid forInvalid)
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

        ScopedUndoBatch undo("Reparent Entity(s)");
        // The new parent is dirty due to sort change(s)
        undo.MarkEntityDirty(GetEntityIdForSortInfo(newParentId));

        EntityIdList processedEntityIds;
        {
            ScopedUndoBatch undo2("Reparent Entity(s)");

            for (AZ::EntityId entityId : selectedEntityIds)
            {
                AZ::EntityId oldParentId;
                AZ::TransformBus::EventResult(oldParentId, entityId, &AZ::TransformBus::Events::GetParentId);

                //  Guarding this to prevent the entity from being marked dirty when the parent doesn't change.
                AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetParent, newParentId);

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

        ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, Refresh_Values);
        return true;
    }

    QMimeData* EntityOutlinerListModelFromPrefab::mimeData(const QModelIndexList& indexes) const
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

    QStringList EntityOutlinerListModelFromPrefab::mimeTypes() const
    {
        // the mimeTypes function needs to return the possible types of mimeData
        // that this model will PRODUCE, not which types it accepts.
        QStringList list = QAbstractItemModel::mimeTypes();
        list.append(EditorEntityIdContainer::GetMimeType());
        return list;
    }

    void EntityOutlinerListModelFromPrefab::QueueEntityUpdate(AZ::EntityId entityId)
    {
        if (m_layoutResetQueued)
        {
            return;
        }
        if (!m_entityChangeQueued)
        {
            m_entityChangeQueued = true;
            QTimer::singleShot(0, this, &EntityOutlinerListModelFromPrefab::ProcessEntityUpdates);
        }
        m_entityChangeQueue.insert(entityId);
    }

    void EntityOutlinerListModelFromPrefab::QueueAncestorUpdate(AZ::EntityId entityId)
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

    void EntityOutlinerListModelFromPrefab::QueueEntityToExpand(AZ::EntityId entityId, bool expand)
    {
        m_entityExpansionState[entityId] = expand;
        QueueEntityUpdate(entityId);
    }

    // Helper struct for tracking a range of indices.
    struct ModelIndexRange
    {
    public:
        QModelIndex m_start;
        QModelIndex m_end;
    };


    void EntityOutlinerListModelFromPrefab::ProcessEntityUpdates()
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
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModelFromPrefab::ProcessEntityUpdates:SelectQueue");
            for (auto entityId : m_entitySelectQueue)
            {
                emit SelectEntity(entityId, IsSelected(entityId));
            };
            m_entitySelectQueue.clear();
        }

        if (!m_entityChangeQueue.empty())
        {
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModelFromPrefab::ProcessEntityUpdates:ChangeQueue");

            for (auto entityId : m_entityChangeQueue)
            {
                if (const QModelIndex beginIndex = GetIndexFromEntity(entityId, ColumnName); beginIndex.isValid())
                {
                    const QModelIndex endIndex = createIndex(beginIndex.row(), VisibleColumnCount - 1, beginIndex.internalId());
                    emit dataChanged(beginIndex, endIndex);
                }
            }
        
            m_entityChangeQueue.clear();
        }

        {
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModelFromPrefab::ProcessEntityUpdates:LayoutChanged");
            if (m_entityLayoutQueued)
            {
                emit layoutAboutToBeChanged();
                emit layoutChanged();
            }
            m_entityLayoutQueued = false;
        }

        {
            AZ_PROFILE_SCOPE(Editor, "EntityOutlinerListModelFromPrefab::ProcessEntityUpdates:InvalidateFilter");
            if (m_isFilterDirty)
            {
                InvalidateFilter();
            }
        }
    }

    /*
    void EntityOutlinerListModelFromPrefab::OnEntityInfoResetBegin()
    {
        emit EnableSelectionUpdates(false);
        beginResetModel();
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoResetEnd()
    {
        m_layoutResetQueued = true;
        endResetModel();
        QTimer::singleShot(0, this, &EntityOutlinerListModelFromPrefab::ProcessEntityInfoResetEnd);
    }

    void EntityOutlinerListModelFromPrefab::ProcessEntityInfoResetEnd()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        m_layoutResetQueued = false;
        m_entityChangeQueued = false;
        m_entityChangeQueue.clear();
        QueueEntityUpdate(AZ::EntityId());
        emit EnableSelectionUpdates(true);
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedAddChildBegin(AZ::EntityId parentId, AZ::EntityId childId)
    {
        //add/remove operations trigger selection change signals which assert and break undo/redo operations in progress in inspector etc.
        //so disallow selection updates until change is complete
        emit EnableSelectionUpdates(false);
        auto parentIndex = GetIndexFromEntity(parentId);
        auto childIndex = GetIndexFromEntity(childId);
        beginInsertRows(parentIndex, childIndex.row(), childIndex.row());
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
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
    */

    void EntityOutlinerListModelFromPrefab::OnEntityRuntimeActivationChanged(AZ::EntityId entityId, [[maybe_unused]] bool activeOnStart)
    {
        QueueEntityUpdate(entityId);
    }

    void EntityOutlinerListModelFromPrefab::OnContainerEntityStatusChanged(AZ::EntityId entityId, [[maybe_unused]] bool open)
    {
        if (!Prefab::IsOutlinerOverrideManagementEnabled())
        {
            // Trigger a refresh of all direct children so that they can be shown or hidden appropriately.
            QueueEntityUpdate(entityId);

            EntityIdList children;
            EditorEntityInfoRequestBus::EventResult(children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);
            for (auto childId : children)
            {
                QueueEntityUpdate(childId);
            }

            // Always expand containers
            QueueEntityToExpand(entityId, true);
        }
        else
        {
            QModelIndex changedIndex = GetIndexFromEntity(entityId);

            // Trigger a refresh of all direct children so that they can be shown or hidden appropriately.
            int numChildren = rowCount(changedIndex);
            if (numChildren > 0)
            {
                emit dataChanged(index(0, 0, changedIndex), index(numChildren - 1, ColumnCount - 1, changedIndex));
            }
        } 
    }

    /*
    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedRemoveChildBegin([[maybe_unused]] AZ::EntityId parentId, [[maybe_unused]] AZ::EntityId childId)
    {
        //add/remove operations trigger selection change signals which assert and break undo/redo operations in progress in inspector etc.
        //so disallow selection updates until change is complete
        emit EnableSelectionUpdates(false);

        auto parentIndex = GetIndexFromEntity(parentId);
        auto childIndex = GetIndexFromEntity(childId);
        beginRemoveRows(parentIndex, childIndex.row(), childIndex.row());
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
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

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedOrderBegin(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index)
    {
        (void)parentId;
        (void)childId;
        (void)index;
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedOrderEnd(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index)
    {
        AZ_PROFILE_FUNCTION(Editor);
        (void)index;
        m_entityLayoutQueued = true;
        QueueEntityUpdate(parentId);
        QueueEntityUpdate(childId);
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedSelection(AZ::EntityId entityId, bool selected)
    {
        //update all ancestors because they will show highlight if ancestor is selected
        QueueAncestorUpdate(entityId);

        //expand ancestors upon new selection
        if (selected && m_autoExpandEnabled)
        {
            ExpandAncestors(entityId);
        }

        if (!m_suppressNextSelectEntity)
        {
            // notify observers
            emit SelectEntity(entityId, selected);
        }

        m_suppressNextSelectEntity = false;
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedLocked(AZ::EntityId entityId, bool)
    {
        // Prevent a SelectEntity call occurring during the update to stop the item clicked from scrolling away.
        m_suppressNextSelectEntity = true;

        //update all ancestors because they will show partial state for descendants
        QueueEntityUpdate(entityId);
        QueueAncestorUpdate(entityId);
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool)
    {
        // Prevent a SelectEntity call occurring during the update to stop the item clicked from scrolling away.
        m_suppressNextSelectEntity = true;

        //update all ancestors because they will show partial state for descendants
        QueueEntityUpdate(entityId);
        QueueAncestorUpdate(entityId);
    }

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name)
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

    void EntityOutlinerListModelFromPrefab::OnEntityInfoUpdatedUnsavedChanges(AZ::EntityId entityId)
    {
        QueueEntityUpdate(entityId);
    }
    */

    QModelIndex EntityOutlinerListModelFromPrefab::GetIndexFromEntity(const AZ::EntityId& entityId, int column) const
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

    AZ::EntityId EntityOutlinerListModelFromPrefab::GetEntityFromIndex(const QModelIndex& index) const
    {
        if (m_itemAliases.contains(index))
        {
            return m_rootInstance->get().GetEntityId(AZStd::string_view(m_itemAliases[index].toStdString().c_str()));
        }

        return AZ::EntityId();
    }

    void EntityOutlinerListModelFromPrefab::SearchStringChanged(const AZStd::string& filter)
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

    void EntityOutlinerListModelFromPrefab::SearchFilterChanged(const AZStd::vector<ComponentTypeValue>& componentFilters)
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

    bool EntityOutlinerListModelFromPrefab::ShouldOverrideUnfilteredSelection()
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

    void EntityOutlinerListModelFromPrefab::CacheSelectionIfAppropriate()
    {
        if (ShouldOverrideUnfilteredSelection())
        {
            ToolsApplicationRequests::Bus::BroadcastResult(m_unfilteredSelectionEntityIds, &ToolsApplicationRequests::GetSelectedEntities);
        }
    }

    void EntityOutlinerListModelFromPrefab::RestoreSelectionIfAppropriate()
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

    void EntityOutlinerListModelFromPrefab::AfterEntitySelectionChanged(const EntityIdList&, const EntityIdList&)
    {
        if (m_unfilteredSelectionEntityIds.size() > 0)
        {
            if (ShouldOverrideUnfilteredSelection())
            {
                m_unfilteredSelectionEntityIds.clear();
            }
        }
    }

    void EntityOutlinerListModelFromPrefab::OnEntityExpanded(const AZ::EntityId& entityId)
    {
        m_isFilterDirty = true;
        m_entityExpansionState[entityId] = true;
        QueueEntityUpdate(entityId);
    }

    void EntityOutlinerListModelFromPrefab::OnEntityCollapsed(const AZ::EntityId& entityId)
    {
        m_isFilterDirty = true;
        m_entityExpansionState[entityId] = false;
        QueueEntityUpdate(entityId);
    }

    void EntityOutlinerListModelFromPrefab::InvalidateFilter()
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

    void EntityOutlinerListModelFromPrefab::OnEditorEntityDuplicated(const AZ::EntityId& oldEntity, const AZ::EntityId& newEntity)
    {
        AZStd::list_iterator<AZStd::pair<AZ::EntityId, bool>> expansionIter = m_entityExpansionState.find(oldEntity);
        QueueEntityToExpand(newEntity, expansionIter != m_entityExpansionState.end() && expansionIter->second);
    }

    void EntityOutlinerListModelFromPrefab::ExpandAncestors(const AZ::EntityId& entityId)
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

    bool EntityOutlinerListModelFromPrefab::IsExpanded(const AZ::EntityId& entityId) const
    {
        auto expandedItr = m_entityExpansionState.find(entityId);
        return expandedItr != m_entityExpansionState.end() && expandedItr->second;
    }

    void EntityOutlinerListModelFromPrefab::RestoreDescendantExpansion(const AZ::EntityId& entityId)
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

    void EntityOutlinerListModelFromPrefab::RestoreDescendantSelection(const AZ::EntityId& entityId)
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

    bool EntityOutlinerListModelFromPrefab::FilterEntity(const AZ::EntityId& entityId)
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

    bool EntityOutlinerListModelFromPrefab::IsFiltered(const AZ::EntityId& entityId) const
    {
        auto hiddenItr = m_entityFilteredState.find(entityId);
        return hiddenItr != m_entityFilteredState.end() && hiddenItr->second;
    }

    void EntityOutlinerListModelFromPrefab::EnableAutoExpand(bool enable)
    {
        m_autoExpandEnabled = enable;
    }

    void EntityOutlinerListModelFromPrefab::SetDropOperationInProgress(bool inProgress)
    {
        m_dropOperationInProgress = inProgress;
    }

    bool EntityOutlinerListModelFromPrefab::HasSelectedDescendant(const AZ::EntityId& entityId) const
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

    bool EntityOutlinerListModelFromPrefab::AreAllDescendantsSameLockState(const AZ::EntityId& entityId) const
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

    bool EntityOutlinerListModelFromPrefab::AreAllDescendantsSameVisibleState(const AZ::EntityId& entityId) const
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

    void EntityOutlinerListModelFromPrefab::OnEntityInitialized(const AZ::EntityId& entityId)
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

    void EntityOutlinerListModelFromPrefab::OnEntityCompositionChanged(const EntityIdList& /*entityIds*/)
    {
        if (m_componentFilters.size() > 0)
        {
            m_isFilterDirty = true;
            emit ReapplyFilter();
        }
    }

    void EntityOutlinerListModelFromPrefab::OnPrepareForContextReset()
    {
        if (m_filterString.size() > 0 || m_componentFilters.size() > 0)
        {
            m_isFilterDirty = true;
            emit ResetFilter();
        }
    }

    void EntityOutlinerListModelFromPrefab::OnStartPlayInEditorBegin()
    {
        m_beginStartPlayInEditor = true;
    }

    void EntityOutlinerListModelFromPrefab::OnStartPlayInEditor()
    {
        m_beginStartPlayInEditor = false;
    }

    void EntityOutlinerListModelFromPrefab::OnEditorFocusChanged([[maybe_unused]] AZ::EntityId previousFocusEntityId, AZ::EntityId newFocusEntityId)
    {
        // Gather the Template Id of the root prefab being edited.
        Prefab::InstanceOptionalReference rootPrefabInstance = m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        if (rootPrefabInstance.has_value())
        {
            m_rootTemplateId = rootPrefabInstance->get().GetTemplateId();
            m_rootInstance = rootPrefabInstance;
            Generate();
        }

        // Ensure all descendants of the current focus root are expanded, so it is visible.
        ExpandAncestors(newFocusEntityId);
    }

    void EntityOutlinerListModelFromPrefab::Generate()
    {
        if (m_rootTemplateId == Prefab::InvalidTemplateId)
        {
            return;
        }

        const auto& rootDom = m_prefabSystemComponentInterface->FindTemplateDom(m_rootTemplateId);

        QModelIndex containerEntityIndex = createIndex(0, 0, rand());
        AZStd::string containerEntityName = rootDom.FindMember("ContainerEntity")->value.FindMember("Name")->value.GetString();
        AZStd::string containerEntityAlias = rootDom.FindMember("ContainerEntity")->value.FindMember("Id")->value.GetString();
        
        // Container Entity
        beginInsertRows(QModelIndex(), 0, 0);

        m_itemNames.insert(containerEntityIndex, containerEntityName.c_str());
        m_itemAliases.insert(containerEntityIndex, containerEntityAlias.c_str());
        m_indices[QModelIndex()][0] = containerEntityIndex;

        endInsertRows();

        // All children indiscriminately
        const auto& entities = rootDom.FindMember("Entities")->value;

        auto entitiesCount = entities.MemberCount();
        if (entitiesCount > 0)
        {
            beginInsertRows(containerEntityIndex, 0, entitiesCount - 1);

            int i = 0;
            for (Prefab::PrefabDom::ConstMemberIterator entityIter = entities.MemberBegin(); entityIter != entities.MemberEnd(); ++entityIter)
            {
                QModelIndex entityIndex = createIndex(i, 0, rand());

                m_itemNames.insert(entityIndex, entityIter->name.GetString());
                m_itemAliases.insert(entityIndex, entityIter->name.GetString());
                m_indices[containerEntityIndex][i] = entityIndex;
                ++i;
            }

            endInsertRows();
        }
    }
}

#include <UI/Outliner/moc_EntityOutlinerListModelFromPrefab.cpp>
