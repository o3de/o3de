/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/sort.h>

#include <GraphCanvas/Widgets/GraphOutliner/GraphOutlinerTableModel.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>

namespace GraphCanvas
{
    ////////////////////////
    // NodeTableSourceModel
    ////////////////////////

    NodeTableSourceModel::NodeTableSourceModel()
    {

    }

    NodeTableSourceModel ::~NodeTableSourceModel()
    {

    }

    void NodeTableSourceModel::SetActiveScene(const AZ::EntityId& graphId)
    {
        m_activeGraph = graphId;
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        GraphCanvas::NodeTitleNotificationsBus::MultiHandler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_activeGraph);
        
        layoutAboutToBeChanged();

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_nodexMtx);
            m_nodes.clear();
            m_nodeNames.clear();
            GraphCanvas::SceneRequestBus::EventResult(m_nodes, m_activeGraph, &GraphCanvas::SceneRequests::GetNodes);
            for (auto& nodeId: m_nodes)
            {
                AZStd::string nodeName;
                GraphCanvas::NodeTitleRequestBus::EventResult(nodeName, nodeId, &GraphCanvas::NodeTitleRequests::GetTitle);
                m_nodeNames[nodeId] = nodeName;

                GraphCanvas::NodeTitleNotificationsBus::MultiHandler::BusConnect(nodeId);
            }
            ReSortNodes();
        }
        
        layoutChanged();
    }

    int NodeTableSourceModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return static_cast<int>(m_nodes.size());
    }

    int NodeTableSourceModel::columnCount([[maybe_unused]] const QModelIndex& index) const
    {
        return CD_Count;
    }

    QVariant NodeTableSourceModel::data(const QModelIndex& index, int role) const
    {
        return index.isValid() && role == Qt::DisplayRole ? m_nodeNames.at(m_nodes[index.row()]).c_str() : QVariant();
    }

    bool NodeTableSourceModel::setData([[maybe_unused]] const QModelIndex& index, [[maybe_unused]] const QVariant& value, [[maybe_unused]] int role)
    {
        return true;
    }

    Qt::ItemFlags NodeTableSourceModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags flags = QAbstractTableModel::flags(index);
        flags |= Qt::ItemFlag::ItemIsEnabled;
        flags |= Qt::ItemFlag::ItemIsSelectable;
        flags &= ~Qt::ItemFlag::ItemIsEditable;
        return index.isValid() ? flags : Qt::ItemFlag();
    }
    
    void NodeTableSourceModel::OnNodeAdded(const AZ::EntityId& nodeId,[[maybe_unused]] bool isPaste)
    {
        GraphCanvas::NodeTitleNotificationsBus::MultiHandler::BusConnect(nodeId);
        AZStd::lock_guard<AZStd::mutex> lock(m_nodexMtx);

        layoutAboutToBeChanged();
        m_nodes.emplace_back(nodeId);

        AZStd::string nodeName;
        GraphCanvas::NodeTitleRequestBus::EventResult(nodeName, nodeId, &GraphCanvas::NodeTitleRequests::GetTitle);
        m_nodeNames[nodeId] = nodeName;
        ReSortNodes();

        layoutChanged();
    }
    void NodeTableSourceModel::OnNodeRemoved(const AZ::EntityId& nodeId)
    {
        GraphCanvas::NodeTitleNotificationsBus::MultiHandler::BusDisconnect(nodeId);
        AZStd::lock_guard<AZStd::mutex> lock(m_nodexMtx);
        auto nodeIter = AZStd::find<AZStd::vector<AZ::EntityId>::iterator, AZ::EntityId>(m_nodes.begin(), m_nodes.end(), nodeId);
        if (nodeIter != m_nodes.end())
        {
            int row = aznumeric_cast<int>(nodeIter - m_nodes.begin());
            beginRemoveRows(QModelIndex(), row, row);
            m_nodes.erase(nodeIter);
            m_nodeNames.erase(nodeId);
            endRemoveRows();
        }
    }

    void NodeTableSourceModel::OnTitleChanged()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_nodexMtx);
        layoutAboutToBeChanged();
        for (auto nodeId : m_nodes)
        {
            AZStd::string nodeName;
            GraphCanvas::NodeTitleRequestBus::EventResult(nodeName, nodeId, &GraphCanvas::NodeTitleRequests::GetTitle);
            m_nodeNames[nodeId] = nodeName;
        }
        ReSortNodes();
        layoutChanged();
    }

    void NodeTableSourceModel::ReSortNodes()
    {
        AZStd::sort(
            m_nodes.begin(),
            m_nodes.end(),
            [this](const AZ::EntityId& a, const AZ::EntityId& b)
            {
                return m_nodeNames[a] < m_nodeNames[b];
            });
    }

    AZ::EntityId NodeTableSourceModel::FindNodeByIndex(const QModelIndex& index)
    {
        if (index.isValid() and index.row() < m_nodes.size())
        {
            return m_nodes[index.row()];
        }
        else
        {
            return AZ::EntityId();
        }
    }

    void NodeTableSourceModel::JumpToNodeArea(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return;
        }

        AZ::EntityId nodeId = FindNodeByIndex(index);
        QGraphicsItem* graphicsItem;
        SceneMemberUIRequestBus::EventResult(graphicsItem, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);
        ViewId viewId;

        SceneRequestBus::EventResult(viewId, m_activeGraph, &SceneRequests::GetViewId);
        QRectF rect;
        ViewRequestBus::EventResult(rect, viewId, &ViewRequests::GetViewableAreaInSceneCoordinates);

        rect.moveCenter(graphicsItem->sceneBoundingRect().center());

        ViewRequestBus::Event(viewId, &ViewRequests::DisplayArea, rect);
    }

    //////////////////////////
    // NodeTableSortProxyModel
    //////////////////////////
    
    NodeTableSortProxyModel::NodeTableSortProxyModel(NodeTableSourceModel* sourceModel)
    {
        setSourceModel(sourceModel);
    }

    bool NodeTableSortProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_filter.isEmpty())
        {
            return true;
        }

        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        QString typeName = sourceModel()->data(index, Qt::DisplayRole).toString();

        return typeName.lastIndexOf(m_filterRegex) >= 0;
    }

    void NodeTableSortProxyModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);

        invalidateFilter();
    }
    void NodeTableSortProxyModel::ClearFilter()
    {
        SetFilter("");
    }
}
