/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>
// qvariant.h(457) : error C2220 : warning treated as error - no 'object' file generated
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <qabstractitemmodel.h>
AZ_POP_DISABLE_WARNING

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional_basic.h>

namespace GraphCanvas
{
    class GraphCanvasMimeEvent;
    class GraphCanvasTreeModel;
    
    class GraphCanvasTreeItem
    {
    private:
        struct Comparator
        {
            Comparator() = default;

            bool operator()(const GraphCanvasTreeItem* lhs, const GraphCanvasTreeItem* rhs) const
            {
                return lhs->LessThan(rhs);
            }
        };

        friend struct Comparator;
        friend class GraphCanvasTreeModel;
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasTreeItem, AZ::SystemAllocator);
        AZ_RTTI(GraphCanvasTreeItem, "{BB2B829D-64B5-4D33-9390-85056AA0F3AA}");

        virtual ~GraphCanvasTreeItem();

        // Categorizer Specific Methods
        void SetAllowPruneOnEmpty(bool allowsEmpty);
        bool AllowPruneOnEmpty() const;
        ////

        void DetachItem();
        int GetChildCount() const;

        void ClearChildren();

        GraphCanvasTreeItem* FindChildByRow(int row) const;
        int FindRowUnderParent() const;

        GraphCanvasTreeItem* GetParent() const;

        void RegisterModel(GraphCanvasTreeModel* itemModel);

        QModelIndex GetIndexFromModel();
        
        // Fields that can be customized to manipulate how the tree view behaves.
        virtual int GetColumnCount() const = 0;
        virtual Qt::ItemFlags Flags(const QModelIndex& index) const = 0;
        virtual QVariant Data(const QModelIndex& index, int role) const = 0;

        virtual bool SetData([[maybe_unused]] const QModelIndex& index, [[maybe_unused]] const QVariant& value, [[maybe_unused]] int role) { return false; }
        virtual GraphCanvasMimeEvent* CreateMimeEvent() const { return nullptr; };

        template<class NodeType, class... Params>
        NodeType* CreateChildNode(Params&&... rest)
        {
            NodeType* nodeType = aznew NodeType(AZStd::forward<Params>(rest)...);
            this->AddChild(nodeType);
            return nodeType;
        }

        template<class NodeType, class... Params>
        NodeType* CreateChildNodeWithoutAddSignal(Params&&... rest)
        {
            NodeType* nodeType = aznew NodeType(AZStd::forward<Params>(rest)...);
            this->AddChild(nodeType, false);
            return nodeType;
        }

    protected:

        GraphCanvasTreeItem();

        int FindRowForChild(const GraphCanvasTreeItem* item) const;
        
        void RemoveParent(GraphCanvasTreeItem* item);

        void AddChild(GraphCanvasTreeItem* item, bool signalAdd = true);
        void RemoveChild(GraphCanvasTreeItem* item, bool deleteObject = true);

        void SignalLayoutAboutToBeChanged();
        void SignalLayoutChanged();
        void SignalDataChanged();
        
        // Overrides for various internal bits of operation
        virtual bool LessThan(const GraphCanvasTreeItem* graphItem) const;
        virtual void PreOnChildAdded(GraphCanvasTreeItem* item);
        virtual void OnChildAdded(GraphCanvasTreeItem* item);
        virtual void OnChildDataChanged(GraphCanvasTreeItem* item);
        
    private:

        void ClearModel();

        GraphCanvasTreeModel*   m_abstractItemModel;
        bool m_allowSignals;
        bool m_deleteRemoveChildren;

        bool m_allowPruneOnEmpty;

        GraphCanvasTreeItem* m_parent;
        AZStd::vector< GraphCanvasTreeItem* > m_childItems;
    };
}
