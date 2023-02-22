/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QCompleter>
#include <QSortFilterProxyModel>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#endif

namespace GraphCanvas
{
    class GraphCanvasTreeItem;
    class NodePaletteSortFilterProxyModel;

    // Should be a private class of the other model.
    // Needs to be external because of Q_OBJECT not supporting nested classes.
    class NodePaletteAutoCompleteModel
        : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        friend class NodePaletteSortFilterProxyModel;

        AZ_CLASS_ALLOCATOR(NodePaletteAutoCompleteModel, AZ::SystemAllocator);

        NodePaletteAutoCompleteModel(QObject* parent = nullptr);
        ~NodePaletteAutoCompleteModel() = default;

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        const GraphCanvas::GraphCanvasTreeItem* FindItemForIndex(const QModelIndex& index);

    private:

        void ClearAvailableItems();
        void AddAvailableItem(const GraphCanvas::GraphCanvasTreeItem* item, bool signalAdd);
        void RemoveAvailableItem(const GraphCanvas::GraphCanvasTreeItem* item);

        AZStd::vector<const GraphCanvas::GraphCanvasTreeItem*> m_availableItems;
    };

    class NodePaletteSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(NodePaletteSortFilterProxyModel, AZ::SystemAllocator);

        NodePaletteSortFilterProxyModel(QObject* parent);

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

        void PopulateUnfilteredModel();

        void ResetSourceSlotFilter();
        void FilterForSourceSlot(const AZ::EntityId& sceneId, const AZ::EntityId& sourceSlotId);

        bool HasFilter() const;
        void SetFilter(const QString& filter);
        void ClearFilter();

        QCompleter* GetCompleter();

        void OnModelElementAdded(const GraphCanvasTreeItem* treeItem);
        void OnModelElementAboutToBeRemoved(const GraphCanvasTreeItem* treeItem);

    private:
        int CalculateSortingScore(const QModelIndex& source) const;
        void ProcessItemForUnfilteredModel(const GraphCanvasTreeItem* currentItem, bool signalAdd = false);

        QCompleter m_unfilteredCompleter;
        QCompleter m_sourceSlotCompleter;

        NodePaletteAutoCompleteModel* m_unfilteredAutoCompleteModel;
        NodePaletteAutoCompleteModel* m_sourceSlotAutoCompleteModel;

        bool m_hasSourceSlotFilter;
        AZStd::unordered_set<const GraphCanvas::GraphCanvasTreeItem*> m_sourceSlotFilter;

        QString m_filter;
        QRegExp m_filterRegex;
    };
}
