/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef FIND_ENTITY_WIDGET_H
#define FIND_ENTITY_WIDGET_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <QWidget>
#include <QItemSelection>
#endif

#pragma once

class FindEntityItemModel;
class FindEntitySortFilterProxyModel;
class QPushButton;

class FindEntityWidget
    : public QWidget
{
    Q_OBJECT;
public:
    AZ_CLASS_ALLOCATOR(FindEntityWidget, AZ::SystemAllocator)

    FindEntityWidget(AZ::EntityId canvasEntityId, QWidget* pParent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
    virtual ~FindEntityWidget();

Q_SIGNALS:

    void OnFinished(AZStd::vector<AZ::EntityId> selectedEntities);
    void OnCanceled();

    private Q_SLOTS:
    void OnSelectionChanged(const QItemSelection&, const QItemSelection&);
    void OnItemDblClick(const QModelIndex& index);
    void OnSearchTextChanged(const QString& activeTextFilter);
    void OnFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);
    void OnSelectClicked();
    void OnCancelClicked();
    
private:
    AZ::EntityId GetEntityIdFromIndex(const QModelIndex& index) const;
    QModelIndex GetIndexFromEntityId(const AZ::EntityId& entityId) const;

    void SetupUI();

    void GetUsedComponents(AZ::EntityId canvasEntityId, AZStd::unordered_set<AZ::Uuid>& usedComponents);

    AzQtComponents::FilteredSearchWidget* m_searchWidget;
    QTreeView* m_objectTree;
    QPushButton* m_selectButton;
    QPushButton* m_cancelButton;

    FindEntityItemModel* m_listModel;
    FindEntitySortFilterProxyModel* m_proxyModel;
};

#endif
