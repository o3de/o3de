/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzCore/Interface/Interface.h>
#include <Core/Core.h>
#include <PrefabDependencyViewerWidget.h>
#include <QBoxLayout>
#include <QLabel>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>

namespace PrefabDependencyViewer
{
    GraphCanvas::GraphCanvasTreeItem* PrefabDependencyViewerConfig::CreateNodePaletteRoot()
    {
        const GraphCanvas::EditorId& editorId = PREFAB_DEPENDENCY_VIEWER_EDITOR_ID;
        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", editorId);

        return rootItem;
    }

    /** Returns a bare minimum configuration to configure GraphCanvas UI
    for the purpose of visualizing the Prefab hierarchy. */
    PrefabDependencyViewerConfig* GetDefaultConfig()
    {
        // Make sure no memory leak happens here
        PrefabDependencyViewerConfig* config = new PrefabDependencyViewerConfig();
        config->m_editorId = PREFAB_DEPENDENCY_VIEWER_EDITOR_ID;
        config->m_baseStyleSheet = "PrefabDependencyViewer/StyleSheet/graphcanvas_style.json";

        return config;
    }

    PrefabDependencyViewerWidget::PrefabDependencyViewerWidget(QWidget* pParent)
        : GraphCanvas::AssetEditorMainWindow(GetDefaultConfig(), pParent)
    {
        AZ::Interface<PrefabDependencyViewerInterface>::Register(this);
    }

    void PrefabDependencyViewerWidget::SetupUI() {
        GraphCanvas::AssetEditorMainWindow::SetupUI();
        delete m_nodePalette;
        m_nodePalette = nullptr;
    }

    void PrefabDependencyViewerWidget::DisplayTree([[maybe_unused]]const Utils::DirectedGraph& graph)
    {
        m_sceneId = CreateNewGraph();
        GraphCanvas::GraphModelRequestBus::Handler::BusConnect(m_sceneId);

        int widestLevelSize = 0;
        AZStd::vector nodeCountAtEachLevel = graph.countNodesAtEachLevel(widestLevelSize);
        DisplayNodesByLevel(graph, nodeCountAtEachLevel, widestLevelSize);
        // CreateNodeUi(AzToolsFramework::Prefab::InvalidTemplateId);
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

                Utils::NodeSet currChildren = graph.GetChildren(currNode);
                for (Utils::Node* currChild : currChildren)
                {
                    queue.push(currChild);
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
        ;

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

        /* GraphCanvas::SlotId inputSlotId =
                CreateExecutionSlot(nodeUiId, "Input", "Child",
                                            GraphCanvas::SlotGroups::ExecutionGroup, true);
        GraphCanvas::SlotId outputSlotId = CreateExecutionSlot(nodeUiId, "Output", "Parent",
                                            GraphCanvas::SlotGroups::ExecutionGroup, false);
        */
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
        const AZStd::string& slotName,
        const AZStd::string& tooltip,
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
        if (isInput)
        {
            dataSlotConfiguration.m_connectionType = GraphCanvas::CT_Input;
        }
        else
        {
            dataSlotConfiguration.m_connectionType = GraphCanvas::CT_Output;
        }

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
    GraphCanvas::SlotId PrefabDependencyViewerWidget::CreateExecutionSlot(
        GraphCanvas::NodeId nodeId,
        const AZStd::string& slotName,
        const AZStd::string& tooltip,
        GraphCanvas::SlotGroup slotGroup,
        bool isInput)
    {
        GraphCanvas::ExecutionSlotConfiguration executionConfiguration;
        executionConfiguration.m_name = slotName;
        executionConfiguration.m_tooltip = tooltip;
        executionConfiguration.m_slotGroup = slotGroup;

        // Need to specify the ConnectionType for this slot.
        if (isInput)
        {
            executionConfiguration.m_connectionType = GraphCanvas::CT_Input;
        }
        else
        {
            executionConfiguration.m_connectionType = GraphCanvas::CT_Output;
        }

        AZ::Entity* slotEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
            slotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, nodeId, executionConfiguration);

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

    void PrefabDependencyViewerWidget::CreateNodeUi([[maybe_unused]]const AzToolsFramework::Prefab::TemplateId& tid)
    {
        const char* nodeStyle = "";
        const AZ::Entity* graphCanvasNode = nullptr;;

        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
            graphCanvasNode,
            &GraphCanvas::GraphCanvasRequests::CreateGeneralNodeAndActivate,
            nodeStyle);

        AZ_Assert(graphCanvasNode, "Unable to create GraphCanvas Node");

        AZ::EntityId nodeUiId = graphCanvasNode->GetId();
        GraphCanvas::NodeTitleRequestBus::Event(nodeUiId, &GraphCanvas::NodeTitleRequests::SetTitle, "Prefab");

        GraphCanvas::SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::AddNode, nodeUiId, AZ::Vector2(5, 5), false);
        GraphCanvas::SceneMemberUIRequestBus::Event(nodeUiId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        // AddNodeUiToScene()
    }

    PrefabDependencyViewerWidget::~PrefabDependencyViewerWidget() {
        AZ::Interface<PrefabDependencyViewerInterface>::Unregister(this);
    }
}

// Qt best practice for Q_OBJECT macro issues. File available at compile time.
#include <moc_PrefabDependencyViewerWidget.cpp>


