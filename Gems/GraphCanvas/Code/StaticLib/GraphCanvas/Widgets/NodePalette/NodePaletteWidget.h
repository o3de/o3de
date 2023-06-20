/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QAction>
#include <QTimer>
#include <qlabel.h>
#include <qitemselectionmodel.h>
#include <qstyleditemdelegate.h>
AZ_POP_DISABLE_WARNING

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
    class NodePaletteWidget;
}

namespace GraphCanvas
{
    class GraphCanvasMimeEvent;
    class NodePaletteDockWidget;
    class NodePaletteSortFilterProxyModel;
    class NodePaletteTreeView;
    
    class NodePaletteTreeDelegate : public IconDecoratedNameDelegate
    {
    public:
        AZ_CLASS_ALLOCATOR(NodePaletteTreeDelegate, AZ::SystemAllocator);

        NodePaletteTreeDelegate(QWidget* parent = nullptr);
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    };

    struct NodePaletteConfig
    {
        GraphCanvasTreeItem* m_rootTreeItem = nullptr;
        EditorId m_editorId;

        const char* m_mimeType = "";

        AZStd::string_view m_saveIdentifier;

        bool m_isInContextMenu = false;
        bool m_clearSelectionOnSceneChange = true;

        bool m_allowArrowKeyNavigation = true;
    };

    class NodePaletteWidget
        : public QWidget
        , public AssetEditorNotificationBus::Handler
        , public GraphCanvasTreeModelRequestBus::Handler
    {
        Q_OBJECT

    private:
        friend class NodePaletteDockWidget;

    public:
        AZ_CLASS_ALLOCATOR(NodePaletteWidget, AZ::SystemAllocator);

        NodePaletteWidget(QWidget* parent);
        ~NodePaletteWidget();

        void SetupNodePalette(const NodePaletteConfig& paletteConfig);

        void FocusOnSearchFilter();

        void ResetModel(GraphCanvasTreeItem* rootItem = nullptr);
        void ResetDisplay();

        GraphCanvasMimeEvent* GetContextMenuEvent() const;

        void ResetSourceSlotFilter();
        void FilterForSourceSlot(const AZ::EntityId& sceneId, const AZ::EntityId& sourceSlotId);

        void SetItemDelegate(NodePaletteTreeDelegate* itemDelegate);

        void AddSearchCustomizationWidget(QWidget* widget);
        void ConfigureSearchCustomizationMargins(const QMargins& margins, int elementSpacing);

        // AssetEditorNotificationBus
        void PreOnActiveGraphChanged() override;
        void PostOnActiveGraphChanged() override;
        ////

        // GraphCanvasTreeModelRequestBus::Handler
        void ClearSelection() override;
        ////

        // NodeTreeView
        const GraphCanvasTreeItem* GetTreeRoot() const;
        NodePaletteTreeView* GetTreeView() const;
        ////

        QLineEdit* GetSearchFilter() const;

        // QWidget
        bool eventFilter(QObject* object, QEvent* event) override;
        ////

        NodePaletteSortFilterProxyModel* GetFilterModel();
        
        GraphCanvasTreeItem* FindItemWithName(QString name);

    protected:

        GraphCanvasTreeItem* ModTreeRoot();

        // This method here is to help facilitate resetting the model. This will not be called during
        // the initial construction(because yay virtual functions).
        virtual GraphCanvasTreeItem* CreatePaletteRoot() const;

    public slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnScrollChanged(int scrollPosition);

    signals:
        void OnCreateSelection();
        void OnSelectionCleared();
        void OnTreeItemSelected(const GraphCanvasTreeItem* treeItem);

    private:

        void RefreshFloatingHeader();
        void OnFilterTextChanged(const QString &text);
        void UpdateFilter();
        void ClearFilter();

        void OnRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);

        // Will try and spawn the item specified by the QCompleter
        void TrySpawnItem();

        void HandleSelectedItem(const GraphCanvasTreeItem* treeItem);

        AZStd::string m_mimeType;
        AZStd::string m_saveIdentifier;

        bool m_isInContextMenu;
        bool m_searchFieldSelectionChange;

        AZStd::unique_ptr<Ui::NodePaletteWidget> m_ui;
        NodePaletteTreeDelegate* m_itemDelegate;

        EditorId m_editorId;
        AZStd::unique_ptr<GraphCanvasMimeEvent> m_contextMenuCreateEvent;

        QTimer        m_filterTimer;
        NodePaletteSortFilterProxyModel* m_model;
    };
}
