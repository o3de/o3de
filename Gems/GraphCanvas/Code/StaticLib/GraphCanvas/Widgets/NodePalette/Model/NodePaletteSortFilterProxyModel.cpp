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
#include <QRegExp>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

#include <GraphCanvas/Widgets/NodePalette/Model/NodePaletteSortFilterProxyModel.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace GraphCanvas
{
    /////////////////////////////////
    // NodePaletteAutoCompleteModel
    /////////////////////////////////

    NodePaletteAutoCompleteModel::NodePaletteAutoCompleteModel()
    {
    }

    QModelIndex NodePaletteAutoCompleteModel::index(int row, int column, [[maybe_unused]] const QModelIndex& parent) const
    {
        if (row >= m_availableItems.size())
        {
            return QModelIndex();
        }

        const GraphCanvas::GraphCanvasTreeItem* childItem = m_availableItems[row];

        // Const cast since Qt can't handle the data being const.
        return createIndex(row, column, const_cast<GraphCanvas::GraphCanvasTreeItem*>(childItem));
    }

    QModelIndex NodePaletteAutoCompleteModel::parent([[maybe_unused]] const QModelIndex& index) const
    {
        return QModelIndex();
    }

    int NodePaletteAutoCompleteModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return 1;
    }

    int NodePaletteAutoCompleteModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return static_cast<int>(m_availableItems.size());
    }

    QVariant NodePaletteAutoCompleteModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        const GraphCanvas::GraphCanvasTreeItem* item = static_cast<const GraphCanvas::GraphCanvasTreeItem*>(index.internalPointer());
        return item->Data(index, role);
    }

    const GraphCanvas::GraphCanvasTreeItem* NodePaletteAutoCompleteModel::FindItemForIndex(const QModelIndex& index)
    {
        if (!index.isValid() || index.row() >= m_availableItems.size())
        {
            return nullptr;
        }

        return m_availableItems[index.row()];
    }

    void NodePaletteAutoCompleteModel::ClearAvailableItems()
    {
        m_availableItems.clear();
    }

    void NodePaletteAutoCompleteModel::AddAvailableItem(const GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        m_availableItems.push_back(treeItem);
    }

    ////////////////////////////////////
    // NodePaletteSortFilterProxyModel
    ////////////////////////////////////

    NodePaletteSortFilterProxyModel::NodePaletteSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
        , m_unfilteredAutoCompleteModel(aznew NodePaletteAutoCompleteModel())
        , m_sourceSlotAutoCompleteModel(aznew NodePaletteAutoCompleteModel())
        , m_hasSourceSlotFilter(false)
    {
        m_unfilteredCompleter.setModel(m_unfilteredAutoCompleteModel);
        m_unfilteredCompleter.setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
        m_unfilteredCompleter.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

        m_sourceSlotCompleter.setModel(m_sourceSlotAutoCompleteModel);
        m_sourceSlotCompleter.setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
        m_sourceSlotCompleter.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    }

    bool NodePaletteSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QAbstractItemModel* model = sourceModel();
        QModelIndex index = model->index(sourceRow, 0, sourceParent);

        NodePaletteTreeItem* currentItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(index.internalPointer());

        if (m_hasSourceSlotFilter && m_sourceSlotFilter.count(currentItem) == 0)
        {
            return false;
        }

        if (m_filter.isEmpty())
        {
            currentItem->ClearHighlight();
            return true;
        }

        QString test = model->data(index).toString();

        bool showRow = false;
        int regexIndex = test.lastIndexOf(m_filterRegex);

        if (regexIndex >= 0)
        {
            showRow = true;

            AZStd::pair<int, int> highlight(regexIndex, m_filter.size());
            currentItem->SetHighlight(highlight);
        }
        else
        {
            currentItem->ClearHighlight();
        }

        // If we have children. We need to remain visible if any of them are valid.
        if (!showRow && sourceModel()->hasChildren(index))
        {
            for (int i = 0; i < sourceModel()->rowCount(index); ++i)
            {
                if (filterAcceptsRow(i, index))
                {
                    showRow = true;
                    break;
                }
            }
        }

        // We also want to display ourselves if any of our parents match the filter
        QModelIndex parentIndex = sourceModel()->parent(index);
        while (!showRow && parentIndex.isValid())
        {
            QString test2 = model->data(parentIndex).toString();
            showRow = test2.contains(m_filterRegex);

            parentIndex = sourceModel()->parent(parentIndex);
        }

        return showRow;
    }

    void NodePaletteSortFilterProxyModel::PopulateUnfilteredModel()
    {
        m_unfilteredAutoCompleteModel->beginResetModel();
        m_unfilteredAutoCompleteModel->ClearAvailableItems();

        GraphCanvas::GraphCanvasTreeModel* treeModel = static_cast<GraphCanvas::GraphCanvasTreeModel*>(sourceModel());

        const GraphCanvas::GraphCanvasTreeItem* treeItem = treeModel->GetTreeRoot();

        AZStd::list<const GraphCanvas::GraphCanvasTreeItem*> exploreItems;
        exploreItems.push_back(treeItem);

        const QModelIndex k_flagIndex;

        while (!exploreItems.empty())
        {
            const GraphCanvas::GraphCanvasTreeItem* currentItem = exploreItems.front();
            exploreItems.pop_front();

            int numChildren = currentItem->GetChildCount();

            if ((currentItem->Flags(k_flagIndex) & Qt::ItemIsDragEnabled) != 0)
            {
                m_unfilteredAutoCompleteModel->AddAvailableItem(currentItem);
            }

            for (int i = 0; i < numChildren; ++i)
            {
                exploreItems.push_back(currentItem->FindChildByRow(i));
            }
        }

        m_unfilteredAutoCompleteModel->endResetModel();
    }

    void NodePaletteSortFilterProxyModel::ResetSourceSlotFilter()
    {
        if (m_hasSourceSlotFilter)
        {
            m_hasSourceSlotFilter = false;
            m_sourceSlotFilter.clear();

            invalidateFilter();
        }
    }

    void NodePaletteSortFilterProxyModel::FilterForSourceSlot([[maybe_unused]] const AZ::EntityId& sceneId, [[maybe_unused]] const AZ::EntityId& sourceSlotId)
    {
        m_hasSourceSlotFilter = true;
        m_sourceSlotAutoCompleteModel->beginResetModel();
        m_sourceSlotAutoCompleteModel->ClearAvailableItems();

        GraphCanvas::GraphCanvasTreeModel* treeModel = static_cast<GraphCanvas::GraphCanvasTreeModel*>(sourceModel());

        const GraphCanvas::GraphCanvasTreeItem* treeItem = treeModel->GetTreeRoot();

        AZStd::list<const GraphCanvas::GraphCanvasTreeItem*> exploreItems;
        exploreItems.push_back(treeItem);

        const QModelIndex k_flagIndex;

        while (!exploreItems.empty())
        {
            const GraphCanvas::GraphCanvasTreeItem* currentItem = exploreItems.front();
            exploreItems.pop_front();

            // TODO the actual filter
            m_sourceSlotFilter.emplace(currentItem);

            int numChildren = currentItem->GetChildCount();

            if ((currentItem->Flags(k_flagIndex) & Qt::ItemIsDragEnabled) != 0)
            {
                m_sourceSlotAutoCompleteModel->AddAvailableItem(currentItem);
            }

            for (int i = 0; i < numChildren; ++i)
            {
                exploreItems.push_back(currentItem->FindChildByRow(i));
            }
        }

        m_sourceSlotAutoCompleteModel->endResetModel();
        invalidateFilter();
    }

    bool NodePaletteSortFilterProxyModel::HasFilter() const
    {
        return !m_filter.isEmpty();
    }

    void NodePaletteSortFilterProxyModel::SetFilter(const QString& filter)
    {
        m_filter = QRegExp::escape(filter);
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);
    }

    void NodePaletteSortFilterProxyModel::ClearFilter()
    {
        m_filter.clear();
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);
    }

    QCompleter* NodePaletteSortFilterProxyModel::GetCompleter()
    {
        if (m_hasSourceSlotFilter)
        {
            return &m_sourceSlotCompleter;
        }
        else
        {
            return &m_unfilteredCompleter;
        }
    }

    #include <StaticLib/GraphCanvas/Widgets/NodePalette/Model/moc_NodePaletteSortFilterProxyModel.cpp>
}
