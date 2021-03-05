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
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAction>
#include <QTimer>
#include <qlabel.h>
#include <qitemselectionmodel.h>
#include <qstyleditemdelegate.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>
#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#endif

class QSortFilterProxyModel;

namespace Ui
{
    class NodePaletteDockWidget;
}

namespace GraphCanvas
{    
    class GraphCanvasMimeEvent;
    class NodePaletteTreeDelegate;
    class NodePaletteTreeView;
    class NodePaletteWidget;
    struct NodePaletteConfig;    

    class NodePaletteDockWidget
        : public AzQtComponents::StyledDockWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(NodePaletteDockWidget, AZ::SystemAllocator, 0);

        NodePaletteDockWidget(GraphCanvasTreeItem* treeItem, const EditorId& editorId, const QString& windowLabel, QWidget* parent, const char* mimeType, bool inContextMenu, AZStd::string_view identifier);
        NodePaletteDockWidget(QWidget* parent, const QString& windowLabel, const NodePaletteConfig& configuration);

        ~NodePaletteDockWidget();

        void FocusOnSearchFilter();

        void ResetModel();
        void ResetDisplay();

        GraphCanvasMimeEvent* GetContextMenuEvent() const;

        void ResetSourceSlotFilter();
        void FilterForSourceSlot(const AZ::EntityId& sceneId, const AZ::EntityId& sourceSlotId);

        void SetItemDelegate(NodePaletteTreeDelegate* itemDelegate);

        void AddHeaderWidget(QWidget* widget);
        void ConfigureHeaderMargins(const QMargins& margins, int elementSpacing);

        void AddFooterWidget(QWidget* widget);
        void ConfigureFooterMargins(const QMargins& margins, int elementSpacing);

        void AddSearchCustomizationWidget(QWidget* widget);
        void ConfigureSearchCustomizationMargins(const QMargins& margins, int elementSpacing);

        const GraphCanvasTreeItem* GetTreeRoot() const;

    protected:

        GraphCanvasTreeItem* ModTreeRoot();

        NodePaletteTreeView* GetTreeView() const;
        NodePaletteWidget* GetNodePaletteWidget() const;

        // This method here is to help facilitate resetting the model. This will not be called during
        // the initial construction(because yay virtual functions).
        virtual GraphCanvasTreeItem* CreatePaletteRoot() const;

    signals:
        void OnContextMenuSelection();        

    private:

        AZStd::unique_ptr<Ui::NodePaletteDockWidget> m_ui;
        EditorId m_editorId;
    };
}
