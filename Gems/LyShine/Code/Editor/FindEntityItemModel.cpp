/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "FindEntityItemModel.h"

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiCanvasBus.h>

#include <QObject>
#include <QWidget>
#include <QPalette>
#include <QBrush>

FindEntityItemModel::FindEntityItemModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_entityFilteredState()
    , m_entityMatchState()
{
}

void FindEntityItemModel::Initialize(AZ::EntityId canvasEntityId)
{
    beginResetModel();

    m_canvasEntityId = canvasEntityId;

    endResetModel();
}

int FindEntityItemModel::rowCount(const QModelIndex& parent) const
{
    int childCount = 0;

    auto parentId = GetEntityFromIndex(parent);
    if (parentId.IsValid())
    {
        EBUS_EVENT_ID_RESULT(childCount, parentId, UiElementBus, GetNumChildElements);
    }
    else
    {
        // Root element
        EBUS_EVENT_ID_RESULT(childCount, m_canvasEntityId, UiCanvasBus, GetNumChildElements);
    }

    return childCount;
}

int FindEntityItemModel::columnCount(const QModelIndex& /*parent*/) const
{
    return ColumnCount;
}

QModelIndex FindEntityItemModel::index(int row, int column, const QModelIndex& parent) const
{
    // sanity check
    if (!hasIndex(row, column, parent) || (parent.isValid() && parent.column() != 0) || (row < 0 || row >= rowCount(parent)))
    {
        return QModelIndex();
    }

    auto parentId = GetEntityFromIndex(parent);

    AZ::EntityId childId;
    if (parentId.IsValid())
    {
        EBUS_EVENT_ID_RESULT(childId, parentId, UiElementBus, GetChildEntityId, row);
    }
    else
    {
        // Root element
        EBUS_EVENT_ID_RESULT(childId, m_canvasEntityId, UiCanvasBus, GetChildElementEntityId, row);
    }

    return createIndex(row, column, static_cast<AZ::u64>(childId));
}

QVariant FindEntityItemModel::data(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);
    if (id.IsValid())
    {
        switch (index.column())
        {
        case ColumnName:
            return DataForName(index, role);
        }
    }

    return QVariant();
}

QModelIndex FindEntityItemModel::parent(const QModelIndex& index) const
{
    auto id = GetEntityFromIndex(index);
    if (id.IsValid())
    {
        AZ::EntityId parentId;
        EBUS_EVENT_ID_RESULT(parentId, id, UiElementBus, GetParentEntityId);
        return GetIndexFromEntity(parentId, index.column());
    }
    return QModelIndex();
}

QModelIndex FindEntityItemModel::GetIndexFromEntity(const AZ::EntityId& entityId, int column) const
{
    if (entityId.IsValid())
    {
        AZ::EntityId parentId;
        EBUS_EVENT_ID_RESULT(parentId, entityId, UiElementBus, GetParentEntityId);

        if (parentId.IsValid())
        {
            AZStd::size_t row = 0;
            EBUS_EVENT_ID_RESULT(row, parentId, UiElementBus, GetIndexOfChildByEntityId, entityId);
            return createIndex(static_cast<int>(row), column, static_cast<AZ::u64>(entityId));
        }
    }
    return QModelIndex();
}

AZ::EntityId FindEntityItemModel::GetEntityFromIndex(const QModelIndex& index) const
{
    return index.isValid() ? AZ::EntityId(static_cast<AZ::u64>(index.internalId())) : AZ::EntityId();
}

void FindEntityItemModel::SearchStringChanged(const AZStd::string& filter)
{
    m_filterString = filter;
    InvalidateFilter();
}

void FindEntityItemModel::SearchFilterChanged(const AZStd::vector<AZ::Uuid>& componentFilters)
{
    m_componentFilters = AZStd::move(componentFilters);
    InvalidateFilter();
}

QVariant FindEntityItemModel::DataForName(const QModelIndex& index, int role) const
{
    auto id = GetEntityFromIndex(index);

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        AZStd::string name;
        EBUS_EVENT_ID_RESULT(name, id, UiElementBus, GetName);
        return QString(name.data());
    }

    case Qt::ForegroundRole:
    {
        const QColor notMatchedTextColor(130, 130, 130);

        // We use the parent's palette because the GUI Application palette is returning the wrong colors
        auto parentWidgetPtr = static_cast<QWidget*>(QObject::parent());
        return QBrush(IsMatch(id) ? parentWidgetPtr->palette().color(QPalette::Text) : notMatchedTextColor);
    }

    case VisibilityRole:
    {
        return !IsFiltered(id);
    }
    }

    return QVariant();
}

void FindEntityItemModel::InvalidateFilter()
{
    FilterEntity(AZ::EntityId());
}

bool FindEntityItemModel::FilterEntity(const AZ::EntityId& entityId)
{
    bool isFilterMatch = true;

    if (m_filterString.size() > 0)
    {
        if (entityId.IsValid())
        {
            AZStd::string name;
            EBUS_EVENT_ID_RESULT(name, entityId, UiElementBus, GetName);

            if (AzFramework::StringFunc::Find(name.c_str(), m_filterString.c_str()) == AZStd::string::npos)
            {
                isFilterMatch = false;
            }
        }
    }

    // If we matched the filter string and have any component filters, make sure we match those too
    if (isFilterMatch && m_componentFilters.size() > 0)
    {
        if (entityId.IsValid())
        {
            bool hasMatchingComponent = false;

            for (AZ::Uuid& componentType : m_componentFilters)
            {
                if (AzToolsFramework::EntityHasComponentOfType(entityId, componentType))
                {
                    hasMatchingComponent = true;
                    break;
                }
            }

            isFilterMatch &= hasMatchingComponent;
        }
    }

    AzToolsFramework::EntityIdList children;
    if (entityId.IsValid())
    {
        EBUS_EVENT_ID_RESULT(children, entityId, UiElementBus, GetChildEntityIds);
    }
    else
    {
        // Root element
        EBUS_EVENT_ID_RESULT(children, m_canvasEntityId, UiCanvasBus, GetChildElementEntityIds);
    }

    m_entityMatchState[entityId] = isFilterMatch;

    for (auto childId : children)
    {
        isFilterMatch |= FilterEntity(childId);
    }
 
    m_entityFilteredState[entityId] = !isFilterMatch;

    return isFilterMatch;
}

bool FindEntityItemModel::IsFiltered(const AZ::EntityId& entityId) const
{
    auto hiddenItr = m_entityFilteredState.find(entityId);
    return hiddenItr != m_entityFilteredState.end() && hiddenItr->second;
}

bool FindEntityItemModel::IsMatch(const AZ::EntityId& entityId) const
{
    auto matchItr = m_entityMatchState.find(entityId);
    return matchItr == m_entityMatchState.end() || matchItr->second;
}

#include <moc_FindEntityItemModel.cpp>
