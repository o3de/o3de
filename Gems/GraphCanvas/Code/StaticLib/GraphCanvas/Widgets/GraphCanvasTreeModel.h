/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <qmimedata.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#endif

namespace GraphCanvas
{
    class GraphCanvasTreeModelRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphCanvasTreeModel*;

        virtual void ClearSelection() = 0;
    };

    using GraphCanvasTreeModelRequestBus = AZ::EBus<GraphCanvasTreeModelRequests>;

    //! Contains all the information required to build any Tree based widget that will support Drag/Drop with the GraphicsView.
    class GraphCanvasTreeModel
        : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasTreeModel, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflectContext);

        GraphCanvasTreeModel(GraphCanvasTreeItem* treeRoot, QObject* parent = nullptr);
        ~GraphCanvasTreeModel() = default;
        
        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
        ////
        
        void setMimeType(const char* mimeType);
        const GraphCanvas::GraphCanvasTreeItem* GetTreeRoot() const;
        GraphCanvas::GraphCanvasTreeItem* ModTreeRoot();

        QModelIndex CreateIndex(GraphCanvasTreeItem* treeItem, int column = 0);
        QModelIndex CreateParentIndex(GraphCanvasTreeItem* treeItem, int column = 0);

        void ChildAboutToBeAdded(GraphCanvasTreeItem* parentItem, int position = -1);
        void OnChildAdded(GraphCanvasTreeItem* itemAdded);

    signals:

        void OnTreeItemAdded(const GraphCanvasTreeItem* treeItem);
        void OnTreeItemAboutToBeRemoved(const GraphCanvasTreeItem* treeItem);
        
    public:

        QString                                m_mimeType;
        AZStd::unique_ptr<GraphCanvasTreeItem> m_treeRoot;
    };
}
