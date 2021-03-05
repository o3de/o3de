/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef FIND_ENTITY_WIDGET_H
#define FIND_ENTITY_WIDGET_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityId.h>
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
    AZ_CLASS_ALLOCATOR(FindEntityWidget, AZ::SystemAllocator, 0)

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
