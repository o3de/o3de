/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <MainWindow.h>
#include <QBoxLayout>
#include <QLabel>

namespace PrefabDependencyViewer
{
    static const GraphCanvas::EditorId PrefabDependencyViewerEditorId = AZ_CRC_CE("PrefabDependencyViewerEditor");

    GraphCanvas::GraphCanvasTreeItem* PrefabDependencyViewerConfig::CreateNodePaletteRoot()
    {
        const GraphCanvas::EditorId& editorId = PrefabDependencyViewerEditorId;
        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", editorId);

        return rootItem;
    }

    /** Returns a bare minimum configuration to configure GraphCanvas UI
    for the purpose of visualizing the Prefab hierarchy. */
    PrefabDependencyViewerConfig* GetDefaultConfig()
    {
        // Make sure no memory leak happens here
        PrefabDependencyViewerConfig* config = aznew PrefabDependencyViewerConfig();
        config->m_editorId = PrefabDependencyViewerEditorId;
        config->m_baseStyleSheet = "PrefabDependencyViewer/StyleSheet/graphcanvas_style.json";

        return config;
    }

    PrefabDependencyViewerWidget::PrefabDependencyViewerWidget(QWidget* pParent)
        : GraphCanvas::AssetEditorMainWindow(GetDefaultConfig(), pParent)
    {
        AZ::Interface<PrefabDependencyViewerInterface>::Register(this);
    }

    void PrefabDependencyViewerWidget::SetupUI()
    {
        GraphCanvas::AssetEditorMainWindow::SetupUI();
        delete m_nodePalette;
        m_nodePalette = nullptr;
    }

    void PrefabDependencyViewerWidget::DisplayTree(const Utils::DirectedGraph& graph)
    {
        m_sceneId = CreateNewGraph();
        GraphCanvas::GraphModelRequestBus::Handler::BusConnect(m_sceneId);

        const auto [nodeCountAtEachLevel, widestLevelSize] = graph.countNodesAtEachLevel();
        DisplayNodesByLevel(graph, nodeCountAtEachLevel, widestLevelSize);
    }

    void PrefabDependencyViewerWidget::DisplayNodesByLevel(const Utils::DirectedGraph& graph, [[maybe_unused]] AZStd::vector<int> numNodesAtEachLevel, [[maybe_unused]] int widestLevelSize)
    {
        AZStd::queue<Utils::Node*> queue;
        queue.push(graph.GetRoot());

        int stepDown = 100;
        int stepRight = 250;
        double currDepth = 10;

        for (int level = 0; level < numNodesAtEachLevel.size(); ++level)
        {
            double currRight = (widestLevelSize - 1.0 * (numNodesAtEachLevel[level] - 1) / 2) * stepRight;
            for (int nodeNum = 0; nodeNum < numNodesAtEachLevel[level]; ++nodeNum)
            {
                Utils::Node* currNode = queue.front();
                queue.pop();

                AZ::Vector2 pos(currRight, currDepth);

                DisplayNode(currNode, pos);

                AZStd::optional<Utils::NodeSet> currChildren = graph.GetChildren(currNode);
                if (AZStd::nullopt != currChildren && currChildren.has_value())
                {
                    for (Utils::Node* currChild : currChildren.value())
                    {
                        queue.push(currChild);
                    }
                }
                currRight += stepRight;
            }

            currDepth += stepDown;
        }

        // Queue should be empty once done with all levels.
        AZ_Assert(queue.empty(), "Queue should be empty.");
    }

    void PrefabDependencyViewerWidget::DisplayNode(Utils::Node* node, AZ::Vector2 pos)
    {
        const char* nodeStyle = "";
        const AZ::Entity* graphCanvasNode = nullptr;

        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
            graphCanvasNode, &GraphCanvas::GraphCanvasRequests::CreateGeneralNodeAndActivate, nodeStyle);

        AZ_Assert(graphCanvasNode, "Unable to create GraphCanvas Node");

