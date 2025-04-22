/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    NodePaletteAutoCompleteModel::NodePaletteAutoCompleteModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    QModelIndex NodePaletteAutoCompleteModel::index(int row, int column, const QModelIndex& /*parent*/) const
    {
        if (row >= m_availableItems.size())
        {
            return QModelIndex();
        }

        const GraphCanvas::GraphCanvasTreeItem* childItem = m_availableItems[row];

        // Const cast since Qt can't handle the data being const.
        return createIndex(row, column, const_cast<GraphCanvas::GraphCanvasTreeItem*>(childItem));
    }

    QModelIndex NodePaletteAutoCompleteModel::parent(const QModelIndex& /*index*/) const
    {
        return QModelIndex();
    }

    int NodePaletteAutoCompleteModel::columnCount(const QModelIndex& /*parent*/) const
    {
        return 1;
    }

    int NodePaletteAutoCompleteModel::rowCount(const QModelIndex& /*parent*/) const
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
        AZ_Assert(item, "null const GraphCanvas::GraphCanvasTreeItem* in QVariant NodePaletteAutoCompleteModel::data(const QModelIndex& index, int role) const");
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

    void NodePaletteAutoCompleteModel::AddAvailableItem(const GraphCanvas::GraphCanvasTreeItem* treeItem, bool signalAdd)
    {
        if (signalAdd)
        {
            beginInsertRows(QModelIndex(), aznumeric_cast<int>(m_availableItems.size()), aznumeric_cast<int>(m_availableItems.size()));
        }

        m_availableItems.push_back(treeItem);

        if (signalAdd)
        {
            endInsertRows();
        }
    }

    void NodePaletteAutoCompleteModel::RemoveAvailableItem(const GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        for (int i = 0; i < m_availableItems.size(); ++i)
        {
            if (m_availableItems[i] == treeItem)
            {
                beginRemoveRows(QModelIndex(), i, i);
                m_availableItems.erase(m_availableItems.begin() + i);
                endRemoveRows();
            }
        }
    }

    ////////////////////////////////////
    // NodePaletteSortFilterProxyModel
    ////////////////////////////////////

    NodePaletteSortFilterProxyModel::NodePaletteSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
        , m_unfilteredAutoCompleteModel(aznew NodePaletteAutoCompleteModel(this))
        , m_sourceSlotAutoCompleteModel(aznew NodePaletteAutoCompleteModel(this))
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
        int regexIndex = m_filterRegex.indexIn(test);

        if (regexIndex >= 0)
        {
            showRow = true;
            
            AZStd::pair<int, int> highlight(regexIndex, m_filterRegex.matchedLength());
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

    bool NodePaletteSortFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
    {
        QAbstractItemModel* model = sourceModel();
        if (m_filter.isEmpty())
        {
            // When item has no children, put it at front
            if (model->hasChildren(source_left) && !model->hasChildren(source_right))
            {
                return false;
            }
            else if (!model->hasChildren(source_left) && model->hasChildren(source_right))
            {
                return true;
            }
        }
        else
        {
            // Calculate a sorting score based on the filter
            const int leftScore = CalculateSortingScore(source_left);
            const int rightScore = CalculateSortingScore(source_right);
            if (leftScore != rightScore)
            {
                return leftScore < rightScore;
            }
        }

        // Fall back to a case insensitive alphabetical sort
        const QString left = model->data(source_left).toString();
        const QString right = model->data(source_right).toString();
        return left.compare(right, Qt::CaseInsensitive) < 0;
    }

    int NodePaletteSortFilterProxyModel::CalculateSortingScore(const QModelIndex& source) const
    {
        QAbstractItemModel* model = sourceModel();
        QString sourceString = model->data(source).toString();
        if (sourceString.compare(m_filter, Qt::CaseInsensitive) == 0)
        {
            return -1; // match has highest priority
        }

        int result = INT_MAX;
        // Calculate the score from children if available
        if (model->hasChildren(source))
        {
            for (int i = 0; i < model->rowCount(source); ++i)
            {
                QModelIndex child = model->index(i, 0, source);
                result = AZStd::min(result, CalculateSortingScore(child));
            }
        }
        // If name contains filter or filter regex, assuming shorter name has stronger relevance
        if (sourceString.contains(m_filter) || sourceString.contains(m_filterRegex))
        {
            result = AZStd::min(result, sourceString.size());
        }
        return result;
    }

    void NodePaletteSortFilterProxyModel::PopulateUnfilteredModel()
    {
        m_unfilteredAutoCompleteModel->beginResetModel();
        m_unfilteredAutoCompleteModel->ClearAvailableItems();

        GraphCanvas::GraphCanvasTreeModel* treeModel = static_cast<GraphCanvas::GraphCanvasTreeModel*>(sourceModel());

        QObject::connect(treeModel, &GraphCanvas::GraphCanvasTreeModel::OnTreeItemAdded, this, &NodePaletteSortFilterProxyModel::OnModelElementAdded);
        QObject::connect(treeModel, &GraphCanvas::GraphCanvasTreeModel::OnTreeItemAboutToBeRemoved, this, &NodePaletteSortFilterProxyModel::OnModelElementAboutToBeRemoved);

        const GraphCanvas::GraphCanvasTreeItem* treeItem = treeModel->GetTreeRoot();

        AZStd::list<const GraphCanvas::GraphCanvasTreeItem*> exploreItems;
        exploreItems.push_back(treeItem);

        while (!exploreItems.empty())
        {
            const GraphCanvas::GraphCanvasTreeItem* currentItem = exploreItems.front();
            exploreItems.pop_front();

            int numChildren = currentItem->GetChildCount();

            ProcessItemForUnfilteredModel(currentItem);

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

    void NodePaletteSortFilterProxyModel::FilterForSourceSlot(const AZ::EntityId& /*sceneId*/, const AZ::EntityId& /*sourceSlotId*/)
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
                m_sourceSlotAutoCompleteModel->AddAvailableItem(currentItem, false);
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
        // Remove whitespace and escape() so every regexp special character is escaped with a backslash
        // Then ignore all whitespace by adding \s* (regex optional whitespace match) in between every other character.
        // We use \s* instead of simply removing all whitespace from the filter and node-names in order to preserve the node-name and accurately highlight the matching portion.
        // Example: "OnGraphStart" or "On Graph Start"
        m_filter = filter.simplified().replace(" ", "");
        
        QString regExIgnoreWhitespace = QRegExp::escape(QString(m_filter[0]));
        for (int i = 1; i < m_filter.size(); ++i)
        {
            regExIgnoreWhitespace.append("\\s*");
            regExIgnoreWhitespace.append(QRegExp::escape(QString(m_filter[i])));
        }
        
        m_filterRegex = QRegExp(regExIgnoreWhitespace, Qt::CaseInsensitive);
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

    void NodePaletteSortFilterProxyModel::OnModelElementAdded(const GraphCanvasTreeItem* treeItem)
    {
        const bool signalAdd = true;
        ProcessItemForUnfilteredModel(treeItem, signalAdd);
    }

    void NodePaletteSortFilterProxyModel::OnModelElementAboutToBeRemoved(const GraphCanvasTreeItem* treeItem)
    {
        m_unfilteredCompleter.setCompletionPrefix("");
        m_unfilteredAutoCompleteModel->RemoveAvailableItem(treeItem);
    }

    void NodePaletteSortFilterProxyModel::ProcessItemForUnfilteredModel(const GraphCanvasTreeItem* currentItem, bool signalAdd)
    {
        const QModelIndex k_flagIndex;

        if ((currentItem->Flags(k_flagIndex) & Qt::ItemIsDragEnabled) != 0)
        {
            m_unfilteredAutoCompleteModel->AddAvailableItem(currentItem, signalAdd);
        }
    }

    #include <StaticLib/GraphCanvas/Widgets/NodePalette/Model/moc_NodePaletteSortFilterProxyModel.cpp>
}
