/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef FIND_ENTITY_ITEM_MODEL_H
#define FIND_ENTITY_ITEM_MODEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>

#include <QAbstractItemModel>
#endif

#pragma once

//! Model for items in the "Find Entity" tree view.
//! Each item represents an Entity.
class FindEntityItemModel
    : public QAbstractItemModel
{
    Q_OBJECT;

public:
    AZ_CLASS_ALLOCATOR(FindEntityItemModel, AZ::SystemAllocator);

    //! Columns of data to display about each Entity.
    enum Column
    {
        ColumnName,                 //!< Entity name
        ColumnCount                 //!< Total number of columns
    };

    enum Roles
    {
        VisibilityRole = Qt::UserRole + 1,
        RoleCount
    };

    FindEntityItemModel(QObject* parent = nullptr);

    void Initialize(AZ::EntityId canvasEntityId);

    // Qt overrides.
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    QModelIndex GetIndexFromEntity(const AZ::EntityId& entityId, int column = 0) const;
    AZ::EntityId GetEntityFromIndex(const QModelIndex& index) const;

    void SearchStringChanged(const AZStd::string& filter);
    void SearchFilterChanged(const AZStd::vector<AZ::Uuid>& componentFilters);

protected:

    QVariant DataForName(const QModelIndex& index, int role) const;

    //! Use the current filter setting and re-evaluate the filter.
    void InvalidateFilter();

    bool FilterEntity(const AZ::EntityId& entityId);
    bool IsFiltered(const AZ::EntityId& entityId) const;
    bool IsMatch(const AZ::EntityId& entityId) const;

    AZStd::string m_filterString;
    AZStd::vector<AZ::Uuid> m_componentFilters;

    AZStd::unordered_map<AZ::EntityId, bool> m_entityFilteredState;
    AZStd::unordered_map<AZ::EntityId, bool> m_entityMatchState;

    AZ::EntityId m_canvasEntityId;
};

Q_DECLARE_METATYPE(AZ::ComponentTypeList); // allows type to be stored by QVariable

#endif
