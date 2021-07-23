/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasAssetEditorMainWindow.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <QWidget>
#include <PrefabDependencyViewerInterface.h>
#include <AzCore/std/containers/unordered_map.h>

namespace PrefabDependencyViewer
{
    struct PrefabDependencyViewerConfig
        : GraphCanvas::AssetEditorWindowConfig
    {
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
        virtual ~PrefabDependencyViewerWidget();

        ////////////////// GraphCanvas::AssetEditorMainWindow overrides //////////////
        /** Sets up the GraphCanvas UI without the Node Palette. */
        virtual void SetupUI() override;

        ////////////////// PrefabDependencyViewerWidget overrides ////////////////////
        virtual void DisplayTree(const Utils::DirectedGraph& graph) override;

        void DisplayNodesByLevel(const Utils::DirectedGraph& graph, AZStd::vector<int> numNodesAtEachLevel, int widestLevel);

        void DisplayNode(Utils::Node* node, AZ::Vector2 pos);

        
        GraphCanvas::SlotId CreateExecutionSlot(
            GraphCanvas::NodeId nodeId,
            const AZStd::string& slotName,
            const AZStd::string& tooltip,
            GraphCanvas::SlotGroup slotGroup,
            bool isInput);

        GraphCanvas::SlotId CreateDataSlot(
            GraphCanvas::NodeId nodeId,
            const AZStd::string& slotName,
            const AZStd::string& tooltip,
            AZ::Uuid dataType,
            GraphCanvas::SlotGroup slotGroup,
            bool isInput);

        void AddSlotToNode(AZ::Entity* slotEntity, GraphCanvas::NodeId nodeId);
        void CreateNodeUi(const AzToolsFramework::Prefab::TemplateId& tid);

    protected:
        ////////////////// GraphCanvas::AssetEditorMainWindow overrides //////////////
        /** Overriding RefreshMenu in order to remove the
        unnecessary menu bar on the top. As a bonus, this
        also removes the ability to revive NodePalette from the UI. */
        virtual void RefreshMenu() override {}

        // GraphCanvas::GraphModelRequestBus::Handler overrides
        virtual void RequestUndoPoint() {}
        virtual void RequestPushPreventUndoStateUpdate() {}
        virtual void RequestPopPreventUndoStateUpdate() {}
        virtual void TriggerUndo() {}
        virtual void TriggerRedo() {}
        //! This is sent when a connection is disconnected.
        virtual void DisconnectConnection([[maybe_unused]]const ConnectionId& connectionId)
        {
        }
        //! This is sent when attempting to create a given connection.
        virtual bool CreateConnection(
            [[maybe_unused]] const ConnectionId& connectionId,
            [[maybe_unused]] const Endpoint& sourcePoint,
            [[maybe_unused]] const Endpoint& targetPoint)
        {
            return true;
        }
        //! This is sent to confirm whether or not a connection can take place.
        virtual bool IsValidConnection([[maybe_unused]] const Endpoint& sourcePoint, [[maybe_unused]] const Endpoint& targetPoint) const
        {
            return true;
        }
        //! This is sent to confirm whether or not a variable assignment can take place.
        virtual bool IsValidVariableAssignment([[maybe_unused]]const AZ::EntityId& variableId, [[maybe_unused]]const Endpoint& targetPoint) const
        {
            return true;
        }

                //! Get the Display Type name for the given AZ type
        virtual AZStd::string GetDataTypeString([[maybe_unused]] const AZ::Uuid& typeId)
        {
            return "";
        };

        // Signals out that the specified elements save data is dirty.
        virtual void OnSaveDataDirtied([[maybe_unused]]const AZ::EntityId& savedElement){};

        // Signals out that the graph was signeld to clean itself up.
        virtual void OnRemoveUnusedNodes()
        {
        }
        virtual void OnRemoveUnusedElements()
        {
        }
        virtual void ResetSlotToDefaultValue([[maybe_unused]] const Endpoint& endpoint)
        {
        }
        /*
        bool IsValidConnection([[maybe_unused]]const GraphCanvas::Endpoint& sourcePoint, [[maybe_unused]]const GraphCanvas::Endpoint& targetPoint) const override
        {
            return true;
        }
        */
    private:
        AZ::EntityId m_sceneId;
        AZStd::unordered_map<Utils::Node*, AZ::EntityId> nodeToNodeUiId;
        // To be able to map a Node to it's input and output slots in that order.
        AZStd::unordered_map<Utils::Node*, AZStd::pair<AZ::EntityId, AZ::EntityId>> nodeToSlotId;

    };
}; // namespace AzToolsFramework