        AZ::EntityId nodeUiId = graphCanvasNode->GetId();
        nodeToNodeUiId[node] = nodeUiId;

        GraphCanvas::NodeTitleRequestBus::Event(nodeUiId, &GraphCanvas::NodeTitleRequests::SetTitle, node->GetMetaData().GetSource());

        GraphCanvas::SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::AddNode, nodeUiId, pos, false);
        GraphCanvas::SceneMemberUIRequestBus::Event(nodeUiId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

        // Add slot
        GraphCanvas::SlotLayoutRequestBus::Event(
            nodeUiId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup,
            GraphCanvas::SlotGroupConfiguration(1));

        GraphCanvas::SlotId inputSlotId = CreateDataSlot(nodeUiId, "Input", "Parent",
                                                        azrtti_typeid<AZ::EntityId>(),
                                                        GraphCanvas::SlotGroups::DataGroup, true);

        GraphCanvas::SlotId outputSlotId = CreateDataSlot(nodeUiId, "Output", "Child",
                                                        azrtti_typeid<AZ::EntityId>(),
                                                        GraphCanvas::SlotGroups::DataGroup, false);

        nodeToSlotId[node] = AZStd::make_pair(inputSlotId, outputSlotId);

        // This might break if the parent is nullptr
        if (node->GetParent())
        {
            AZ::EntityId sourceNodeUiId = nodeToNodeUiId[node->GetParent()];
            AZ::EntityId sourceSlotUiId = nodeToSlotId[node->GetParent()].second;

            AZ::EntityId connectionUiId;
            GraphCanvas::SceneRequestBus::EventResult(
                connectionUiId, m_sceneId, &GraphCanvas::SceneRequests::CreateConnectionBetween,
                GraphCanvas::Endpoint(sourceNodeUiId, sourceSlotUiId),
                GraphCanvas::Endpoint(nodeUiId, inputSlotId));
        }
    }
    GraphCanvas::SlotId PrefabDependencyViewerWidget::CreateDataSlot(
        GraphCanvas::NodeId nodeId,
        AZStd::string&& slotName,
        AZStd::string&& tooltip,
        AZ::Uuid dataType,
        GraphCanvas::SlotGroup slotGroup,
        bool isInput)
    {
        GraphCanvas::DataSlotConfiguration dataSlotConfiguration;
        dataSlotConfiguration.m_typeId = dataType;
        dataSlotConfiguration.m_dataSlotType = GraphCanvas::DataSlotType::Value;

        dataSlotConfiguration.m_name = slotName;
        dataSlotConfiguration.m_tooltip = tooltip;
        dataSlotConfiguration.m_slotGroup = slotGroup;

        // Need to specify the ConnectionType for this slot.
        dataSlotConfiguration.m_connectionType = isInput ? GraphCanvas::CT_Input : GraphCanvas::CT_Output;
   
        AZ::Entity* slotEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
            slotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, nodeId, dataSlotConfiguration);

        if (slotEntity)
        {
            // Any customization to the Slot Entity will need to be done here before being activated.

            AddSlotToNode(slotEntity, nodeId);
        }

        return slotEntity ? slotEntity->GetId() : GraphCanvas::SlotId();
    }

    void PrefabDependencyViewerWidget::AddSlotToNode(AZ::Entity* slotEntity, GraphCanvas::NodeId nodeId)
    {
        slotEntity->Init();
        slotEntity->Activate();

        // At this point the Slot's user data should be set to help tie it to whatever the underlying model wants.

        GraphCanvas::NodeRequestBus::Event(nodeId, &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());
    }

    PrefabDependencyViewerWidget::~PrefabDependencyViewerWidget() {
        AZ::Interface<PrefabDependencyViewerInterface>::Unregister(this);
    }
}

// Qt best practice for Q_OBJECT macro issues. File available at compile time.
#include <moc_MainWindow.cpp>


