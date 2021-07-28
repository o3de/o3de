/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>

#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasAssetEditorMainWindow.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>

#include <MainWindowInterface.h>
#include <QWidget>

namespace PrefabDependencyViewer
{
    struct PrefabDependencyViewerConfig
        : GraphCanvas::AssetEditorWindowConfig
    {
        AZ_CLASS_ALLOCATOR(PrefabDependencyViewerConfig, AZ::SystemAllocator, 0);
        /** Return an empty NodePalette tree */
        GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteRoot() override;
    };

    using Endpoint = GraphCanvas::Endpoint;
    using ConnectionId = GraphCanvas::ConnectionId;

    class PrefabDependencyViewerWidget 
        : public GraphCanvas::AssetEditorMainWindow
        , public PrefabDependencyViewerInterface
        , private GraphCanvas::GraphModelRequestBus::Handler
    {
        Q_OBJECT;
    public:
        explicit PrefabDependencyViewerWidget(QWidget* pParent = NULL);
        ~PrefabDependencyViewerWidget() override;

        ////////////////// GraphCanvas::AssetEditorMainWindow overrides //////////////
        /** Sets up the GraphCanvas UI without the Node Palette. */
        void SetupUI() override;

        ////////////////// PrefabDependencyViewerWidget overrides ////////////////////
        void DisplayTree(const Utils::DirectedGraph& graph) override;

        void DisplayNodesByLevel(const Utils::DirectedGraph& graph, AZStd::vector<int> numNodesAtEachLevel, int widestLevel);

        void DisplayNode(Utils::Node* node, AZ::Vector2 pos);

        GraphCanvas::SlotId CreateDataSlot(
            GraphCanvas::NodeId nodeId,
            const AZStd::string& slotName,
            const AZStd::string& tooltip,
            AZ::Uuid dataType,
            GraphCanvas::SlotGroup slotGroup,
            bool isInput);

        void AddSlotToNode(AZ::Entity* slotEntity, GraphCanvas::NodeId nodeId);

    protected:
        ////////////////// GraphCanvas::AssetEditorMainWindow overrides //////////////
        /** Overriding RefreshMenu in order to remove the
        unnecessary menu bar on the top. As a bonus, this
        also removes the ability to revive NodePalette from the UI. */
        void RefreshMenu() override {}

        // GraphCanvas::GraphModelRequestBus::Handler overrides
        void RequestUndoPoint() override {}
        void RequestPushPreventUndoStateUpdate() override {}
        void RequestPopPreventUndoStateUpdate() override {}
        void TriggerUndo() override {}
        void TriggerRedo() override {}
        //! This is sent when a connection is disconnected.
        void DisconnectConnection([[maybe_unused]]const ConnectionId& connectionId) override
        {
        }
        //! This is sent when attempting to create a given connection.
        bool CreateConnection(
            [[maybe_unused]] const ConnectionId& connectionId,
            [[maybe_unused]] const Endpoint& sourcePoint,
            [[maybe_unused]] const Endpoint& targetPoint) override
        {
            return true;
        }
        //! This is sent to confirm whether or not a connection can take place.
        bool IsValidConnection([[maybe_unused]] const Endpoint& sourcePoint, [[maybe_unused]] const Endpoint& targetPoint) const override
        {
            return true;
        }

        //! Get the Display Type name for the given AZ type
        AZStd::string GetDataTypeString([[maybe_unused]] const AZ::Uuid& typeId) override
        {
            return {};
        };

        // Signals out that the specified elements save data is dirty.
        void OnSaveDataDirtied([[maybe_unused]]const AZ::EntityId& savedElement) override {};

        // Signals out that the graph was signaled to clean itself up.
        void OnRemoveUnusedNodes() override { }
        void OnRemoveUnusedElements() override { }
        void ResetSlotToDefaultValue([[maybe_unused]] const Endpoint& endpoint) override { }

    private:
        AZ::EntityId m_sceneId;
        AZStd::unordered_map<Utils::Node*, AZ::EntityId> nodeToNodeUiId;
        // To be able to map a Node to it's input and output slots in that order.
        AZStd::unordered_map<Utils::Node*, AZStd::pair<AZ::EntityId, AZ::EntityId>> nodeToSlotId;

    };
}; // namespace AzToolsFramework
