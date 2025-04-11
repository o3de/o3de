/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <qabstractitemmodel.h>
#include <qitemdelegate.h>
#include <qobject.h>
#include <qregexp.h>
#include <qsortfilterproxymodel.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>

#endif

namespace GraphCanvas
{
    class NodeTableSourceModel
        : public QAbstractTableModel
        , public GraphCanvas::NodeTitleNotificationsBus::MultiHandler
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        enum ColumnDescriptor
        {
            CD_IndexForce = -1,

            CD_Name,

            CD_Count
        };

        AZ_CLASS_ALLOCATOR(NodeTableSourceModel, AZ::SystemAllocator);

        NodeTableSourceModel();
        ~NodeTableSourceModel();

        // QAbstractTableModel
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& index = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        ////


        // SceneNotificationBus
        void OnNodeAdded(const AZ::EntityId& /*nodeId*/, bool /*isPaste*/) override;
        void OnNodeRemoved(const AZ::EntityId& /*nodeId*/) override;
        ////

        // GraphCanvas::NodeTitleNotificationsBus
        void OnTitleChanged() override;
        ////

        void SetActiveScene(const AZ::EntityId& sceneId);
        AZ::EntityId FindNodeByIndex(const QModelIndex& index);
        void JumpToNodeArea(const QModelIndex& index);
    private:
        void ReSortNodes();

        AZ::EntityId m_activeGraph;
        EditorId m_activeEditorId;
        AZStd::vector<AZ::EntityId> m_nodes;
        AZStd::unordered_map<AZ::EntityId, AZStd::string> m_nodeNames;
        AZStd::mutex m_nodexMtx;
    };

    class NodeTableSortProxyModel : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeTableSortProxyModel, AZ::SystemAllocator);

        NodeTableSortProxyModel(NodeTableSourceModel* sourceModel);
        ~NodeTableSortProxyModel() override = default;

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        void SetFilter(const QString& filter);
        void ClearFilter();

    private:
        QString m_filter;
        QRegExp m_filterRegex;
    };
}
